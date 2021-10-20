#include <ArduCAM.h>
#include <SPI.h>
#include <Wire.h>

#define CAM_SPI_CS 8
ArduCAM myCAM(OV2640, CAM_SPI_CS);

#define CAM_POLL_SNAP_MS 10
uint32_t camSnapLength;
#define CAM_BUF_SIZE 50
uint8_t camBuf[CAM_BUF_SIZE];
uint8_t camBufNdx;
bool camIsHeader = false;

EVT_Event_t camPollSnapEvent = {EVT_TYPE_CAM_POLL_SNAP};

void camInit()
{
  uint8_t testVal = 0; 
  camSnapLength = 0;
  camBufNdx = 0;

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

void camSnap() 
{
  camSnapLength = 0;

  myCAM.flush_fifo();
  myCAM.clear_fifo_flag();

  myCAM.start_capture();

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

  camSnapLength = myCAM.read_fifo_length();

  if (0 != TO_set(&to, TO_PARAM_CAM_LEN, camSnapLength)) {
    Error(ERR_TO_SET);
  }
}

void checkCamData()
{
  uint8_t temp = 0, temp_last = 0;

  if (0 < camBufNdx) {
    int ret = COM_send(&com, COM_TYPE_BROADCAST, COM_CHANNEL_CAM_DATA, camBuf, camBufNdx, millis());
    if (0 == ret) {
      camBufNdx = 0;
    } else if (-2 != ret) {
      // We only complain about non-buffer errors.
      Error(ERR_COM_SEND);
    }
  }

  if (0 < camSnapLength) {
    myCAM.CS_LOW();
    myCAM.set_fifo_burst();

    while (camSnapLength && camBufNdx < CAM_BUF_SIZE) {
      temp_last = temp;
      temp = SPI.transfer(0x00);
      camSnapLength--;

      if (camIsHeader) { 
        camBuf[camBufNdx++] = temp;
        if ( (temp == 0xD9) && (temp_last == 0xFF) ) {
          camIsHeader = false;
          break;
        }
      } else if ((temp == 0xD8) & (temp_last == 0xFF)) {
        camIsHeader = true;
        camBuf[camBufNdx++] = temp_last;
        camBuf[camBufNdx++] = temp;   
      } 
    }

    myCAM.CS_HIGH();

    if (0 != TO_set(&to, TO_PARAM_CAM_LEN, camSnapLength)) {
      Error(ERR_TO_SET);
    }
  }
}
