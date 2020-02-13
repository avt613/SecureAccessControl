#include <SPI.h>
#include <MFRC522.h>

#include <SD.h>
#define chipSelect    3
#define RST_PIN         9          // Configurable, see typical pin layout above
#define SS_PIN          10         // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance


byte storedCard[4] = {135,34,112,96};   // Stores an ID read from EEPROM
byte readCard[4];   // Stores scanned ID read from RFID Module
boolean Status = 0;

void setup() {


  Serial.begin(9600);    // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();      // Init SPI bus
  
  Serial.print("Initializing SD card...");
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    // while (1);
  }
  Serial.println("card initialized.");
  
  mfrc522.PCD_Init();   // Init MFRC522
  delay(4);       // Optional delay. Some board do need more time after init to be ready, see Readme
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);

}

void loop() {
  mfrc522.PCD_Init();   // Init MFRC522
  Status = getID();
  if(Status == 1){
      Serial.println("Stored Card Data");
      for ( uint8_t i = 0; i < 4; i++) {
        Serial.print(storedCard[i]);
        Serial.print(" ");
        }
      Serial.println("");

      if(readCard[0] == storedCard[0] && readCard[1] == storedCard[1] && readCard[2] == storedCard[2] && readCard[3] == storedCard[3]){
        Serial.println("Card Known");
        }else{
          Serial.println("Card Unknown");
          }
    }  
}


uint8_t getID() {
  // Getting ready for Reading PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Since a PICC placed get Serial and continue
    return 0;
  }
  // There are Mifare PICCs which have 4 byte or 7 byte UID care if you use 7 byte PICC
  // I think we should assume every PICC as they have 4 byte UID
  // Until we support 7 byte PICCs
  Serial.println("Scanned PICC's UID:");
  for ( uint8_t i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
    Serial.print(readCard[i]);
    Serial.print(" ");
  }
  Serial.println("");
  mfrc522.PICC_HaltA(); // Stop reading
  return 1;
}
