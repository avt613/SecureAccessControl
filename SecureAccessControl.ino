/* Error Codes
 *  3 - Failed to initialise SD Module
*   4 - failed to write to Log 
*/

// use pixels.setPixelColor(LED#, Color); to set a LED color (Red/ Green/ Blue/ Off)
// pixels.clear(); will set all LEDs to off
// and then use pixels.show();

#include <Adafruit_NeoPixel.h>
#define NeoPixelPin 2 // Which pin on the Arduino is connected to the NeoPixels?
#define NumNeoPixels 30 // How many NeoPixels are attached to the Arduino?
Adafruit_NeoPixel pixels(NumNeoPixels, NeoPixelPin, NEO_GRB + NEO_KHZ800);
static uint32_t Red   = pixels.Color(255,   0,   0);
static uint32_t Green = pixels.Color(  0, 255,   0);
static uint32_t Blue  = pixels.Color(  0,   0, 255);
static uint32_t Off   = pixels.Color(  0,   0,   0);
#define NeoPixelNotify 0

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
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.setBrightness(30); // Set BRIGHTNESS to about 1/5 (max = 255)
  pixels.clear(); // Set all pixel colors to 'off'
  pixels.show();
  Serial.begin(9600); // Init Serial port
//  while(!Serial); // Wait for computer serial box
  SPI.begin(); // Init SPI bus
  
  // Setup RFID module
  rfid.PCD_Init(); // Init MFRC522
  rfid.PCD_SetAntennaGain(rfid.RxGain_max); // increases the range of the RFID module

  // Setup SD card module
  Serial.print("Initializing SD card...");
  if(!SD.begin(CS_SD)) {
    Serial.println("initialization failed!");
    ErrorCode(3);  
  }
  Serial.println("initialization done.");
}

//*****************************************************************************************//

void loop() {
  //look for new cards
  if(rfid.PICC_IsNewCardPresent()) {
    pixels.setPixelColor(NeoPixelNotify, Blue);
    pixels.show();
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
    AccessGranted(3000);
    uidString = "0";
  }
  else{
    AccessDenied(1000);
    uidString = "0";
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
        ErrorCode(4);  
      }
  }

//*****************************************************************************************//
  
void AccessGranted(int TimeDoorOpen){
  pixels.setPixelColor(NeoPixelNotify, Green);
  pixels.show();
  Serial.println("Access Granted");
  LogToSD(uidString + ", Access Granted");
  delay(TimeDoorOpen);
  pixels.setPixelColor(NeoPixelNotify, Off);
  pixels.show();
}

void AccessDenied(int LockoutTime){
  pixels.setPixelColor(NeoPixelNotify, Red);
  pixels.show();
  Serial.println("Access Denied");
  LogToSD(uidString + ", Access Denied");
  delay(LockoutTime);
  pixels.setPixelColor(NeoPixelNotify, Off);
  pixels.show();
}

//*****************************************************************************************//

void ErrorCode(int ErrorNum){
  pixels.setPixelColor(NeoPixelNotify, Blue);
  pixels.show();
  delay(1000);
  for(int i = 0; i < ErrorNum; i++){
    pixels.setPixelColor(NeoPixelNotify, Red);
    pixels.show();
    delay(500);
    Serial.println("Error Code: " + ErrorNum);
    LogToSD("Error Code: " + ErrorNum);
    pixels.setPixelColor(NeoPixelNotify, Off);
    pixels.show();
    delay(500);
  }
}
