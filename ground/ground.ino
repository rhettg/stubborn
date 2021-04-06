
#include <SPI.h>
#include <RH_RF69.h>

extern "C" {
#include "evt.h"
#include "com.h"
#include "to.h"
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

#define ERR_UNKNOWN  1
#define ERR_CMD      2
#define ERR_RFM_INIT 5
#define ERR_RFM_FREQ 6
#define ERR_RFM_RECV 7
#define ERR_TO_SET   10
#define ERR_COM_SEND 20
#define ERR_COM_RECV 21

#define CI_CMD_NOOP  1
#define CI_CMD_CLEAR 2
#define CI_CMD_BOOM  3

typedef struct {
  uint8_t cmd;
  uint8_t data[4];
} Command_t;

Command_t cmd = {0};

#define MAX_COMMAND_LENGTH        25
char commandLine[MAX_COMMAND_LENGTH + 1];  // read commands into this buffer from Serial.
uint8_t commandLineChars = 0;              // number of characters read into buffer

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial) { delay(1); } // wait until serial console is open, remove if not tethered to computer
  Serial.println("Booting Ground Station..");

  EVT_init(&evt);
  com.evt = &evt;

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

  Serial.println("Ready");
  Serial.print("-> ");
}

bool running = false;

void loop() {
  uint8_t n = 0;

/*
  if (millis() - last_cmd > 5000) {
    last_cmd = millis(); 
    if (0 == serialNotify()) {
      Serial.println("[AUTO NOOP]");
      Serial.print("<- ");
      dispatchCommand("NOOP");
      Serial.print("-> ");
    }
  }
*/

  if (Serial.available() > 0) {
    if (0 != getCommandLineFromSerialPort()) {
      return;
    }

    if (0 != commandLine[0]) {
      Serial.print("<- ");
      dispatchCommand(commandLine);
    }
    Serial.print("-> ");
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
  if (0 == parseCmd(s, &cmd)) {
    // TODO: constant for command size
    if (0 != COM_send_ci(&com, cmd.cmd, ++cmd_num, cmd.data, 4)) {
      Serial.println("[ERR Sending]");
    } else {
      Serial.print("[SENT "); 
      Serial.print(cmd_num);
      Serial.println("]");
    }
  } else {
    Serial.print("[ERR Parsing '");
    Serial.print(s);
    Serial.println("']");
  }
}

void rfm_notify(EVT_Event_t *evt) {
  if (COM_EVT_TYPE_DATA != evt->type) {
    return;
  }

  COM_Data_Event_t *d_evt = (COM_Data_Event_t *)evt;
  /*
  Serial.print("Sending "); Serial.print(d_evt->length); Serial.print(" bytes");
  for(int i = 0; i < d_evt->length; i++) {
    Serial.print(d_evt->data[i], HEX); Serial.print(' ');
  }
  */
  rf69.send(d_evt->data, d_evt->length);
  rf69.waitPacketSent();
}

void to_notify(EVT_Event_t *evt) {
  if (COM_EVT_TYPE_TO != evt->type) {
    return;
  }

  // XXX remove
  return;

  COM_TO_Event_t *to_evt = (COM_TO_Event_t *)evt;
  TO_Object_t *obj = (TO_Object_t *)to_evt->data;

  Serial.print("-> ");

  size_t sizeRemaining = to_evt->length;
  
  while (sizeRemaining >= sizeof(TO_Object_t)) {
    Serial.print(obj->param, HEX);
    Serial.print(':');
    Serial.print(obj->data, HEX);
    Serial.print(' ');

    obj++;
    sizeRemaining = sizeRemaining - sizeof(TO_Object_t);
  }

  Serial.println();
}

#define CI_R_OK            0
#define CI_R_ERR_NOT_FOUND 1
#define CI_R_ERR_FAILED    2

void ci_r_notify(EVT_Event_t *evt) {
  if (COM_EVT_TYPE_CI_R != evt->type) {
    return;
  }

  COM_CI_R_Event_t *r_evt = (COM_CI_R_Event_t *)evt;
  if (0 != serialNotify()) {
    return;
  }

  Serial.print("[");
  Serial.print(r_evt->frame->cmd_num);
  if (CI_R_OK == r_evt->frame->result) {
    Serial.print(" OK");
  } 
  else {
    Serial.print(" ERR ");
    Serial.print(r_evt->frame->result);
  }
  Serial.println("]");

}

void debugEvent(EVT_Event_t *e)
{
  if (0 == serialNotify()) {
    Serial.print("Event: ");
    Serial.println(e->type);
  }
}

void Error(uint8_t errno) 
{
  if (0 == serialNotify()) {
    Serial.print("ERROR: ");
    Serial.println(errno);
  }
}

int parseCmd(char *s, Command_t *cmd) {
  uint8_t v1, v2;

  if (strncmp(s, "NOOP", 5) == 0) {
    cmd->cmd = CI_CMD_NOOP;
    return 0;
  } else if (strncmp(s, "CLEAR", 6) == 0) {
    cmd->cmd = CI_CMD_CLEAR;
    return 0;
  } else if (strncmp(s, "BOOM", 5) == 0) {
    cmd->cmd = CI_CMD_BOOM;
    return 0;
  } else if (strncmp(s, "FWD ", 4) == 0) {
    cmd->cmd = CI_CMD_NOOP;
    v1 = atoi(s+4);
    if (v1 > 0 && v1 < 256) {
      cmd->data[0] = v1;

      return 0;
    }
  } else if (strncmp(s, "BCK ", 4) == 0) {
    v1 = atoi(s+4);
    if (v1 > 0 && v1 < 256) {
      cmd->data[0] = v1;

      return 0;
    }
  /*
  } else if (strncmp(cmd_str, "RT ", 3) == 0) {
    v1 = atoi(cmd+3);
    if (v1 > 0 && v1 < 256) {
      motorASpeed = v1;
      motorBSpeed = -v1;

      return true;
    }
  } else if (strncmp(cmd, "LT ", 3) == 0) {
    v1 = atoi(cmd+3);
    if (v1 > 0 && v1 < 256) {
      motorASpeed = -v1;
      motorBSpeed = v1;

      return true;
    }
    */
  } else if (strncmp(s, "STOP ", 5) == 0) {
      cmd->data[0] = 0;

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