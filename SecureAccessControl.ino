/* Error Codes
 *  3 - Failed to initialise SD Module
*   4 - failed to write to Log 
*   5 - Could not find RTC Module
*   6 - error opening "uidString".txt in ManagerMode()
*   7 - Could not find RC522 Module
*/

#define AccessGrantedTime 3000 // Time to keep the door unlocked for in milliseconds
#define AccessDeniedTime 1000 // Time to wait after an incorrect unlock attempt in milliseconds
// Setup RTC Module
#include <Wire.h>
#include <RTClib.h>
RTC_DS1307 rtc; 
// Setting up the NeoPixel Strip
#include <Adafruit_NeoPixel.h>
#define NeoPixelPin 2 // Which pin on the Arduino is connected to the NeoPixels?
#define NumNeoPixels 2 // How many NeoPixels are attached to the Arduino?
#define NeoPixelNotify 0 // Determine which LED to use as a notification LED
Adafruit_NeoPixel pixels(NumNeoPixels, NeoPixelPin, NEO_GRB + NEO_KHZ800);
static uint32_t Red   = pixels.Color(255,   0,   0);  // Setting easy name for common colours for the LED's 
static uint32_t Green = pixels.Color(  0, 255,   0);  // use pixels.setPixelColor(LED#, Color); to set a LED color (Red/ Green/ Blue/ Off)
static uint32_t Blue  = pixels.Color(  0,   0, 255);  // pixels.clear(); will set all LEDs to off
static uint32_t Off   = pixels.Color(  0,   0,   0);  // and then use pixels.show();
// Setting up the RFID and SD modules
#include <MFRC522.h> // for the RFID
#include <SPI.h> // for the RFID and SD card module
#include <SD.h> // for the SD card
#define CS_RFID 10    // define pins for RFID
#define RST_RFID 9    // define pins for RFID
#define CS_SD 3   // define select pin for SD card module
MFRC522 rfid(CS_RFID, RST_RFID); // Instance of the class for RFID
MFRC522::MIFARE_Key keyA = {keyByte: {0x35, 0x35, 0x35, 0x32, 0x39, 0x34}};
MFRC522::MIFARE_Key keyB = {keyByte: {0x00, 0x00, 0xC8, 0xFF, 0x7F, 0x00}};
File myFile; // Create a file to store the data
String LogFile = "LOG.TXT"; // name of Log File
String uidString; // Variable to hold the tag's UID
String ACBits = "0f00ff69"; // AC bits the cards must have

//*****************************************************************************************//

void setup() {  
  Serial.begin(9600); // Init Serial port
  // while(!Serial); // Wait for computer serial box
  // Setup NeoPixels
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.setBrightness(10); // Set BRIGHTNESS (max = 255)
  pixels.clear(); // Set all pixel colors to 'off'
  pixels.show();
  
  // Setup RTC Module
  if (! rtc.begin()) {
    Serial.println(F("Couldn't find RTC"));
    ErrorCode(5);
  }
  if (! rtc.isrunning()) {
    Serial.println(F("RTC is NOT running!"));
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));  
  }
  
  SPI.begin(); // Init SPI bus for RFID and SD modules
  // Setup RFID module
  if (rfid.PCD_PerformSelfTest() != 1){
      Serial.println(F("Couldn't find RC522"));
      ErrorCode(7);
    }
  rfid.PCD_Init(); // Init MFRC522
  //rfid.PCD_SetAntennaGain(rfid.RxGain_max); // increases the range of the RFID module

  // Setup SD card module
  if(!SD.begin(CS_SD)) {
    Serial.println(F("Initializing SD card failed!"));
    ErrorCode(3);  
  }
  FlashNeoPixel(NeoPixelNotify, 1, 250, Red);
  FlashNeoPixel(NeoPixelNotify, 1, 250, Blue);
  FlashNeoPixel(NeoPixelNotify, 1, 250, Green);

  randomSeed(analogRead(0));  // used to generate random numbers
  Serial.print(F("Using key for A: "));
    for (byte i = 0; i < MFRC522::MF_KEY_SIZE; i++) {
        Serial.print(keyA.keyByte[i] < 0x10 ? " 0" : " ");
        Serial.print(keyA.keyByte[i], HEX);
    }
  Serial.println();
  Serial.print(F("Using key for B: "));
    for (byte i = 0; i < MFRC522::MF_KEY_SIZE; i++) {
        Serial.print(keyB.keyByte[i] < 0x10 ? " 0" : " ");
        Serial.print(keyB.keyByte[i], HEX);
    }
  Serial.println();
  Serial.print(F("Using AC bits  : "));
  Serial.println(ACBits);
  LogToSD(F("Startup Initialising Complete"));
  Serial.println(F("Startup Initialising Complete"));
}

//*****************************************************************************************//

void loop() {
  /*
   if(Serial.available()){
    //Serial.print("You typed: " );
    //Serial.println(Serial.read());
    ManagerMode();
  }*/
  //look for new cards
  if(rfid.PICC_IsNewCardPresent()) {
    readRFID();
    if(uidString != "0000"){  // ignore if it didn't read the card properly
      pixels.setPixelColor(NeoPixelNotify, Blue);
      pixels.show();
      if(verifyRFIDCard() == 1){
        AccessGranted(AccessGrantedTime);
      }else{
        AccessDenied(AccessDeniedTime);
      }
      // Halt PICC
      rfid.PICC_HaltA();
      // Stop encryption on PCD
      rfid.PCD_StopCrypto1();
      ResetRFIDReadVariables();
    }
  }
  delay(10);
}

//*****************************************************************************************//

void readRFID() {
  rfid.PICC_ReadCardSerial();
  Serial.print(F("Card with uid detected: "));
  uidString = (rfid.uid.uidByte[0] < 0x10 ? "0" : "") + String(rfid.uid.uidByte[0], HEX) + (rfid.uid.uidByte[1] < 0x10 ? "0" : "") + String(rfid.uid.uidByte[1], HEX) + 
    (rfid.uid.uidByte[2] < 0x10 ? "0" : "") + String(rfid.uid.uidByte[2], HEX) + (rfid.uid.uidByte[3] < 0x10 ? "0" : "") + String(rfid.uid.uidByte[3], HEX);
  Serial.println(uidString);
  LogToSD("Card with uid detected: " + uidString);
  delay(100);
}

//*****************************************************************************************//

int verifyRFIDCard(){
  //check UUID
  if(!SD.exists(uidString + ".txt")){
    Serial.println(F("UID not recognized"));
    return(2);
  }
  
    byte sector         = 1;
    byte blockAddr      = (4 * (sector + 1)) - 2;
    byte trailerBlock   = (4 * (sector + 1)) - 1;
    byte buffer[18];
    for (byte i = 0; i < 16; i++) {
        buffer[i] = 0x00; 
    }
    byte size = sizeof(buffer);

    // Authenticate using key A
    Serial.println(F("Authenticating using key A... "));
    MFRC522::StatusCode StatusKeyA = (MFRC522::StatusCode) rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &keyA, &(rfid.uid));
    // Check Key A
    if (StatusKeyA != MFRC522::STATUS_OK) {
      Serial.println(F("Key A not recognized"));
      return(3);
    }
    
    // Authenticate using key B
    Serial.println(F("Authenticating using key B... "));
    MFRC522::StatusCode StatusKeyB = (MFRC522::StatusCode) rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, trailerBlock, &keyB, &(rfid.uid));
    // Check Key B
    if (StatusKeyB != MFRC522::STATUS_OK) {
      Serial.println(F("Key B not recognized"));
      return(4);
    }
    
    
    //Authenticate using AC bits
    Serial.println(F("Authenticating using AC bits... "));
    MFRC522::StatusCode StatusReadingACBlock = (MFRC522::StatusCode) rfid.MIFARE_Read(trailerBlock, buffer, &size);
    String ACString = (buffer[6] < 0x10 ? "0" : "") + String(buffer[6],HEX) + (buffer[7] < 0x10 ? "0" : "") + String(buffer[7],HEX) + 
      (buffer[8] < 0x10 ? "0" : "") + String(buffer[8],HEX) + (buffer[9] < 0x10 ? "0" : "") + String(buffer[9],HEX);
    //Serial.println(ACString);
    if (StatusReadingACBlock != MFRC522::STATUS_OK || ACString != ACBits) {
      Serial.println(F("AC not recognized"));
      return(5);
    }
    
    Serial.println(F("Reading card data "));
    MFRC522::StatusCode StatusReadingCardData = (MFRC522::StatusCode) rfid.MIFARE_Read(blockAddr, buffer, &size);
    String CardData;
    for (byte i = 0; i < 16; i++) {
        CardData += (buffer[i] < 0x10 ? "0" : "") + String(buffer[i],HEX);
    }
    Serial.println(CardData);
    if (StatusReadingCardData != MFRC522::STATUS_OK) {
      Serial.println(F("Reading card data failed"));
      //return(5);
    }
      
      
    /*
    // Write data to the block
    Serial.print(F("Writing data into block ")); Serial.print(blockAddr);
    Serial.println(F(" ..."));
    for (byte i = 0; i < 16; i++) {
        Serial.print(dataBlock[i] < 0x10 ? " 0" : " ");
        Serial.print(dataBlock[i], HEX);
    }; 
    Serial.println();
    status = (MFRC522::StatusCode) rfid.MIFARE_Write(blockAddr, dataBlock, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Write() failed: "));
        Serial.println(rfid.GetStatusCodeName(status));
    }
    Serial.println();

    // Read data from the block (again, should now be what we have written)
    Serial.print(F("Reading data from block ")); Serial.print(blockAddr);
    Serial.println(F(" ..."));
    status = (MFRC522::StatusCode) rfid.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(rfid.GetStatusCodeName(status));
    }
    Serial.print(F("Data in block ")); Serial.print(blockAddr); Serial.println(F(":"));
    for (byte i = 0; i < 16; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }; Serial.println();

    // Check that data in block is what we have written
    // by counting the number of bytes that are equal
    Serial.println(F("Checking result..."));
    byte count = 0;
    for (byte i = 0; i < 16; i++) {
        // Compare buffer (= what we've read) with dataBlock (= what we've written)
        if (buffer[i] == dataBlock[i])
            count++;
    }
    Serial.print(F("Number of bytes that match = ")); Serial.println(count);
    if (count == 16) {
        Serial.println(F("Success :-)"));
    } else {
        Serial.println(F("Failure, no match :-("));
        Serial.println(F("  perhaps the write didn't work properly..."));
    }
    Serial.println();

    // Dump the sector data
    Serial.println(F("Current data in sector:"));
    rfid.PICC_DumpMifareClassicSectorToSerial(&(rfid.uid), &keyA, sector);
    Serial.println();
    
*/
    
    
  
  Serial.println("");
  return(1);
}

//*****************************************************************************************//

void LogToSD(String DataToLogToSD){
        // Open file
      myFile=SD.open(LogFile, FILE_WRITE);
      // If the file opened ok, write to it
      if (myFile) {
        DateTime now = rtc.now(); //get current time
        // Serial.println(F("Log opened ok"));
        myFile.print(now.toString("YYYY.MM.DD, hh:mm:ss, "));
        myFile.println(DataToLogToSD);
        Serial.println(F("sucessfully written to log"));
        myFile.close();
        // Serial.println(F("Log closed"));
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
  // activate relay
  Serial.println(F("Access Granted"));
  LogToSD("Access Granted to card: " + uidString);
  delay(TimeDoorOpen);
  // de-activate relay
  pixels.setPixelColor(NeoPixelNotify, Off);
  pixels.show();
}

void AccessDenied(int LockoutTime){
  pixels.setPixelColor(NeoPixelNotify, Red);
  pixels.show();
  Serial.println(F("Access Denied"));
  LogToSD("Access Denied  to card: " + uidString);
  delay(LockoutTime);
  pixels.setPixelColor(NeoPixelNotify, Off);
  pixels.show();
}

//*****************************************************************************************//

void ErrorCode(int ErrorNum){
  //Serial.println("");
  Serial.print(F("Error Code: "));
  Serial.println(String(ErrorNum));
  Serial.println("----------------------------------------");
  pixels.clear();
  delay(500);
  FlashNeoPixel(NeoPixelNotify, 2, 500, Blue);
  FlashNeoPixel(NeoPixelNotify, ErrorNum, 500, Red);
  FlashNeoPixel(NeoPixelNotify, 2, 500, Blue);
  asm volatile ("  jmp 0"); //restart Uno
}


void FlashNeoPixel(int PixelNum, int NumberOfTimesToFlash, int FlashDelayTime, uint32_t FlashColor){
  
  for(int i = 0; i < NumberOfTimesToFlash; i++){
    pixels.setPixelColor(PixelNum, FlashColor);
    pixels.show();
    delay(FlashDelayTime);
    pixels.setPixelColor(PixelNum, Off);
    pixels.show();
    delay(FlashDelayTime);
  }
}

//*******************************************************************************************//

void ResetRFIDReadVariables(){
    uidString = "0";
    for (byte i = 0; i < 3; i++) {
      rfid.uid.uidByte[i] = 00;
    }
}

//******************************************************************************************//

byte DecToHex(byte Dec){
  byte DecToHex[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
                      0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
                      0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
                      0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
                      0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
                      0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
                      0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
                      0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
                      0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
                      0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
                      0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
                      0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
                      0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
                      0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
                      0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
                      0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF}; 
  return DecToHex[Dec]; 
}

//*******************************************************************************************//
/*
void ManagerMode(){
  Serial.println(F("----------ManagerMode Activated----------"));
  while(1){
  delay(500);
  pixels.setPixelColor(NeoPixelNotify, Blue);
  pixels.show();
  //look for new cards
  if(rfid.PICC_IsNewCardPresent()) {
    readRFID();
    if(uidString != "0000"){  // ignore if it didn't read the card properly
      if(SD.exists(uidString + ".txt")){
        SD.remove(uidString + ".txt");
        LogToSD("Card removed with UID: " + uidString);
      }
      else{
        byte sector         = 1;
        byte blockAddr      = (4 * (sector + 1)) - 2;
        byte trailerBlock   = (4 * (sector + 1)) - 1;
        byte buffer[18];
        byte size = sizeof(buffer);
        // Authenticate using key B
        Serial.println(F("Authenticating again using key B..."));
        MFRC522::StatusCode StatusKeyB = (MFRC522::StatusCode) rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, trailerBlock, &keyB, &(rfid.uid));
        // Read data from the block
        Serial.println(F("Reading data from block ")); Serial.print(blockAddr);
        MFRC522::StatusCode StatusReadingBlock = (MFRC522::StatusCode) rfid.MIFARE_Read(blockAddr, buffer, &size);
        myFile=SD.open(uidString + ".txt", FILE_WRITE);
        // If the file opened ok, write to it
        if (myFile) {
          Serial.println( uidString + ".txt opened ok");
          Serial.println( "Block: " + String(sector) + " blockAddr: " + String(blockAddr));
          for (byte i = 0; i < 16; i++) {
            myFile.print(buffer[i] < 0x10 ? "0" : "");
            myFile.print(buffer[i], HEX);
            Serial.print(buffer[i] < 0x10 ? "0" : "");
            Serial.print(buffer[i], HEX);
          }; 
          Serial.println();
          myFile.println();
          Serial.println("sucessfully written to " + uidString + ".txt");
          myFile.close();
          Serial.println(uidString + ".txt closed");
          }
          else {
            Serial.println("error opening " + uidString + ".txt");
            ErrorCode(6);  
          }
        ResetRFIDReadVariables();
      }
    }
    delay(10);
    }
    if(Serial.available()){
      //Serial.print("You typed: " );
      //Serial.println(Serial.read());
      delay(500);
      Serial.println(F("----------ManagerMode De-Activated----------"));
      pixels.setPixelColor(NeoPixelNotify, Off);
      pixels.show();
      return;
    }
  }
  return;
}
*/
//******************************************************************************************//
