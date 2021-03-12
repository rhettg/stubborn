
#include <SPI.h>
#include <RH_RF69.h>

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

// int lastPing = 0;

RH_RF69 rf69(RFM69_CS, RFM69_INT);
int16_t packetnum = 0;  // packet counter, we increment per xmission

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  //while (!Serial) { delay(1); } // wait until serial console is open, remove if not tethered to computer
  Serial.println("Booting..");

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

void loop() {
  //if (millis() - lastPing > 1000) {
    //Serial.println("PONG");
    //lastPing = millis(); 
  //}

  uint8_t n = 0;
  
  if (Serial.available() > 0) {
    n = Serial.readBytesUntil('\n', cmd, MAX_CMD);
    while (Serial.available() > 0) {
      Serial.read();
    }
  }

  if (rf69.available()) {  
    n = MAX_CMD;
    if (!rf69.recv((uint8_t *)cmd, &n)) {
      Serial.println("failed to recv");
    }
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
