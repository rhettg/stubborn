
#include <SPI.h>
#include <RH_RF69.h>

extern "C" {
#include "stubborn.h"
#include "evt.h"
#include "com.h"
#include "to.h"
#include "ci.h"
};

int cmd_num = 0;
unsigned long last_cmd = 0;

#define RFM69_INT     3  //
#define RFM69_CS      4  //
#define RFM69_RST     2
#define LED 13

#define RF69_FREQ 915.0

RH_RF69 rf69(RFM69_CS, RFM69_INT);
int16_t packetnum = 0;  // packet counter, we increment per xmission

uint8_t rf_buf[RH_RF69_MAX_MESSAGE_LEN];

EVT_t evt = {0};
COM_t com = {0};
CI_t  ci  = {0};

#define ERR_UNKNOWN  1
#define ERR_CMD      2

#define ERR_RFM_INIT 5
#define ERR_RFM_FREQ 6
#define ERR_RFM_RECV 7

#define ERR_TO_SET   10

#define ERR_COM_SEND 20
#define ERR_COM_RECV 21

#define ERR_CI_SEND_FAIL 30
#define ERR_CI_ACK_FAIL  31

#define MAX_COMMAND_LENGTH        25
char commandLine[MAX_COMMAND_LENGTH + 1];  // read commands into this buffer from Serial.
uint8_t commandLineChars = 0;              // number of characters read into buffer
//bool prompt = false;

#define DISPLAY_MODE_NONE   0
#define DISPLAY_MODE_PROMPT 1
#define DISPLAY_MODE_RESULT 2
#define DISPLAY_MODE_TO     5
int display_mode = DISPLAY_MODE_NONE;

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial) { delay(1); } // wait until serial console is open, remove if not tethered to computer
  Serial.println("Booting Ground Station..");

  EVT_init(&evt);
  com.evt = &evt;
  ci.evt = &evt;

  // EVT_subscribe(&evt, &debugEvent);

  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

  // manual reset
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);

  if (!rf69.init()) {
    Serial.println("RFM69 radio init failed");
    while (1);
  }
  Serial.println("RFM69 radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for low power module)
  // No encryption
  if (!rf69.setFrequency(RF69_FREQ)) {
    Serial.println("setFrequency failed");
  }

  // If you are using a high power RF69 eg RFM69HW, you *must* set a Tx power with the
  // ishighpowermodule flag set like this:
  rf69.setTxPower(20, true);  // range from 14-20 for power, 2nd arg must be true for 69HCW

  Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");

  EVT_subscribe(&evt, &rfm_notify);
  EVT_subscribe(&evt, &to_notify);
  EVT_subscribe(&evt, &ci_r_notify);
  EVT_subscribe(&evt, &ci_ready_notify);
  EVT_subscribe(&evt, &ci_ack_notify);

  Serial.println("Ready");
}

void do_prompt()
{
  Serial.println();
  Serial.print("-> ");
  display_mode = DISPLAY_MODE_PROMPT;
}

void do_result()
{
  Serial.print("<- ");
  display_mode = DISPLAY_MODE_RESULT;
}

void display_none()
{
  display_mode = DISPLAY_MODE_NONE;
}

int fwd = 0;

void loop()
{
  uint8_t n = 0;

  if (DISPLAY_MODE_NONE == display_mode) {
    do_prompt();
  }

#ifdef AUTOCMD
  if (millis() - last_cmd > 5000) {
    last_cmd = millis();
    if (0 == serialNotify()) {
      Serial.println("[AUTO FWD]");
      Serial.print("<- ");
      if (fwd) {
        dispatchCommand("FWD 150");
      } else {
        dispatchCommand("STOP");
      }
      fwd = !fwd;
      Serial.print("-> ");
    }
  }
#endif

  if (Serial.available() > 0) {
    if (0 != getCommandLineFromSerialPort()) {
      return;
    }

    if (0 == commandLine[0]) {
      display_none();
    } else {
      dispatchCommand(commandLine);
    }
  }

  if (rf69.available()) {
    n = sizeof(rf_buf);

    if (!rf69.recv(rf_buf, &n)) {
      Error(ERR_RFM_RECV);
      return;
    }

    int cret = COM_recv(&com, rf_buf, n);
    if (cret != 0) {
      Serial.print("return value: ");
      Serial.println(cret);
      for (int ndx=0; ndx<n; ndx++) {
        Serial.print(rf_buf[ndx], HEX);
        Serial.print(' ');
      }
      Error(ERR_COM_RECV);
      return;
    }

/*
    Serial.print("RSSI: ");
    Serial.print(rf69.lastRssi());
    Serial.print(" dBM");
    Serial.print(" recv: ");
    Serial.print(n);
    Serial.println();
    */
  }
}

void dispatchCommand(char *s)
{
  do_result();

  int r = parseCICommand(s);

  if (0 == r) {
    Serial.print("[SENT ");
    Serial.print(ci.current.cmd_num);
    Serial.println("]");
    do_result();
    return;
  }

  if (ERR_UNKNOWN == r) {
    int r = parseConsoleCommand(s);
    if (0 == r) {
      return;
    }

    Serial.print("[ERR Parsing '");
    Serial.print(s);
    Serial.println("']");
  } else {
    Serial.print("[ERR issuing command '");
    Serial.print(s);
    Serial.print("': ");
    Serial.print(r);
    Serial.println("]");
  }

  display_none();
}

void rfm_notify(EVT_Event_t *evt) {
  if (COM_EVT_TYPE_DATA != evt->type) {
    return;
  }

  COM_Data_Event_t *d_evt = (COM_Data_Event_t *)evt;
#ifdef DEBUG
  Serial.print("Sending "); Serial.print(d_evt->length); Serial.print(" bytes");
  for(int i = 0; i < d_evt->length; i++) {
    Serial.print(d_evt->data[i], HEX); Serial.print(' ');
  }
#endif
  rf69.send(d_evt->data, d_evt->length);
  rf69.waitPacketSent();
}

void to_notify(EVT_Event_t *evt) {
  if (COM_EVT_TYPE_TO != evt->type) {
    return;
  }

  if (DISPLAY_MODE_TO != display_mode) {
    return;
  }

  COM_TO_Event_t *to_evt = (COM_TO_Event_t *)evt;
  TO_Object_t *obj = (TO_Object_t *)to_evt->data;

  Serial.print("\n\r\t");

  size_t sizeRemaining = to_evt->length;

  int motorVelocity;

  while (sizeRemaining >= sizeof(TO_Object_t)) {
    if (TO_PARAM_ERROR == obj->param)  {
      Serial.print("ERR:");
      Serial.print(obj->data);
    } else if (TO_PARAM_MILLIS == obj->param) {
      Serial.print("UP:");
      Serial.print((unsigned long)obj->data / 1000);
      Serial.print("s");
    } else if (TO_PARAM_LOOP == obj->param) {
      Serial.print("LOOP:");
      Serial.print(obj->data);
      Serial.print("ms");
    } else if (TO_PARAM_COM_SEQ == obj->param) {
      Serial.print("COM:");
      Serial.print(obj->data);
    } else if (TO_PARAM_IMPACT == obj->param) {
      if (obj->data > 0) {
        Serial.print("IMPACT");
      }
    } else if (TO_PARAM_MOTOR_A == obj->param) {
      Serial.print("M.A:");
      motorVelocity = (int)obj->data;
      Serial.print(motorVelocity);
    } else if (TO_PARAM_MOTOR_B == obj->param) {
      Serial.print("M.B:");
      motorVelocity = (int)obj->data;
      Serial.print(motorVelocity);
    } else if (TO_PARAM_RFM_RSSI == obj->param) {
      Serial.print("RSSI:");
      Serial.print((int)obj->data);
    } else {
      Serial.print(obj->param, HEX);
      Serial.print(':');
      Serial.print(obj->data, HEX);
    }
    Serial.print(' ');

    obj++;
    sizeRemaining = sizeRemaining - sizeof(TO_Object_t);
  }
}

void ci_r_notify(EVT_Event_t *evt) {
  if (COM_EVT_TYPE_CI_R != evt->type) {
    return;
  }

  COM_CI_R_Event_t *r_evt = (COM_CI_R_Event_t *)evt;
  if (0 != CI_ack(&ci, r_evt->frame->cmd_num, r_evt->frame->result, millis())) {
    Error(ERR_CI_ACK_FAIL);
    return;
  }
}

void ci_ready_notify(EVT_Event_t *evt)
{
  if (CI_EVT_TYPE_READY != evt->type) {
    return;
  }

  CI_Ready_Event_t *ready = (CI_Ready_Event_t *)evt;

  if (0 != COM_send_ci(&com, ready->cmd->cmd, ready->cmd->cmd_num, ready->cmd->data, sizeof(ready->cmd->data))) {
    Error(ERR_COM_SEND);
    return;
  }
}

void ci_ack_notify(EVT_Event_t *evt)
{
  if (CI_EVT_TYPE_ACK != evt->type) {
    return;
  }

  CI_Ack_Event_t *ack = (CI_Ack_Event_t *)evt;

  if (DISPLAY_MODE_RESULT != display_mode) {
    return;
  }

  Serial.print("[");
  Serial.print(ack->cmd->cmd_num);
  if (CI_R_OK == ack->cmd->result) {
    Serial.print(" OK");
  } else {
    Serial.print(" ERR ");
    Serial.print(ack->cmd->result);
  }
  Serial.print("]");
  display_none();
}

void debugEvent(EVT_Event_t *e)
{
  if (0 == serialNotify()) {
    Serial.print("Event: ");
    Serial.print(e->type);
  }
}

void Error(uint8_t errno)
{
  if (0 == serialNotify()) {
    Serial.print("ERROR: ");
    Serial.print(errno);
  }
}

int parseCICommand(char *s) {
  uint8_t cmd = 0;
  uint8_t cmd_data[CI_MAX_DATA] = {0,0,0,0};

  uint8_t v1, v2;

  if (strncmp(s, "NOOP", 5) == 0) {
    cmd = CI_CMD_NOOP;
  } else if (strncmp(s, "CLEAR", 6) == 0) {
    cmd = CI_CMD_CLEAR;
  } else if (strncmp(s, "BOOM", 5) == 0) {
    cmd = CI_CMD_BOOM;
  } else if (strncmp(s, "FWD ", 4) == 0) {
    cmd = CI_CMD_EXT_FWD;
    v1 = atoi(s+4);
    if (v1 > 0 && v1 < 256) {
      cmd_data[0] = v1;
    }
  } else if (strncmp(s, "BCK ", 4) == 0) {
    cmd = CI_CMD_EXT_BCK;
    v1 = atoi(s+4);
    if (v1 > 0 && v1 < 256) {
      cmd_data[0] = v1;
    }
  } else if (strncmp(s, "RT ", 3) == 0) {
    cmd = CI_CMD_EXT_RT;
    v1 = atoi(s+3);
    if (v1 > 0 && v1 < 256) {
      cmd_data[0] = v1;
    }
  } else if (strncmp(s, "LT ", 3) == 0) {
    cmd = CI_CMD_EXT_LT;
    v1 = atoi(s+3);
    if (v1 > 0 && v1 < 256) {
      cmd_data[0] = v1;
    }
  } else if (strncmp(s, "STOP", 4) == 0) {
      cmd_data[0] = 0;
      cmd = CI_CMD_EXT_STOP;
  }

  if (0 == cmd) {
    return ERR_UNKNOWN;
  }

  if (0 != CI_prepare_send(&ci, cmd, cmd_data, millis())) {
    return ERR_CI_SEND_FAIL;
  }

  return 0;
}

int parseConsoleCommand(char *s)
{
  if (strncmp(s, "TO", 2) == 0) {
    display_mode = DISPLAY_MODE_TO;
    return 0;
  }

  if (strncmp(s, "RESEND", 5) == 0) {
    display_mode = DISPLAY_MODE_TO;
    COM_send_retry(&com);
    Serial.print("[SENT ");
    Serial.print(ci.current.cmd_num);
    Serial.println("]");
    do_result();
    return 0;
  }

  return ERR_UNKNOWN;
}

// returns if it's appropriate to notify user on the console
int serialNotify()
{
  if (!Serial.availableForWrite()) {
    return -1;
  }

  if (0 != commandLineChars) {
    return -1;
  }

  display_mode = DISPLAY_MODE_NONE;
  return 0;
}

#define CR '\r'
#define LF '\n'
#define BS '\b'
#define NULLCHAR '\0'
#define SPACE ' '

/*************************************************************************************************************
    getCommandLineFromSerialPort()
      Return the string of the next command. Commands are delimited by return"
      Handle BackSpace character
      Make all chars lowercase

    Adapted from https://create.arduino.cc/projecthub/mikefarr/simple-command-line-interface-4f0a3f
*************************************************************************************************************/

int
getCommandLineFromSerialPort()
{
  while (Serial.available()) {
    char c = Serial.read();

    switch (c) {
      case CR:      //likely have full command in buffer now, commands are terminated by CR and/or LS
      case LF:
        Serial.println();
        commandLine[commandLineChars] = NULLCHAR;
        commandLineChars = 0;
        return 0;
        break;
      case BS:
        if (commandLineChars > 0) {
          commandLine[--commandLineChars] = NULLCHAR;

          Serial.print(BS); Serial.print(SPACE); Serial.print(BS);
        }
        break;
      default:
        Serial.print(c);
        if (commandLineChars < MAX_COMMAND_LENGTH) {
          commandLine[commandLineChars++] = c;
        }
        commandLine[commandLineChars] = NULLCHAR;
        break;
    }
  }
  return -1;
}