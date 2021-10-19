#include <ArduCAM.h>
#include <SPI.h>
#include <Wire.h>

#define CAMERA_SPI_CS 8
ArduCAM myCAM(OV2640, CAMERA_SPI_CS);

void cameraInit()
{
  uint8_t testVal = 0; 

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
}