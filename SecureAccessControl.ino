/*
 * Rui Santos 
 * Complete Project Details https://randomnerdtutorials.com
 */

#include <MFRC522.h> // for the RFID
#include <SPI.h> // for the RFID and SD card module
#include <SD.h> // for the SD card

// define pins for RFID
#define CS_RFID 10
#define RST_RFID 9
// define select pin for SD card module
#define CS_SD 3

// Create a file to store the data
File myFile;

// Instance of the class for RFID
MFRC522 rfid(CS_RFID, RST_RFID); 

// Variable to hold the tag's UID
String uidString;



void setup() {
   
  // Init Serial port
  Serial.begin(9600);
  while(!Serial); // for Leonardo/Micro/Zero
  
  // Init SPI bus
  SPI.begin(); 
  // Init MFRC522 
  rfid.PCD_Init(); 

  // Setup for the SD card
  Serial.print("Initializing SD card...");
  if(!SD.begin(CS_SD)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
}

void loop() {
  //look for new cards
  if(rfid.PICC_IsNewCardPresent()) {
    readRFID();
    verifyCheckIn();
  }
  delay(10);
}

void readRFID() {
  rfid.PICC_ReadCardSerial();
  Serial.print("Tag UID: ");
  uidString = String(rfid.uid.uidByte[0]) + "-" + String(rfid.uid.uidByte[1]) + "-" + 
    String(rfid.uid.uidByte[2]) + "-" + String(rfid.uid.uidByte[3]);
  Serial.println(uidString);
  delay(100);
}

void verifyCheckIn(){
  digitalWrite(CS_SD,LOW);
  myFile=SD.open("knowncards/" + uidString + ".txt", FILE_WRITE);
  if(myFile){
    myFile.close();
    Serial.println("Access Granted");
      
      // Enables SD card chip select pin
      digitalWrite(CS_SD,LOW);
      // Open file
      myFile=SD.open("log.txt", FILE_WRITE);
      // If the file opened ok, write to it
      if (myFile) {
        Serial.println("File opened ok");
        myFile.print(uidString + ", Access Granted");
        Serial.println("sucessfully written on SD card");
        myFile.close();
      }
      else {
        Serial.println("error opening data.txt");  
      }
      // Disables SD card chip select pin  
      digitalWrite(CS_SD,HIGH);
    
  }
  else{
    Serial.println("Access Denied");
    // Enables SD card chip select pin
      digitalWrite(CS_SD,LOW);
      // Open file
      myFile=SD.open("log.txt", FILE_WRITE);
      // If the file opened ok, write to it
      if (myFile) {
        Serial.println("File opened ok");
        myFile.print(uidString + ", Access Denied");
        Serial.println("sucessfully written on SD card");
        myFile.close();
      }
      else {
        Serial.println("error opening data.txt");  
      }
      // Disables SD card chip select pin  
      digitalWrite(CS_SD,HIGH);
  }
}
