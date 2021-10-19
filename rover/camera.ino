#include <ArduCAM.h>
#include <SPI.h>
#include <Wire.h>

#define CAM_SPI_CS 8
ArduCAM myCAM(OV2640, CAM_SPI_CS);

#define CAM_POLL_SNAP_MS 10
uint32_t camSnapLength;

EVT_Event_t camPollSnapEvent = {EVT_TYPE_CAM_POLL_SNAP};

void camInit()
{
  uint8_t testVal = 0; 

  SPI.begin();
  Wire.begin();

  myCAM.write_reg(0x07, 0x80);
  delay(100);
  myCAM.write_reg(0x07, 0x00);
  delay(100);

  while(1){
    // Check if the ArduCAM SPI bus is OK
    myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
    testVal = myCAM.read_reg(ARDUCHIP_TEST1);
    if (testVal != 0x55){
      Serial.println("Camera Error");
      delay(1000);
      continue;
    }else{
      Serial.println("Camera On-Line");
      break;
    }
  }

  myCAM.set_format(JPEG);
  myCAM.InitCAM();
  myCAM.OV2640_set_JPEG_size(OV2640_320x240);
}

void cameraSnap() 
{
  camSnapLength = 0;

  myCAM.flush_fifo();
  myCAM.clear_fifo_flag();

  myCAM.start_capture();
  Serial.println("Started capture");

  if (0 != TMR_enqueue(&tmr, &camPollSnapEvent, millis()+CAM_POLL_SNAP_MS)) {
    Error(ERR_TMR_ENQUEUE);
  }
}

void handleCamPollSnap(EVT_Event_t *evt)
{
  if (EVT_TYPE_CAM_POLL_SNAP != evt->type) {
    return;
  }

  if (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK)) {
    if (0 != TMR_enqueue(&tmr, &camPollSnapEvent, millis()+CAM_POLL_SNAP_MS)) {
        Error(ERR_TMR_ENQUEUE);
    }
    return;
  }

  Serial.println("Capture done");

  camSnapLength = myCAM.read_fifo_length();

  Serial.print("Storing ");
  Serial.print(camSnapLength, DEC);
  Serial.println(" byte image.");

  if (0 != TO_set(&to, TO_PARAM_CAM_LEN, camSnapLength)) {
    Error(ERR_TO_SET);
  }
}
