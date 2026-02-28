#include "SD_Card.h"

bool SD_Init() {
  pinMode(SD_CS, OUTPUT);    
  digitalWrite(SD_CS, HIGH);               
  if (SD.begin(SD_CS, SPI, 40000000)) {
    printf("SD card initialization successful!\r\n");
  } else {
    printf("SD card initialization failed!\r\n");
    return false;
  }
  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE){
    printf("No SD card attached\r\n");
    return false;
  }
  return true;
}