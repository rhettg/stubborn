
#include <SPI.h>
#include <RH_RF69.h>

extern "C" {
#include "to.h"
#include "com.h"
#include "ci.h"
#include "tmr.h"
#include "stubborn.h"
}

uint8_t enablePin[] = {10, 5};
uint8_t motorPin1[] = {9, 7};
uint8_t motorPin2[] = {8, 6};

#define RFM69_INT     3  // 
#define RFM69_CS      4  //
#define RFM69_RST     2
#define LED 13

#define RF69_FREQ 915.0

int motorASpeed = 0;
int motorBSpeed = 0;

#define MAX_CMD       64

#define MAX_TO_SIZE   60

// Mission specific event types will start at 64. CiC events fit under that.
#define EVT_TYPE_SYNC_TO 64

// syncTOEvent is triggered to prepare and send a TO (telemetry output) packet.
// There is only one as this is not something that will be used in more places.
struct {
  EVT_Event_t event;
} syncTOEvent;

unsigned long lastTOSync = 0;

#define ERR_UNKNOWN  1
#define ERR_CMD      2
#define ERR_RFM_INIT 5
#define ERR_RFM_FREQ 6
#define ERR_RFM_RECV 7
#define ERR_TO_SET   10
#define ERR_COM_SEND 20
#define ERR_CI_REGISTER 30
#define ERR_TMR_ENQUEUE 40

RH_RF69 rf69(RFM69_CS, RFM69_INT);

uint8_t rf_buf[RH_RF69_MAX_MESSAGE_LEN];

unsigned long impactStart = 0;

TO_t to   = {0};
EVT_t evt = {0};
TMR_t tmr = {0};
COM_t com = {0};
CI_t ci = {0};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial) { delay(1); } // wait until serial console is open, remove if not tethered to computer
  Serial.println("Booting..");

  EVT_init(&evt);
  com.evt = &evt;
  ci.evt = &evt;
  tmr.evt = &evt;

  // Is there some way I could do this as part of init?
  syncTOEvent.event.type = EVT_TYPE_SYNC_TO;

  EVT_subscribe(&evt, &debugEvent);

  if (0 != TO_init(&to)) {
    Serial.println("FAIL: TO init");
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
    Error(ERR_RFM_INIT);
    while (1);
  }
  
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for low power module)
  // No encryption
  if (!rf69.setFrequency(RF69_FREQ)) {
    Error(ERR_RFM_FREQ);
  }

  // If you are using a high power RF69 eg RFM69HW, you *must* set a Tx power with the
  // ishighpowermodule flag set like this:
  rf69.setTxPower(20, true);  // range from 14-20 for power, 2nd arg must be true for 69HCW

  EVT_subscribe(&evt, &rfm_notify);

  Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");

  for (int motor = 0; motor < 2; motor++) {
    pinMode(enablePin[motor], OUTPUT);
    pinMode(motorPin1[motor], OUTPUT);
    pinMode(motorPin2[motor], OUTPUT);
  }

  setMotor(0, 0);
  setMotor(1, 0);

  EVT_subscribe(&evt, &com_ci_notify);
  EVT_subscribe(&evt, &handleSyncTO);

  if (0 != CI_register(&ci, CI_CMD_NOOP, &handleCmdNoOp)) {
    Error(ERR_CI_REGISTER);
  }
  if (0 != CI_register(&ci, CI_CMD_CLEAR, &handleCmdClear)) {
    Error(ERR_CI_REGISTER);
  }
  if (0 != CI_register(&ci, CI_CMD_BOOM, &handleCmdBoom)) {
    Error(ERR_CI_REGISTER);
  }
  if (0 != CI_register(&ci, CI_CMD_STOP, &handleCmdStop)) {
    Error(ERR_CI_REGISTER);
  }
  if (0 != CI_register(&ci, CI_CMD_FWD, &handleCmdFwd)) {
    Error(ERR_CI_REGISTER);
  }
  if (0 != CI_register(&ci, CI_CMD_BCK, &handleCmdBck)) {
    Error(ERR_CI_REGISTER);
  }
  if (0 != CI_register(&ci, CI_CMD_RT, &handleCmdRT)) {
    Error(ERR_CI_REGISTER);
  }
  if (0 != CI_register(&ci, CI_CMD_LT, &handleCmdLT)) {
    Error(ERR_CI_REGISTER);
  }

  // Send initial TO broadcast and start sync schedule.
  handleSyncTO((EVT_Event_t *)&syncTOEvent);

  Serial.println("Ready");
}

void loop() {
  unsigned long loopStart = millis();

  // Dispatch any scheduled events.
  TMR_handle(&tmr, loopStart);

  uint8_t n = 0;
  
  if (Serial.available() > 0) {
    handleCmd();
  }

  if (rf69.available()) {  
    n = sizeof(rf_buf);

    if (!rf69.recv(rf_buf, &n)) {
      Error(ERR_RFM_RECV);
    }

/*
    Serial.print("Received "); Serial.print(n); Serial.print(" bytes: ");
    for (int i = 0; i < n; i++) {
      Serial.print(rf_buf[i], HEX); Serial.print(' ');
    }
    Serial.println();
*/

    COM_recv(&com, rf_buf, n);

    if (0 != TO_set(&to, TO_PARAM_RFM_RSSI, (uint8_t)rf69.lastRssi())) {
      Error(ERR_TO_SET);
    }
  }

  int iv = analogRead(0);
  
  if (iv < 512 && impactStart == 0) {
    impactStart = millis();
    Serial.println("IMPACT");

    if (0 != TO_set(&to, TO_PARAM_IMPACT, 1)) {
      Error(ERR_TO_SET);
    }

    // TODO: Previously this simply sped up the sync step rather than sync synchronously.
    // That was better because it means there won't be a delay for stopping the rover.
    // I need a better control flow to spearate out these inner and outer-loop behaviors.
    syncTO();

    motorASpeed = 0;
    motorBSpeed = 0;
  } else if (impactStart > 0 && iv > 512 && millis() - impactStart > 500) {
    impactStart = 0;

    if (0 > TO_set(&to, TO_PARAM_IMPACT, 0)) {
      Error(ERR_TO_SET);
    }

    syncTO();
  }

  setMotor(0, motorASpeed);
  setMotor(1, motorBSpeed);

  if (0 != TO_set(&to, TO_PARAM_MOTOR_A, motorASpeed)) {
    Error(ERR_TO_SET);
  }
  if (0 != TO_set(&to, TO_PARAM_MOTOR_B, motorBSpeed)) {
    Error(ERR_TO_SET);
  }
  
  if (0 != TO_set(&to, TO_PARAM_MILLIS, millis())) {
    Error(ERR_TO_SET);
  }

  if (0 != TO_set(&to, TO_PARAM_LOOP, millis()-loopStart)) {
    Error(ERR_TO_SET);
  }
}

void rfm_notify(EVT_Event_t *evt) {
  if (COM_EVT_TYPE_DATA != evt->type) {
    return;
  }

  COM_Data_Event_t *d_evt = (COM_Data_Event_t *)evt;

  Serial.print("Sending ");
  Serial.print(d_evt->length);
  Serial.println();
  for(int i = 0; i < d_evt->length; i++) {
    Serial.print(d_evt->data[i], HEX); Serial.print(' ');
  }
  Serial.println();

  rf69.send(d_evt->data, d_evt->length);
  rf69.waitPacketSent();
}

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

void com_ci_notify(EVT_Event_t *evt)
{
  if (COM_EVT_TYPE_CI != evt->type) {
    return;
  }

  COM_CI_Event_t *ci_evt = (COM_CI_Event_t *)evt;

  int r = CI_ingest(&ci, ci_evt->frame->cmd, ci_evt->data);
  if (0 != COM_send_ci_r(&com, ci_evt->frame->cmd_num, r)) {
    Error(ERR_COM_SEND);
  }

  if (0 != TO_set(&to, TO_PARAM_COM_SEQ, ci_evt->frame->cmd_num)) {
    Error(ERR_TO_SET);
  }
}

int handleCmdNoOp(uint8_t data[CI_MAX_DATA])
{
  return CI_R_OK;
}

int handleCmdBoom(uint8_t data[CI_MAX_DATA])
{
  Error(ERR_CMD);
  return CI_R_ERR_FAILED;
}

int handleCmdClear(uint8_t data[CI_MAX_DATA])
{
  if (0 != TO_set(&to, TO_PARAM_ERROR, 0)) {
    Error(ERR_TO_SET);
    return CI_R_ERR_FAILED;
  } else {
    return CI_R_OK;
  }
}

int handleCmdStop(uint8_t data[CI_MAX_DATA])
{
    motorASpeed = 0;
    motorBSpeed = 0;
    return CI_R_OK;
}

int handleCmdFwd(uint8_t data[CI_MAX_DATA])
{
    uint8_t v1 = data[0];

    if (v1 > 0 && v1 < 256) {
      motorASpeed = v1;
      motorBSpeed = v1;

      return CI_R_OK;
    } else {
      return CI_R_ERR_INVALID;
    }
}

int handleCmdBck(uint8_t data[CI_MAX_DATA])
{
    uint8_t v1 = data[0];

    if (v1 > 0 && v1 < 256) {
      motorASpeed = -v1;
      motorBSpeed = -v1;

      return CI_R_OK;
    } else {
      return CI_R_ERR_INVALID;
    }
}

int handleCmdRT(uint8_t data[CI_MAX_DATA])
{
    uint8_t v1 = data[0];

    if (v1 > 0 && v1 < 256) {
      motorASpeed = v1;
      motorBSpeed = -v1;

      return CI_R_OK;
    } else {
      return CI_R_ERR_INVALID;
    }
}

int handleCmdLT(uint8_t data[CI_MAX_DATA])
{
    uint8_t v1 = data[0];

    if (v1 > 0 && v1 < 256) {
      motorASpeed = -v1;
      motorBSpeed = v1;

      return CI_R_OK;
    } else {
      return CI_R_ERR_INVALID;
    }
}

void handleCmd()
{
    char cmd[MAX_CMD] = {0};

    int n = Serial.readBytesUntil('\n', cmd, sizeof(cmd));

    while (Serial.available() > 0) {
      Serial.read();
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


void handleSyncTO(EVT_Event_t *e)
{
    if (EVT_TYPE_SYNC_TO != e->type) {
      return;
    }

    syncTO();

    if (0 != TMR_enqueue(&tmr, (EVT_Event_t *)&syncTOEvent, millis()+1000)) {
      Error(ERR_TMR_ENQUEUE);
    }
}

void syncTO()
{
    uint8_t to_buf[MAX_TO_SIZE];
    size_t to_size = TO_encode(&to, to_buf, sizeof(to_buf));
    if (0 < to_size) {
      if (0 != COM_send_to(&com, to_buf, to_size)) {
        Error(ERR_COM_SEND);
      }
    }
}

void debugEvent(EVT_Event_t *e)
{
  Serial.print("Event: ");
  Serial.println(e->type);
}

void Error(uint8_t errno) 
{
  TO_set(&to, TO_PARAM_ERROR, errno);

  if (Serial.availableForWrite()) {
    Serial.print("ERROR: ");
    Serial.println(errno);
  }
}