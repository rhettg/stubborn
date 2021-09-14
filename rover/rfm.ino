#include <RH_RF69.h>

extern "C" {
    #include "evt.h"
    #include "events.h"
    #include "errors.h"
}

#define RFM69_INT     3  // 
#define RFM69_CS      4  //
#define RFM69_RST     2

#define RF69_FREQ 915.0

EVT_Event_t rfmReadyEvent = {EVT_TYPE_RFM_READY};

RH_RF69 rf69(RFM69_CS, RFM69_INT);

uint8_t rf_buf[RH_RF69_MAX_MESSAGE_LEN];

void rfmInit()
{
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

  EVT_subscribe(&evt, &handleRFMReady);
  EVT_subscribe(&evt, &rfm_notify);

  Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");
}

void checkRFM()
{
  if (rf69.available()) {  
    if (0 != EVT_notify(&evt, &rfmReadyEvent)) {
      Error(ERR_EVT);
    }
  }
}

void handleRFMReady(EVT_Event_t *e)
{
    if (EVT_TYPE_RFM_READY != e->type) {
      return;
    }

    uint8_t n = sizeof(rf_buf);

    if (!rf69.recv(rf_buf, &n)) {
      Error(ERR_RFM_RECV);
    }

    Serial.print("["); Serial.print(millis()); Serial.print("] ");
    Serial.print("Received "); Serial.print(n); Serial.print(" bytes: ");
    for (int i = 0; i < n; i++) {
      Serial.print(rf_buf[i], HEX); Serial.print(' ');
    }
    Serial.println();

    COM_recv(&com, rf_buf, n, millis());

    if (0 != TO_set(&to, TO_PARAM_RFM_RSSI, (uint8_t)rf69.lastRssi())) {
      Error(ERR_TO_SET);
    }
}

void rfm_notify(EVT_Event_t *evt) {
  if (COM_EVT_TYPE_DATA != evt->type) {
    return;
  }

  COM_Data_Event_t *d_evt = (COM_Data_Event_t *)evt;

  Serial.print("["); Serial.print(millis()); Serial.print("] ");
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
