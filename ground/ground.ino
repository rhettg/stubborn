
#include <SPI.h>
#include <RH_RF69.h>

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

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial) { delay(1); } // wait until serial console is open, remove if not tethered to computer
  Serial.println("Booting Ground Station..");

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
    n = MAX_CMD;
    if (!rf69.recv((uint8_t *)recv, &n)) {
      Serial.println("failed to recv");
    }

    if (n > 0) {
      recv[n] = 0;
      Serial.print("<- ");
      Serial.println(recv);
    }
    Serial.print("RSSI: ");
    Serial.print(rf69.lastRssi());
    Serial.println(" dBM");

    //Serial.print("Temp: ");
    //Serial.print(rf69.temperatureRead());
    //Serial.println();
  }
}

  
