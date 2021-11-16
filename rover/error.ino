extern "C" {
    #include "evt.h"
    #include "to.h"
    #include "events.h"
    #include "errors.h"
}

void debugEvent(EVT_Event_t *e)
{
  #ifdef ENABLE_SERIAL
  Serial.print("Event: ");
  Serial.println(e->type);
  #endif
}

void Error(uint8_t errno) 
{
  TO_set(&to, TO_PARAM_ERROR, errno);

#ifdef ENABLE_SERIAL
  if (Serial.availableForWrite()) {
    Serial.print("ERROR: ");
    Serial.println(errno);
  }
#endif
}
