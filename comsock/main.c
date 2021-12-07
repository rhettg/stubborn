#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>
#include <math.h>

#include "stubborn.h"
#include "evt.h"
#include "com.h"
#include "ci.h"
#include "to.h"

#include "command.h"

#define TO_FILE      "/var/stubborn/to"
#define TO_JSON_FILE "/var/stubborn/to.json"
#define SOCK_FILE    "/var/stubborn/comsock"
#define RADIO_FILE   "/var/stubborn/radio"
#define CAM_DATA_FILE "/var/stubborn/cam.jpg"

EVT_t evt = {0};
TMR_t tmr = {0};
COM_t com = {0};
CI_t  ci  = {0};

int current_client = 0;
int radio = 0;
int server = 0;

unsigned long start_time = 0;

FILE *to_file = {0};
FILE *to_json_file = {0};
int cam_file = 0;

void log_format(const char* tag, const char* message, va_list args) {   time_t now;     time(&now);     char * date =ctime(&now);   date[strlen(date) - 1] = '\0';  printf("%s [%s] ", date, tag);  vprintf(message, args);     printf("\n"); }

void log_error(const char* message, ...) {  va_list args;   va_start(args, message);    log_format("error", message, args);     va_end(args); }

void log_info(const char* message, ...) {   va_list args;   va_start(args, message);    log_format("info", message, args);  va_end(args); }

void log_debug(const char* message, ...) {  va_list args;   va_start(args, message);    log_format("debug", message, args);     va_end(args); }

void fatal(const char *msg)
{
    log_error(msg);
    exit(1);
}

void debugEvent(EVT_Event_t *e)
{
    log_debug("event: %d", e->type);
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

int open_to_json_file()
{
  to_json_file = fopen(TO_JSON_FILE, "a");
  if (NULL == to_json_file) {
    perror("failed to open /var/stubborn/to.json");
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

    // Insecure? Maybe but this makes development easier.
    if (0 != chmod(SOCK_FILE, 0777)) {
      perror("failed to chmod sockfile");
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

    log_debug("starting server");

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
            log_debug("radio: reading");
            rlen = read(radio, buf, sizeof(buf));
            if (rlen < 0 && EAGAIN != errno) {
                log_error("failed reading: %d", errno);
                rlen = 0;
            }

            if (0 == rlen) {
              log_error("radio: closed, exiting");
              close(radio);
              radio = 0;
              return 0;
            }

            log_debug("radio: read %ld bytes", rlen);

            int cret = COM_recv(&com, (uint8_t *)buf, rlen, millis());
            if (cret != 0) {
              log_error("error from COM: %d", cret);
              continue;
            }
        }

        if (0 != FD_ISSET(server, &set)) {
            log_info("client: accepting");
            current_client = accept(server, 0, 0);
            if (current_client < 0) {
                log_error("failed to accept: %d", errno);
                continue;
            }
        }

        if (0 != FD_ISSET(current_client, &set)) {
            log_debug("client: reading");
            rlen = read(current_client, buf, sizeof(buf));
            if (rlen < 0 && EAGAIN != errno) {
                log_error("failed reading: %d", errno);
                rlen = 0;
            }

            if (0 < rlen) {
                buf[rlen] = 0;
                log_info("client: RX '%s'", buf);
                int rp = parse_ci_command(&ci, buf);
                if (0 != rp) {
                    log_error("client: failed parsing command: %d", rp);
                    if (0 > send(current_client, "ERR\n", 4, 0)) {
                        perror("failed to report error");
                    }
                }
            } else {
                log_info("client: closing");
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
  log_debug("radio: TX: (%lu len)", d_evt->length);
#ifdef DEBUG
  printf("\t");
  for(int i = 0; i < d_evt->length; i++) {
    printf("0x%x ", d_evt->data[i]);
  }
  printf("\n");
#endif

  log_debug("radio: writing");
  int n = write(radio, d_evt->data, d_evt->length);
  if (0 > n) {
    perror("failed to write");
  } else if (n < d_evt->length) {
    log_error("radio: short write: %d %ld\n", n , d_evt->length);
  }
  log_debug("radio: wrote %ul bytes", n);
}

void com_msg_notify(EVT_Event_t *evt) {
  if (COM_EVT_TYPE_MSG != evt->type) {
    return;
  }

  COM_Msg_Event_t *msg_evt = (COM_Msg_Event_t *)evt;

  if (COM_CHANNEL_CI != msg_evt->channel) {
    return;
  }

  log_debug("com_msg_notify: %d-%d len %ld", msg_evt->channel, msg_evt->seq_num, msg_evt->length);
  int16_t result = -127;
  if (2 == msg_evt->length) {
    result = *(int16_t *)msg_evt->data;
  }

  if (0 != CI_ack(&ci, msg_evt->seq_num, result, millis())) {
    log_error("ERROR CI_ack: %d", ERR_CI_ACK_FAIL);
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
    log_error("failed to COM_send: %d", ERR_COM_SEND);
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

void write_to_file(TO_t *to)
{
  // Nothing to do if we don't have a place to write the data
  if (NULL == to_file) {
    return;
  }

  fprintf(to_file, "NOW=%lu\t", millis());

  for (int i = 0; i < TO_MAX_PARAMS; i++) {
    if (0 == to->objects[i].param) {
      continue;
    }

    if (TO_PARAM_ERROR == to->objects[i].param)  {
      fprintf(to_file, "ERR=%d", to->objects[i].data);
    } else if (TO_PARAM_MILLIS == to->objects[i].param) {
      fprintf(to_file, "UP=%lu", (unsigned long)to->objects[i].data / 1000);
    } else if (TO_PARAM_LOOP == to->objects[i].param) {
      fprintf(to_file, "LOOP=%lu", (unsigned long)to->objects[i].data);
    } else if (TO_PARAM_COM_SEQ == to->objects[i].param) {
      fprintf(to_file, "COM=%d", to->objects[i].data);
    } else if (TO_PARAM_IMPACT == to->objects[i].param) {
      if (to->objects[i].data > 0) {
        fprintf(to_file, "IMPACT=1");
      } else {
        fprintf(to_file, "IMPACT=0");
      }
    } else if (TO_PARAM_MOTOR_A == to->objects[i].param) {
      fprintf(to_file, "MOTORA=%d", to->objects[i].data);
    } else if (TO_PARAM_MOTOR_B == to->objects[i].param) {
      fprintf(to_file, "MOTORB=%d", to->objects[i].data);
    } else if (TO_PARAM_RFM_RSSI == to->objects[i].param) {
      fprintf(to_file, "RSSI=%d", to->objects[i].data);
    } else if (TO_PARAM_CAM_LEN == to->objects[i].param) {
      fprintf(to_file, "CAM_LEN=%lu", (unsigned long)to->objects[i].data);
    } else {
      fprintf(to_file, "%d=0x%08x", to->objects[i].param, to->objects[i].data);
    }
    fprintf(to_file, "\t");
  }

  fprintf(to_file, "\n");
  fflush(to_file);
}

void json_field(FILE *f, const char *key, char *value)
{
  fprintf(f, "\"%s\": \"%s\"", key, value);
}

void json_field_int(FILE *f, const char *key, long int value)
{
  fprintf(f, "\"%s\": %ld", key, value);
}

void write_to_json_file(TO_t *to)
{
  // Nothing to do if we don't have a place to write the data
  if (NULL == to_json_file) {
    return;
  }

  char param_buf[255];
  char value_buf[255];

  fprintf(to_json_file, "{");

  json_field_int(to_json_file, "NOW", time(NULL));

  for (int i = 0; i < TO_MAX_PARAMS; i++) {
    if (0 == to->objects[i].param) {
      continue;
    }

    fprintf(to_json_file, ", ");

    if (TO_PARAM_ERROR == to->objects[i].param)  {
      json_field_int(to_json_file, "ERR", to->objects[i].data);
    } else if (TO_PARAM_MILLIS == to->objects[i].param) {
      json_field_int(to_json_file, "UP", to->objects[i].data / 1000);
    } else if (TO_PARAM_LOOP == to->objects[i].param) {
      json_field_int(to_json_file, "LOOP", to->objects[i].data);
    } else if (TO_PARAM_COM_SEQ == to->objects[i].param) {
      json_field_int(to_json_file, "COM", to->objects[i].data);
    } else if (TO_PARAM_IMPACT == to->objects[i].param) {
      if (to->objects[i].data > 0) {
        json_field_int(to_json_file, "IMPACT", 1);
      } else {
        json_field_int(to_json_file, "IMPACT", 0);
      }
    } else if (TO_PARAM_MOTOR_A == to->objects[i].param) {
      json_field_int(to_json_file, "MOTORA", (int)to->objects[i].data);
    } else if (TO_PARAM_MOTOR_B == to->objects[i].param) {
      json_field_int(to_json_file, "MOTORB", (int)to->objects[i].data);
    } else if (TO_PARAM_RFM_RSSI == to->objects[i].param) {
      json_field_int(to_json_file, "RSSI", to->objects[i].data);
    } else if (TO_PARAM_CAM_LEN == to->objects[i].param) {
      json_field_int(to_json_file, "CAM_LEN", to->objects[i].data);
    } else {
      sprintf(param_buf, "%d", to->objects[i].param);
      sprintf(value_buf, "0x%08x", to->objects[i].data);
      json_field(to_json_file, param_buf, value_buf);
    }
  }

  fprintf(to_json_file, "}\n");
  fflush(to_json_file);
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

  write_to_file(&to);
  write_to_json_file(&to);

  return;
}

void cam_data_notify(EVT_Event_t *evt) {
  static int count;
  static int seq_num;

  if (COM_EVT_TYPE_MSG != evt->type) {
    return;
  }

  COM_Msg_Event_t *msg_evt = (COM_Msg_Event_t *)evt;

  if (COM_CHANNEL_CAM_DATA != msg_evt->channel) {
    return;
  }

  // Beginning of jpg?
  if (msg_evt->length > 2 && msg_evt->data[0] == 0xFF && msg_evt->data[1] == 0xD8) {
    if (0 != cam_file) close(cam_file);
    cam_file = 0;
  }

  if (0 == cam_file) {
    count = 0;
    seq_num = 0;
    log_debug("opening %s", CAM_DATA_FILE);
    cam_file = open(CAM_DATA_FILE, O_WRONLY|O_TRUNC|O_CREAT, 0666);
    if (0 == cam_file) {
      perror("failed to open /var/stubborn/cam.jpb");
      return;
    }
  }

  count++;

  if (msg_evt->seq_num > seq_num) {
    size_t wb = write(cam_file, msg_evt->data, msg_evt->length);
    log_debug("seq:%d count:%d - wrote %lu bytes to cam.jpg", msg_evt->seq_num, count, wb);
    seq_num = msg_evt->seq_num;
  } else {
    log_debug("seq:%d count:%d duplicate packet for cam.jpg", msg_evt->seq_num, count);
  }

  // End of jpg?
  if (msg_evt->length > 2 && msg_evt->data[msg_evt->length - 2] == 0xFF && msg_evt->data[msg_evt->length - 1] == 0xD9) {
    log_debug("closing cam file");
    close(cam_file);
    cam_file = 0;
  }

  uint8_t r = 0;
  if (0 != COM_send_reply(&com, msg_evt->channel, msg_evt->seq_num, (uint8_t *)&r, sizeof(r), millis())) {
    printf("error sending reply\n");
  }

  return;
}


int main(int argc, char const *argv[])
{
    start_time = millis();

    EVT_init(&evt);
    tmr.evt = &evt;
    COM_init(&com, &evt, &tmr);
    ci.evt = &evt;

    EVT_subscribe(&evt, &debugEvent);

    EVT_subscribe(&evt, &COM_notify);
    EVT_subscribe(&evt, &rfm_notify);
    EVT_subscribe(&evt, &to_notify);
    EVT_subscribe(&evt, &com_msg_notify);
    EVT_subscribe(&evt, &cam_data_notify);
    EVT_subscribe(&evt, &ci_ready_notify);
    EVT_subscribe(&evt, &ci_ack_notify);

    if (0 != open_to_file()) {
      exit(1);
    }

    if (0 != open_to_json_file()) {
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

    if (NULL != to_json_file) {
      fclose(to_json_file);
    }

    return 0;
}
