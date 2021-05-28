extern "C" {
    #include "evt.h"
    #include "to.h"
    #include "events.h"
    #include "errors.h"
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
