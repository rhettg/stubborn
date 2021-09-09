#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include "evt.h"
#include "com.h"
#include "ci.h"
#include "to.h"
#include "stubborn.h"

#include "command.h"

#define TO_FILE    "/var/stubborn/to"
#define SOCK_FILE  "/var/stubborn/comsock"
#define RADIO_FILE "/var/stubborn/radio"

EVT_t evt = {0};
TMR_t tmr = {0};
COM_t com = {0};
CI_t  ci  = {0};

int current_client = 0;
int radio = 0;
int server = 0;

FILE *to_file = {0};

void fatal(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

void debugEvent(EVT_Event_t *e)
{
    printf("[%ld] Event: %d\n", millis(), e->type);
}

unsigned long millis()
{
  struct timespec tp;
  if (0 != clock_gettime(CLOCK_MONOTONIC_RAW, &tp)) {
    perror("failed to get clock");
  }

  return tp.tv_sec * 1000 + lround(tp.tv_nsec/1.0e6);
}

int open_to_file()
{
  to_file = fopen(TO_FILE, "a");
  if (NULL == to_file) {
    perror("failed to open /var/stubborn/to");
    return -1;
  }

  return 0;
}

int open_client_sock()
{
    struct sockaddr_un addr;
    int fd;

    fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (0 != unlink(SOCK_FILE)) {
        if (ENOENT != errno) {
            perror("failed to remove");
            return -1;
        }
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_FILE, sizeof(addr.sun_path)-1);

    if (0 != bind(fd, (struct sockaddr*)&addr, sizeof(addr))) {
        perror("failed to open socket");
        return -1;
    }

    if (0 != listen(fd, 1)) {
        perror("failed to listen");
        return -1;
    }

    return fd;
}

int open_radio_sock()
{
    struct sockaddr_un addr;
    int fd;

    fd = socket(AF_UNIX, SOCK_STREAM|SOCK_NONBLOCK, 0);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, RADIO_FILE, sizeof(addr.sun_path)-1);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
      perror("connect error");
      return -1;
    }

    return fd;
}

int run_server()
{
    char buf[1024];
    size_t rlen;
    fd_set set;

    struct timeval tv = {0, 100};   // sleep for ten minutes!

    printf("starting server\n");

    while (1) {
        TMR_handle(&tmr, millis());

        FD_ZERO(&set);

        FD_SET(radio, &set);

        if (0 == current_client) {
            FD_SET(server, &set);
        } else {
            FD_SET(current_client, &set);
        }

        int ret = select(FD_SETSIZE, &set, NULL, NULL, &tv);
        if (0 > ret) {
            perror("failed to select");
            continue;
        }

        if (0 != FD_ISSET(radio, &set)) {
            rlen = read(radio, buf, sizeof(buf));
            if (rlen < 0 && EAGAIN != errno) {
                fprintf(stderr, "failed reading: %d\n", errno);
                rlen = 0;
            }

            if (0 == rlen) {
              fprintf(stderr, "radio closed, exiting\n");
              close(radio);
              radio = 0;
              return 0;
            }

            printf("read %ld bytes\n", rlen);

            int cret = COM_recv(&com, (uint8_t *)buf, rlen, millis());
            if (cret != 0) {
              printf("error from COM: %d\n", cret);
              continue;
            }
        }

        if (0 != FD_ISSET(server, &set)) {
            printf("accepting to connection\n");
            current_client = accept(server, 0, 0);
            if (current_client < 0) {
                fprintf(stderr, "failed to accept: %d\n", errno);
                continue;
            }
        }

        if (0 != FD_ISSET(current_client, &set)) {
            printf("reading request from client\n");
            rlen = read(current_client, buf, sizeof(buf));
            if (rlen < 0 && EAGAIN != errno) {
                fprintf(stderr, "failed reading: %d\n", errno);
                rlen = 0;
            }

            if (0 < rlen) {
                buf[rlen] = 0;
                printf("RX: %s\n", buf);
                int rp = parse_ci_command(&ci, buf);
                if (0 != rp) {
                    fprintf(stderr, "failed parsing: %d\n", rp);
                    if (0 > send(current_client, "ERR\n", 4, 0)) {
                        perror("failed to report error");
                    }
                } else {
                    // It isn't OK yet actually.
                    //if (0 > send(current_client, "OK\n", 4, 0)) {
                        //perror("failed to report error");
                    //}
                }
            } else {
                printf("closing client\n");
                close(current_client);
                current_client = 0;
            }
        }
    }

    return 0;
}

void rfm_notify(EVT_Event_t *evt) {
  if (COM_EVT_TYPE_DATA != evt->type) {
    return;
  }

  COM_Data_Event_t *d_evt = (COM_Data_Event_t *)evt;
#ifdef DEBUG
  printf("sending %lu bytes: ", d_evt->length);
  for(int i = 0; i < d_evt->length; i++) {
    printf("0x%x ", d_evt->data[i]);
  }
  printf("\n");
#endif
  printf("TX: (%lu len)\n", d_evt->length);

  int n = write(radio, d_evt->data, d_evt->length);
  if (0 > n) {
    perror("failed to write");
  } else if (n < d_evt->length) {
    printf("short write: %d %ld\n", n , d_evt->length);
  }
}

void com_msg_notify(EVT_Event_t *evt) {
  if (COM_EVT_TYPE_MSG != evt->type) {
    return;
  }

  COM_Msg_Event_t *msg_evt = (COM_Msg_Event_t *)evt;

  if (COM_CHANNEL_CI != msg_evt->channel) {
    return;
  }

  printf("com_msg_notify: %d-%d len %ld\n", msg_evt->channel, msg_evt->seq_num, msg_evt->length);
  int16_t result = -127;
  if (2 == msg_evt->length) {
    result = *(int16_t *)msg_evt->data;
  }

  if (0 != CI_ack(&ci, msg_evt->seq_num, result, millis())) {
    printf("ERROR CI_ack: %d", ERR_CI_ACK_FAIL);
    return;
  }
}

void ci_ready_notify(EVT_Event_t *evt)
{
  if (CI_EVT_TYPE_READY != evt->type) {
    return;
  }

  CI_Ready_Event_t *ready = (CI_Ready_Event_t *)evt;

  uint8_t ci_msg[5];
  ci_msg[0] = ready->cmd->cmd;
  memcpy(&ci_msg[1], ready->cmd->data, 4);

  if (0 != COM_send(&com, COM_TYPE_REQ, COM_CHANNEL_CI, (uint8_t *)ci_msg, 5, millis())) {
    printf("ERROR COM_send: %d", ERR_COM_SEND);
    return;
  }
}

void ci_ack_notify(EVT_Event_t *evt)
{
  char buf[80];

  if (CI_EVT_TYPE_ACK != evt->type) {
      return;
  }

  if (0 == current_client) {
      return;
  }

  CI_Ack_Event_t *ack = (CI_Ack_Event_t *)evt;

  if (CI_R_OK == ack->cmd->result) {
    if (0 > send(current_client, "OK\n", 3, 0)) {
        perror("failed to send client response");
    }
  } else {
    if (0 > sprintf(buf, "ERR %d\n", ack->cmd->result)) {
        perror("failed to send client response");
        return;
    }
    if (0 > send(current_client, buf, strlen(buf), 0)) {
        perror("failed to send client response");
        return;
    }
  }
}

void to_notify(EVT_Event_t *evt) {
  if (COM_EVT_TYPE_MSG != evt->type) {
    return;
  }

  COM_Msg_Event_t *to_evt = (COM_Msg_Event_t *)evt;

  if (COM_CHANNEL_TO != to_evt->channel) {
    return;
  }

  TO_t to = {0};
  if (0 != TO_init(&to)) {
    printf("failed to init to\n");
    return;
  }

  int toN = TO_decode(&to, to_evt->data, to_evt->length);
  if (0 > toN) {
    printf("failed to decode to data: %d\n", toN);
    return;
  }

  // Nothing to do if we don't have a place to write the data
  if (NULL == to_file) {
    return;
  }

  fprintf(to_file, "NOW=%lu\t", millis());

  for (int i = 0; i < TO_MAX_PARAMS; i++) {
    if (0 == to.objects[i].param) {
      continue;
    }

    if (TO_PARAM_ERROR == to.objects[i].param)  {
      fprintf(to_file, "ERR=%d", to.objects[i].data);
    } else if (TO_PARAM_MILLIS == to.objects[i].param) {
      fprintf(to_file, "UP=%lu", (unsigned long)to.objects[i].data / 1000);
    } else if (TO_PARAM_LOOP == to.objects[i].param) {
      fprintf(to_file, "LOOP=%lu", (unsigned long)to.objects[i].data);
    } else if (TO_PARAM_COM_SEQ == to.objects[i].param) {
      fprintf(to_file, "COM=%d", to.objects[i].data);
    } else if (TO_PARAM_IMPACT == to.objects[i].param) {
      if (to.objects[i].data > 0) {
        fprintf(to_file, "IMPACT=1");
      } else {
        fprintf(to_file, "IMPACT=0");
      }
    } else if (TO_PARAM_MOTOR_A == to.objects[i].param) {
      fprintf(to_file, "MOTORA=%d", to.objects[i].data);
    } else if (TO_PARAM_MOTOR_B == to.objects[i].param) {
      fprintf(to_file, "MOTORB=%d", to.objects[i].data);
    } else if (TO_PARAM_RFM_RSSI == to.objects[i].param) {
      fprintf(to_file, "RSSI=%d", to.objects[i].data);
    } else {
      fprintf(to_file, "%d=0x%08x", to.objects[i].param, to.objects[i].data);
    }
    fprintf(to_file, "\t");
  }

  fprintf(to_file, "\n");
  fflush(to_file);

  return;
}

int main(int argc, char const *argv[])
{
    EVT_init(&evt);
    tmr.evt = &evt;
    COM_init(&com, &evt, &tmr);
    ci.evt = &evt;

    EVT_subscribe(&evt, &debugEvent);

    EVT_subscribe(&evt, COM_notify);
    EVT_subscribe(&evt, &rfm_notify);
    EVT_subscribe(&evt, &to_notify);
    EVT_subscribe(&evt, &com_msg_notify);
    EVT_subscribe(&evt, &ci_ready_notify);
    EVT_subscribe(&evt, &ci_ack_notify);

    if (0 != open_to_file()) {
      exit(1);
    }

    server = open_client_sock();
    if (0 > server) {
        exit(1);
    }

    radio = open_radio_sock();
    if (0 > radio) {
        exit(1);
    }

    if (0 != run_server()) {
        fprintf(stderr, "server failed\n");
        exit(1);
    }

    if (NULL != to_file) {
      fclose(to_file);
    }

    return 0;
}
