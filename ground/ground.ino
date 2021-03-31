
#include <SPI.h>
#include <RH_RF69.h>

extern "C" {
#include "evt.h"
#include "com.h"
#include "to.h"
};

char *cmd = NULL;
char *recv = NULL;

#define MAX_CMD       64

#define RFM69_INT     3  // 
#define RFM69_CS      4  //
#define RFM69_RST     2
#define LED 13

#define RF69_FREQ 915.0

unsigned long lastCmd = 0;

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

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial) { delay(1); } // wait until serial console is open, remove if not tethered to computer
  Serial.println("Booting Ground Station..");

  EVT_init(&evt);
  com.evt = &evt;

  EVT_subscribe(&evt, &debugEvent);

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

  cmd = (char*)malloc(MAX_CMD);
  recv = (char*)malloc(MAX_CMD);

  Serial.println("Ready");
}

bool running = false;

void loop() {
  uint8_t n = 0;

  /*
  if (millis() - lastCmd > 5000) {
    lastCmd = millis(); 
    if (running) {
      n = 4;
      strncpy(cmd, "STOP", n);
    } else {
      n = 7;
      strncpy(cmd, "FWD 255", n);  
    }
    running = !running;
  }
  */
  
  if (Serial.available() > 0) {
    n = Serial.readBytesUntil('\n', cmd, MAX_CMD);
    while (Serial.available() > 0) {
      Serial.read();
    }
    
    lastCmd = millis();
  }

  if (n > 0) {
    cmd[n] = 0;
    Serial.print("-> ");
    Serial.print(cmd);
    
    rf69.send((uint8_t *)cmd, strlen(cmd));
    rf69.waitPacketSent();
    Serial.println(" [SENT]");
  }

  if (rf69.available()) {  
    n = sizeof(rf_buf);

    if (!rf69.recv(rf_buf, &n)) {
      Error(ERR_RFM_RECV);
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
    }

    Serial.print("RSSI: ");
    Serial.print(rf69.lastRssi());
    Serial.println(" dBM");
  }
}

void rfm_notify(EVT_Event_t *evt) {
  if (COM_EVT_TYPE_DATA != evt->type) {
    return;
  }

  COM_Data_Event_t *d_evt = (COM_Data_Event_t *)evt;
  rf69.send(d_evt->data, d_evt->length);
  rf69.waitPacketSent();
}

void to_notify(EVT_Event_t *evt) {
  if (COM_EVT_TYPE_TO != evt->type) {
    return;
  }

  COM_TO_Event_t *to_evt = (COM_TO_Event_t *)evt;

  size_t sizeRemaining = to_evt->length;
  TO_Object_t *obj = (TO_Object_t *)to_evt->data;
  while (sizeRemaining <= sizeof(TO_Object_t)) {
    Serial.print(obj->param, HEX);
    Serial.print(':');
    Serial.print(obj->data, HEX);
    Serial.print(' ');

    obj = obj+sizeof(TO_Object_t);
    sizeRemaining = sizeRemaining - sizeof(TO_Object_t);
  }

  Serial.println();
}

void debugEvent(EVT_Event_t *e)
{
  Serial.print("Event: ");
  Serial.println(e->type);
}

void Error(uint8_t errno) 
{
  if (Serial.availableForWrite()) {
    Serial.print("ERROR: ");
    Serial.println(errno);
  }
}
