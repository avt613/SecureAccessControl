/* With help from:
 * Rui Santos https://randomnerdtutorials.com
 */

#include <MFRC522.h> // for the RFID
#include <SPI.h> // for the RFID and SD card module
#include <SD.h> // for the SD card

// define pins for RFID
#define CS_RFID 10
#define RST_RFID 9
// define select pin for SD card module
#define CS_SD 3


File myFile; // Create a file to store the data
String LogFile = "log.txt"; // name of Log File


MFRC522 rfid(CS_RFID, RST_RFID); // Instance of the class for RFID


String uidString;// Variable to hold the tag's UID

//*****************************************************************************************//

void setup() {
  Serial.begin(9600); // Init Serial port
  while(!Serial); // Wait for computer serial box
  SPI.begin(); // Init SPI bus
  
  // Setup RFID module
  rfid.PCD_Init(); // Init MFRC522
  rfid.PCD_SetAntennaGain(rfid.RxGain_max); // increases the range of the RFID module

  // Setup SD card module
  Serial.print("Initializing SD card...");
  if(!SD.begin(CS_SD)) {
    Serial.println("initialization failed!");
  }
  Serial.println("initialization done.");
}

//*****************************************************************************************//

void loop() {
  //look for new cards
  if(rfid.PICC_IsNewCardPresent()) {
    readRFID();
    verifyRFIDCard();
  }
  delay(10);
}

//*****************************************************************************************//

void readRFID() {
  rfid.PICC_ReadCardSerial();
  Serial.print("Tag UID Scanned: ");
  uidString = String(rfid.uid.uidByte[0], HEX) + String(rfid.uid.uidByte[1], HEX) + 
    String(rfid.uid.uidByte[2], HEX) + String(rfid.uid.uidByte[3], HEX);
  Serial.println(uidString);
  delay(100);
}

//*****************************************************************************************//

void verifyRFIDCard(){
  if(SD.exists(uidString + ".txt")){
    AccessGranted(3);
  }
  else{
    AccessGranted();
  }
  Serial.println("");
}

//*****************************************************************************************//

void LogToSD(String DataToLogToSD){
        // Open file
      myFile=SD.open(LogFile, FILE_WRITE);
      // If the file opened ok, write to it
      if (myFile) {
        Serial.println("Log opened ok");
        myFile.println(DataToLogToSD);
        Serial.println("sucessfully written to log");
        myFile.close();
        Serial.println("Log closed");
      }
      else {
        Serial.println("error opening " + LogFile);  
      }
  }

//*****************************************************************************************//
  
void AccessGranted(int TimeDoorOpen){
  Serial.println("Access Granted");
  LogToSD(uidString + ", Access Granted")
}

void AccessGranted(){
  Serial.println("Access Denied");
  LogToSD(uidString + ", Access Denied");
}
