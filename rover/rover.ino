
#include <SPI.h>

extern "C" {
#include "stubborn.h"
#include "to.h"
#include "com.h"
#include "ci.h"
#include "tmr.h"
}

#include "events.h"
#include "errors.h"

uint8_t enablePin[] = {10, 5};

#define LED 13

#define MAX_CMD       64
#define MAX_TO_SIZE   60
#define MAX_LOOP_DURATION 5

unsigned long lastTOSync = 0;

// syncTOEvent is triggered to prepare and send a TO (telemetry output) packet.
// There is only one as this is not something that will be used in more places.
EVT_Event_t syncTOEvent = {EVT_TYPE_SYNC_TO};

EVT_Event_t ciStopEvent = {EVT_TYPE_CI_STOP};

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

  // EVT_subscribe(&evt, &debugEvent);

  if (0 != TO_init(&to)) {
    Serial.println("FAIL: TO init");
    abort();
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
  if (0 != CI_register(&ci, CI_CMD_EXT_BCK, &handleCmdBck)) {
    Error(ERR_CI_REGISTER);
  }
  if (0 != CI_register(&ci, CI_CMD_EXT_RT, &handleCmdRT)) {
    Error(ERR_CI_REGISTER);
  }
  if (0 != CI_register(&ci, CI_CMD_EXT_LT, &handleCmdLT)) {
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
