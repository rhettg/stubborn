
#include <SPI.h>
#include <RH_RF69.h>

extern "C" {
#include "to.h"
#include "com.h"
}

int enablePin[] = {10, 5};
int motorPin1[] = {9, 7};
int motorPin2[] = {8, 6};

void setMotor(int motor, int speed) {
  int out = 0;
  
  if (speed > 0) {
    digitalWrite(motorPin1[motor], HIGH);
    digitalWrite(motorPin2[motor], LOW);

    out = speed;
  } else {
    digitalWrite(motorPin1[motor], LOW);
    digitalWrite(motorPin2[motor], HIGH);

    out = -speed;
  }
  
  analogWrite(enablePin[motor], out);
}

char *cmd = NULL;
int motorASpeed = 0;
int motorBSpeed = 0;

#define MAX_CMD       64

#define RFM69_INT     3  // 
#define RFM69_CS      4  //
#define RFM69_RST     2
#define LED 13

#define RF69_FREQ 915.0

unsigned long lastSent = 0;

RH_RF69 rf69(RFM69_CS, RFM69_INT);
int16_t packetnum = 0;  // packet counter, we increment per xmission

uint8_t rf_buf[RH_RF69_MAX_MESSAGE_LEN];

unsigned long impact = 0;

struct TO to;

EVT_t evt;
COM_t com = {0};

void debugEvent(EVT_Event_t *e)
{
  Serial.print("Event: ");
  Serial.println(e->type);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial) { delay(1); } // wait until serial console is open, remove if not tethered to computer
  Serial.println("Booting..");

  EVT_init(&evt);
  com.evt = &evt;

  EVT_subscribe(&evt, &debugEvent);

  int r = TO_init(&to, 16);
  if (r != 0) {
    Serial.println("failed to initialize TO");
    abort();
  }

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

  for (int motor = 0; motor < 2; motor++) {
    pinMode(enablePin[motor], OUTPUT);
    pinMode(motorPin1[motor], OUTPUT);
    pinMode(motorPin2[motor], OUTPUT);
  }

  cmd = (char*)malloc(MAX_CMD);

  setMotor(0, 0);
  setMotor(1, 0);

  Serial.println("HELLO");
}

#define TO_PARAM_MILLIS 1

#define MAX_TO_SIZE 60
uint8_t to_buf[MAX_TO_SIZE];
size_t to_size;

void rfm_notify(EVT_Event_t *evt) {
  if (COM_EVT_TYPE_DATA != evt->type) {
    return;
  }

  COM_Data_Event_t *d_evt = (COM_Data_Event_t *)evt;
  rf69.send(d_evt->data, d_evt->length);
  rf69.waitPacketSent();
}

void loop() {
  if (0 > TO_set(&to, TO_PARAM_MILLIS, millis())) {
    Serial.println("ERROR: failed setting millis");
  }

  if (millis() - lastSent > 1000) {
    size_t to_size = TO_encode(&to, to_buf, sizeof(to_buf));
    if (0 < to_size) {
      if (0 > COM_send_to(&com, to_buf, to_size)) {
        Serial.println("Error: COM_send_to failed");
      }
    }

    Serial.println("PING");
    strncpy(cmd, "PING", 4);
    cmd[4] = 0;
    
    rf69.send((uint8_t *)cmd, strlen(cmd));
    rf69.waitPacketSent();
    lastSent = millis(); 
  }

  uint8_t n = 0;
  
  if (Serial.available() > 0) {
    n = Serial.readBytesUntil('\n', cmd, MAX_CMD);
    while (Serial.available() > 0) {
      Serial.read();
    }
  }

  if (rf69.available()) {  
    n = sizeof(rf_buf);

    if (!rf69.recv(rf_buf, &n)) {
      Serial.println("failed to recv");
    }

    COM_recv(&com, rf_buf, n);
  }

  if (n > 0) {
      cmd[n] = 0; // for extra safety
      Serial.print("-> ");
      Serial.println(cmd);
      
      Serial.print("<- ");      
      if (parseCmd(cmd)) {
        Serial.println("OK");
        Blink(LED, 40, 3); //blink LED 3 times, 40ms between blinks
      } else {
        Serial.println("FAIL - failed to parse");
        Blink(LED, 100, 5);
      }
  }

  int iv = analogRead(0);
  
  if (iv < 512 && impact == 0) {
    impact = millis();
    Serial.println("IMPACT");
    strncpy(cmd, "IMPACT", 6);
    cmd[6] = 0;
    
    rf69.send((uint8_t *)cmd, strlen(cmd));
    rf69.waitPacketSent();

    motorASpeed = 0;
    motorBSpeed = 0;
  } else if (iv > 512 && millis() - impact > 500) {
    impact = 0;
  }

  setMotor(0, motorASpeed);
  setMotor(1, motorBSpeed);
}

bool parseCmd(char *cmd) {
  int v1, v2;

  if (strncmp(cmd, "FWD ", 4) == 0) {
    v1 = atoi(cmd+4);
    if (v1 > 0 && v1 < 256) {
      motorASpeed = v1;
      motorBSpeed = v1;

      return true;
    }
  } else if (strncmp(cmd, "BCK ", 4) == 0) {
    v1 = atoi(cmd+4);
    if (v1 > 0 && v1 < 256) {
      motorASpeed = -v1;
      motorBSpeed = -v1;

      return true;
    }
  } else if (strncmp(cmd, "RT ", 3) == 0) {
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
  } else if (strncmp(cmd, "STOP", 4) == 0) {
      motorASpeed = 0;
      motorBSpeed = 0;

      return true;
  }

  return false;
}

void Blink(byte PIN, byte DELAY_MS, byte loops) {
  for (byte i=0; i<loops; i++)  {
    digitalWrite(PIN,HIGH);
    delay(DELAY_MS);
    digitalWrite(PIN,LOW);
    delay(DELAY_MS);
  }
}

int rfm_receive(void)
{
}
