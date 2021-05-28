extern "C" {
    #include "evt.h"
    #include "to.h"
    #include "events.h"
    #include "errors.h"
}

struct {
  EVT_Event_t event = {EVT_TYPE_IMPACT_SENSOR};
  int value;

} impactSensorEvent;

void checkImpactSensor()
{
  impactSensorEvent.value = analogRead(0);
  if (0 != EVT_notify(&evt, (EVT_Event_t *)&impactSensorEvent)) {
    Error(ERR_EVT);
  }
}

void handleImpactSensor(EVT_Event_t *e)
{
  if (EVT_TYPE_IMPACT_SENSOR != e->type) {
    return;
  }

  if (e != (EVT_Event_t *)&impactSensorEvent) {
    return;
  }

  static unsigned long impactStart = 0;
  int iv = impactSensorEvent.value;

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

    changeSpeed(0, 0);
  } else if (impactStart > 0 && iv > 512 && millis() - impactStart > 500) {
    impactStart = 0;

    if (0 > TO_set(&to, TO_PARAM_IMPACT, 0)) {
      Error(ERR_TO_SET);
    }

    syncTO();
  }
}