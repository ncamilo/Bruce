#include "globals.h"

#include <EEPROM.h>
#include <iostream>
#include <functional>
#include <vector>
#include <string>

// Public Globals Variables
int prog_handler;    // 0 - Flash, 1 - SPIFFS, 3 - Download
int rotation;
bool sdcardMounted;
bool wifiConnected;
bool returnToMenu;
String ssid;
String pwd;
std::vector<std::pair<std::string, std::function<void()>>> options;
const int bufSize = 4096;
uint8_t buff[4096] = {0};
// Protected global variables
TFT_eSPI tft = TFT_eSPI();         // Invoke custom library
TFT_eSprite sprite = TFT_eSprite(&tft);
TFT_eSprite menu_op = TFT_eSprite(&tft);
TFT_eSprite draw = TFT_eSprite(&tft);

#if defined(CARDPUTER)
  Keyboard_Class Keyboard = Keyboard_Class();
#elif defined (STICK_C_PLUS)
  AXP192 axp192;
#endif

#include "mykeyboard.h"
#include "display.h"
#include "webInterface.h"
#include "sd_functions.h"
#include "wifi_common.h"
#include "settings.h"


/*********************************************************************
**  Function: setup                                    
**  Where the devices are started and variables set    
*********************************************************************/
void setup() {
  Serial.begin(115200);

  // declare variables
  prog_handler=0;
  sdcardMounted=false;
  wifiConnected=false;

  // Setup GPIOs and stuff
  #if  defined(STICK_C_PLUS2)
    pinMode(UP_BTN, INPUT);   // Sets the power btn as an INPUT
  #elif defined(STICK_C_PLUS)
    axp192.begin();           // Start the energy management of AXP192
  #endif
  
 
  #ifndef CARDPUTER
  pinMode(SEL_BTN, INPUT);  
  pinMode(DW_BTN, INPUT);
  pinMode(4, OUTPUT);     // Keeps the Stick alive after take off the USB cable
  digitalWrite(4,HIGH);   // Keeps the Stick alive after take off the USB cable
  #else
  Keyboard.begin();
  pinMode(10, INPUT);     // Pin that reads the 
  #endif

  tft.init();
  rotation = gsetRotation();
  tft.setRotation(rotation);
  resetTftDisplay();

  #if defined(BACKLIGHT)
  pinMode(BACKLIGHT, OUTPUT);
  #endif

  getBrightness();  
  sprite.createSprite(WIDTH-15,HEIGHT-15);
  //Start Bootscreen timer
  int i = millis();
  while(millis()<i+5000) { // boot image lasts for 5 secs
    initDisplay();         // show boot screen
  
  #if defined (CARDPUTER)   // If any key is pressed, it'll jump the boot screen
    Keyboard.update();
    if(Keyboard.isPressed())
  #else
    if(digitalRead(SEL_BTN)==LOW)  // If M5 key is pressed, it'll jump the boot screen
  #endif
     {
        tft.fillScreen(TFT_BLACK);
        delay(10);
        goto Program;
      }
  }
  
  // If M5 or Enter button is pressed, continue from here
  Program:
  delay(200);

}

/**********************************************************************
**  Function: loop                                     
**  Main loop                                          
**********************************************************************/
void loop() {
  bool redraw = true;
  int index = 0;
  int opt = 6; // there are 3 options> 1 list SD files, 2 OTA and 3 Config
  tft.fillRect(0,0,WIDTH,HEIGHT,BGCOLOR);
  if(!setupSdCard()) index=1; //if SD card is not present, paint SD square grey and auto select OTA
  while(1){
    returnToMenu = false;

    if (redraw) { 
      drawMainMenu(index); 
      redraw = false; 
      delay(200); 
    }

    if(checkPrevPress()) {
      if(index==0) index = opt - 1;
      else if(index>0) index--;
      redraw = true;
    }
    /* DW Btn to next item */
    if(checkNextPress()) { 
      index++;
      if((index+1)>opt) index = 0;
      redraw = true;
    }

    /* Select and run function */
    if(checkSelPress()) { 
      switch(index) {
        case 0:   // WiFi
          if(!wifiConnected) {
            options = {
              {"Connect Wifi", [=]()  { wifiConnectMenu(); }},    //wifi_common.h
              {"WiFi AP", [=]()       { wifiConnectMenu(true); }},//wifi_common.h
            };
          } else {
            options = {
              {"Disconnect Wifi", [=]()  { wifiDisconnect(); }},    //wifi_common.h
            };
          }
          options.push_back({"Wifi Atks", [=]()     { displayRedStripe("Wifi Atks"); }});
          options.push_back({"TelNET", [=]()        { displayRedStripe("TelNET"); }});
          options.push_back({"SSH", [=]()           { displayRedStripe("SSH"); }});
          options.push_back({"Raw Sniffer", [=]()   { displayRedStripe("Raw Sniffer"); }});
          options.push_back({"DOWO-ESP32", [=]()    { displayRedStripe("DOWO-ESP32"); }});
          options.push_back({"Evil Portal", [=]()   { displayRedStripe("Evil Portal"); }});
          options.push_back({"ARP Scan", [=]()      { displayRedStripe("ARP Scan"); }});
          options.push_back({"Wireguard Tun", [=]() { displayRedStripe("Wireguard Tun"); }});
          options.push_back({"Main Menu", [=]()     { displayRedStripe("Main Menu"); }});
          delay(200);
          loopOptions(options,false,true,"WiFi");
          delay(1000); // remover depois, está aqui só por causa do "displayRedStripe"
          break;
        case 1: // BLE
          options = {
            {"AppleJuice", [=]()   { displayRedStripe("AppleJuice"); }},
            {"SwiftPair", [=]()    { displayRedStripe("SwiftPair"); }},
            {"Android Spam", [=]() { displayRedStripe("Android Spam"); }},
            {"SourApple", [=]()    { displayRedStripe("SourApple"); }},
            {"BT Maelstrom", [=]() { displayRedStripe("BT Maelstrom"); }},
            {"Main Menu", [=]()    { displayRedStripe("Main Menu"); }},
          };
          delay(200);
          loopOptions(options,false,true,"Bluetooth");
          delay(1000); // remover depois, está aqui só por causa do "displayRedStripe"
          break;
        case 2: // RF
          options = {
            {"Scan/copy", [=]()   { displayRedStripe("Scan/copy"); }},
            {"Replay", [=]()      { displayRedStripe("Replay"); }},
            {"Spectrum", [=]()    { displayRedStripe("Spectrum"); }},
            {"Main Menu", [=]()   { displayRedStripe("Main Menu"); }},
          };
          delay(200);
          loopOptions(options,false,true,"Radio Frequency");
          delay(1000); // remover depois, está aqui só por causa do "displayRedStripe"
          break;
        case 3: // RFID
          options = {
            {"Scan/copy", [=]()   { displayRedStripe("Scan/copy"); }},
            {"Replay", [=]()      { displayRedStripe("Replay"); }},
            {"Main Menu", [=]()   { displayRedStripe("Main Menu"); }},
          };
          delay(200);
          loopOptions(options,false,true,"RFID");
          delay(1000); // remover depois, está aqui só por causa do "displayRedStripe"
          break;
        case 4: //Other
          options = {
            {"InfraRed", [=]()  { displayRedStripe("InfraRed"); }},
            {"BadUSB", [=]()    { displayRedStripe("BadUSB"); }},
            {"SD Card", [=]()   { loopSD(); }},
            {"WebUI", [=]()     { loopOptionsWebUi(); }},
            {"Main Menu", [=]() { displayRedStripe("Main Menu"); }},
          };
          delay(200);
          loopOptions(options,false,true,"Others");
          delay(1000); // remover depois, está aqui só por causa do "displayRedStripe"
          break;
        case 5: //Config
          options = {
            {"Brightness", [=]()  { setBrightnessMenu(); }},          //settings.h
            {"Orientation", [=]() { gsetRotation(true); }},               //settings.h
            {"Main Menu", [=]()   { displayRedStripe("Main Menu"); }},
            {"Restart", [=]()     { ESP.restart(); }},
          };
          delay(200);
          loopOptions(options,false,true,"Config");
          delay(1000); // remover depois, está aqui só por causa do "displayRedStripe"
          break;
      }
      redraw=true;
    }
  }
}
