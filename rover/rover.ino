
#include <SPI.h>
#include <Ethernet.h>

extern "C" {
#include "stubborn.h"
#include "to.h"
#include "com.h"
#include "ci.h"
#include "tmr.h"
#include "tbl.h"
}

#include "events.h"
#include "errors.h"

#define LED 13

#define MAX_CMD       64
#define MAX_TO_SIZE   60
#define MAX_LOOP_DURATION 5

unsigned long lastTOSync = 0;

// syncTOEvent is triggered to prepare and send a TO (telemetry output) packet.
// There is only one as this is not something that will be used in more places.
EVT_Event_t syncTOEvent = {EVT_TYPE_SYNC_TO};

EVT_Event_t ciStopEvent = {EVT_TYPE_CI_STOP};

#define TBL_VAL_IMPACT_ENABLE 1
#define TBL_VAL_MOTOR_BIAS_A  2
#define TBL_VAL_MOTOR_BIAS_B  3

// Channels for communications layer
#define COM_CHANNEL_TO 1
#define COM_CHANNEL_CI 2

TO_t to   = {0};
EVT_t evt = {0};
TMR_t tmr = {0};
COM_t com = {0};
CI_t ci   = {0};
TBL_t tbl = {0};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial) { delay(1); } // wait until serial console is open, remove if not tethered to computer
  Serial.println("Booting..");

  EVT_init(&evt);
  ci.evt = &evt;
  tmr.evt = &evt;
  COM_init(&com, &evt, &tmr);

  // EVT_subscribe(&evt, &debugEvent);

  if (0 != TO_init(&to)) {
    Serial.println("FAIL: TO init");
    abort();
  }

  TBL_init(&tbl, &saveTable, &loadTable);

  if (0 != TBL_set_default(&tbl, TBL_VAL_IMPACT_ENABLE, 1)) {
    Error(ERR_TBL_SET);
  }

  rfmInit();
  motorInit();

  EVT_subscribe(&evt, &com_ci_notify);
  EVT_subscribe(&evt, &handleSyncTO);
  EVT_subscribe(&evt, &handleCIStop);
  EVT_subscribe(&evt, &handleImpactSensor);
  EVT_subscribe(&evt, &handleMotorSpeed);

  if (0 != CI_register(&ci, CI_CMD_NOOP, &handleCmdNoOp)) {
    Error(ERR_CI_REGISTER);
  }
  if (0 != CI_register(&ci, CI_CMD_CLEAR, &handleCmdClear)) {
    Error(ERR_CI_REGISTER);
  }
  if (0 != CI_register(&ci, CI_CMD_BOOM, &handleCmdBoom)) {
    Error(ERR_CI_REGISTER);
  }
  if (0 != CI_register(&ci, CI_CMD_EXT_STOP, &handleCmdStop)) {
    Error(ERR_CI_REGISTER);
  }
  if (0 != CI_register(&ci, CI_CMD_EXT_FWD, &handleCmdFwd)) {
    Error(ERR_CI_REGISTER);
  }
  if (0 != CI_register(&ci, CI_CMD_EXT_FFWD, &handleCmdFfwd)) {
    Error(ERR_CI_REGISTER);
  }
  if (0 != CI_register(&ci, CI_CMD_EXT_BCK, &handleCmdBck)) {
    Error(ERR_CI_REGISTER);
  }
  if (0 != CI_register(&ci, CI_CMD_EXT_RT, &handleCmdRT)) {
    Error(ERR_CI_REGISTER);
  }
  if (0 != CI_register(&ci, CI_CMD_EXT_LT, &handleCmdLT)) {
    Error(ERR_CI_REGISTER);
  }
  if (0 != CI_register(&ci, CI_CMD_EXT_SET, &handleCmdSet)) {
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

  checkRFM();

  checkImpactSensor();

  if (0 != TO_set(&to, TO_PARAM_MILLIS, millis())) {
    Error(ERR_TO_SET);
  }

  unsigned long loopDuration = millis()-loopStart;
  if (0 != TO_set(&to, TO_PARAM_LOOP, loopDuration)) {
    Error(ERR_TO_SET);
  }

/*
  if (MAX_LOOP_DURATION < loopDuration) {
    Error(ERR_SLOW_LOOP);
  }
*/
}

void com_ci_notify(EVT_Event_t *evt)
{
  if (COM_EVT_TYPE_MSG != evt->type) {
    return;
  }

  COM_Msg_Event_t *msg_evt = (COM_Msg_Event_t *)evt;

  if (COM_CHANNEL_CI != msg_evt->channel) {
    return;
  }

  if (COM_TYPE_REQ != msg_evt->msg_type) {
    return;
  }

  uint8_t cmd = 0;
  uint8_t data[CI_MAX_DATA] = {0};

  // Make sure we have enough bytes
  if (5 <= msg_evt->length) {
    cmd = msg_evt->data[0];
    memcpy(data, &msg_evt->data[1], 4);
  }

  int r = CI_ingest(&ci, cmd, data);

  if (0 != COM_send_reply(&com, msg_evt->channel, msg_evt->seq_num, (uint8_t *)&r, sizeof(r), millis())) {
    Error(ERR_COM_SEND);
  }

  if (0 != TO_set(&to, TO_PARAM_COM_SEQ, msg_evt->seq_num)) {
    Error(ERR_TO_SET);
  }
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
      if (0 != COM_send(&com, COM_TYPE_BROADCAST, COM_CHANNEL_TO, to_buf, to_size, millis())) {
        Error(ERR_COM_SEND);
      }
    }
}
