/* Error Codes
 *  3 - Failed to initialise SD Module
*   4 - failed to write to Log 
*   5 - Could not find RTC Module
*   6 - error opening "uidString".txt
*   7 - Could not find RC522 Module
*   8 - Could not add new card
*/

#include <Wire.h>                 // RTC Module
#include <RTClib.h>               // RTC Module
#include <Adafruit_NeoPixel.h>    // NeoPixel Strip
#include <SPI.h>                  // for the RFID and SD card module
#include <SD.h>                   // for the SD card
#include <MFRC522.h>              // for the RFID

#define AccessGrantedTime 3000    // Time to keep the door unlocked for in milliseconds
#define AccessDeniedTime  1000    // Time to wait after an incorrect unlock attempt in milliseconds
// PINS
#define NeoPixelPin     2 // Which pin on the Arduino is connected to the NeoPixels?
#define relay           3 // Set Relay Pin
#define CS_SD           4 // define select pin for SD card module
#define prog            6 // Button pin for Programming Mode
#define CS_RFID         8 // define pins for RFID
#define RST_RFID        9 // define pins for RFID

#define NumNeoPixels    1 // How many NeoPixels are attached to the Arduino?
#define NeoPixelNotify  0 // Determine which LED to use as a notification LED

byte    sector  =       2;// which sector to read on the RFID card
String  LogFile = "LOG.TXT";      // name of Log File
String  ACBits  = " 0f 00 ff 69"; // AC bits the cards must have
String  uidString;        // Variable to hold the tag's UID
File    myFile;       // Create a file to store the data
MFRC522::MIFARE_Key keyA = {keyByte: {0x35, 0x35, 0x35, 0x32, 0x39, 0x34}};
MFRC522::MIFARE_Key keyB = {keyByte: {0x00, 0x00, 0xC8, 0xFF, 0x7F, 0x00}};

RTC_DS1307 rtc; 
MFRC522 rfid(CS_RFID, RST_RFID);
Adafruit_NeoPixel pixels(NumNeoPixels, NeoPixelPin, NEO_GRB + NEO_KHZ800);
static uint32_t Red   = pixels.Color(255,   0,   0);  // Setting easy name for common colours for the LED's 
static uint32_t Green = pixels.Color(  0, 255,   0);  // use pixels.setPixelColor(LED#, Color); to set a LED color (Red/ Green/ Blue/ Off)
static uint32_t Blue  = pixels.Color(  0,   0, 255);  // pixels.clear(); will set all LEDs to off
static uint32_t Off   = pixels.Color(  0,   0,   0);  // and then use pixels.show();


//*****************************************************************************************//

void setup() {  
  randomSeed(analogRead(0));  // used to generate sudo random numbers
  pinMode(relay, OUTPUT);
  digitalWrite(relay, LOW);   // Make sure door is locked
  pinMode(prog, INPUT_PULLUP);   // Enable pin's pull up resistor
  Serial.begin(2000000);         // Init Serial port
  // while(!Serial);          // Wait for computer serial box
  // Setup NeoPixels
  pixels.begin();             // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.setBrightness(80);   // Set BRIGHTNESS (max = 255)
  pixels.clear();             // Set all pixel colors to 'off'
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
  // Setup SD card module
  if(!SD.begin(CS_SD)) {
    Serial.println(F("Initializing SD card failed!"));
    ErrorCode(3);  
  }
  rfid.PCD_Init(); // Init MFRC522
  // Setup RFID module
  rfid.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  if (rfid.PCD_PerformSelfTest() != 1){
      Serial.println(F("Couldn't find RC522"));
      ErrorCode(7);
    }
  //rfid.PCD_SetAntennaGain(rfid.RxGain_max); // increases the range of the RFID module
  
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
  FlashNeoPixel(NeoPixelNotify, 1, 250, Red);
  FlashNeoPixel(NeoPixelNotify, 1, 250, Blue);
  FlashNeoPixel(NeoPixelNotify, 1, 250, Green);
}

//*****************************************************************************************//

void loop() {
  //look for new cards
  Serial.println(rfid.PICC_IsNewCardPresent());
  if(rfid.PICC_IsNewCardPresent()) {
    readRFID();
    if(uidString != "00000000"){  // ignore if it didn't read the card properly
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
  uidString = "";
  rfid.PICC_ReadCardSerial();
  Serial.print(F("Card with uid detected: "));
  for (byte i = 0; i < 4; i++) {
        uidString += (rfid.uid.uidByte[i] < 0x10 ? "0" : "") + String(rfid.uid.uidByte[i],HEX);
    }
  Serial.println(uidString);
  LogToSD("Card with uid detected: " + uidString);
  delay(100);
}

//*****************************************************************************************//

int verifyRFIDCard(){
  if(SD.exists(uidString + ".txt") && digitalRead(prog) == LOW){
    Serial.println(F("Deleting card - Programming Mode"));
    SD.remove(uidString + ".txt");
    return(2);
  }
  //check UUID
  if(!SD.exists(uidString + ".txt")){
    Serial.println(F("UID not recognized"));
    if (digitalRead(prog) != LOW) {
        return(2);
     }else{
      Serial.println(F("Adding card - Programming Mode"));
      myFile=SD.open(uidString + ".txt",FILE_WRITE);
      myFile.close();
     }
  }
  
  
    //MFRC522::StatusCode Status;
    byte blockAddr      = (4 * (sector + 1)) - 2;
    byte trailerBlock   = (4 * (sector + 1)) - 1;
    byte buffer[18];
    for (byte i = 0; i < 16; i++) {
        buffer[i] = 0x00; 
    }
    byte size = sizeof(buffer);
  

    // Authenticate using key A
    //Serial.println(F("Authenticating using key A... "));
    MFRC522::StatusCode StatusKeyA = (MFRC522::StatusCode) rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &keyA, &(rfid.uid));
    // Authenticate using key B
    //Serial.println(F("Authenticating using key B... "));
    MFRC522::StatusCode StatusKeyB = (MFRC522::StatusCode) rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, trailerBlock, &keyB, &(rfid.uid));
    //Read AC bits
    //Serial.println(F("Authenticating using AC bits... "));
    MFRC522::StatusCode StatusAC = (MFRC522::StatusCode) rfid.MIFARE_Read(trailerBlock, buffer, &size);
    String ACString;
    for (byte i = 6; i < 10; i++) {
        ACString += (buffer[i] < 0x10 ? " 0" : " ") + String(buffer[i],HEX);
    }
    // Read card data
    Serial.println(F("Reading card data..."));
    MFRC522::StatusCode StatusReadData = (MFRC522::StatusCode) rfid.MIFARE_Read(blockAddr, buffer, &size);
    String CardData;
    for (byte i = 0; i < 16; i++) {
        CardData += (buffer[i] < 0x10 ? "0" : "") + String(buffer[i],HEX);
    }
    
    // Check Keys % AC
    if (StatusKeyA != MFRC522::STATUS_OK || StatusKeyB != MFRC522::STATUS_OK || StatusAC != MFRC522::STATUS_OK || ACString != ACBits) {
      Serial.println(F("Keys or AC not recognized"));
      return(3);
    }
    
    
    Serial.println(CardData);
    //Serial.println(F("Reading saved data..."));
    String SavedData = "";
    // Open file
    myFile=SD.open(uidString + ".txt");
    // If the file opened ok, read it
    if (myFile) {
      while (myFile.available()) {
      //Serial.write(myFile.read());
      SavedData += (char)myFile.read();
      }
      myFile.close();
      Serial.println(SavedData);
    }else {
      Serial.println("error opening " + uidString + ".txt");
      ErrorCode(6);  
    }
    if (StatusReadData != MFRC522::STATUS_OK || CardData != SavedData) {
      Serial.println(F("Reading card data failed or card data is inconsistent"));
      if (digitalRead(prog) != LOW) {
        return(6);
      }else{
        Serial.println(F("Adding card - Programming Mode"));
      }
    }
    
    
    // Write data to the block
    Serial.print(F("Writing data into block ")); Serial.println(blockAddr);
    String StringDataToWrite = "";
    byte DataToWrite[16];
    for (byte i = 0; i < 16; i++) {
        //DataToWrite[i] = DecToHex(random(0, 255));
        //DataToWrite[i] = 0x01;
        DataToWrite[i] = random(0, 255);
        if(DataToWrite[i] < 0x10){
          StringDataToWrite += "0";
        }
        StringDataToWrite += String(DataToWrite[i], HEX);
    }; 
    Serial.println(StringDataToWrite);
    MFRC522::StatusCode StatusWriteData = (MFRC522::StatusCode) rfid.MIFARE_Write(blockAddr, DataToWrite, 16);
    if (StatusWriteData != MFRC522::STATUS_OK) {
        Serial.print(F("Writing tag failed: "));
        Serial.println(rfid.GetStatusCodeName(StatusWriteData));
        return(4);
    }
    /*
    // reread card data
    Serial.println(F("Re-Reading card data..."));
    Status = (MFRC522::StatusCode) rfid.MIFARE_Read(blockAddr, buffer, &size);
    CardData = "";
    for (byte i = 0; i < 16; i++) {
        CardData += (buffer[i] < 0x10 ? "0" : "") + String(buffer[i],HEX);
    }
    Serial.println(CardData);
    if (Status != MFRC522::STATUS_OK || CardData != StringDataToWrite) {
      Serial.println(F("Could not verify card"));
      return(7);
    }*/
    
    Serial.println(F("Writing data to SD..."));
    SD.remove(uidString + ".txt");
    //Serial.println("Card removed with UID: " + uidString);
    // Open file
    myFile=SD.open(uidString + ".txt", FILE_WRITE);
    if (myFile) {
      myFile.print(StringDataToWrite);
      myFile.close();
    }else {
      Serial.println("error opening " + uidString + ".txt");
      ErrorCode(6);  
    }
    
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
  digitalWrite(relay, HIGH); // activate relay
  Serial.println(F("Access Granted"));
  LogToSD("Access Granted to card: " + uidString);
  delay(TimeDoorOpen);
  digitalWrite(relay, LOW); // de-activate relay
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
    for (byte i = 0; i < 4; i++) {
      rfid.uid.uidByte[i] = 00;
    }
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
