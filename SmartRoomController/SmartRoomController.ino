
/* 
 *  File:        SmartRoomController
 *  Description: Smart Device Scanner for Lights and Wemo
 *  Author:      Reed Searle
 *  Date:        7 Mar 2022
 */

#include <Wire.h>
#include <WireIMXRT.h>
#include <WireKinetis.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BME280.h>
#include <splash.h>
#include <Encoder.h>
#include <Ethernet.h>
#include <mac.h>
#include <hue.h>
#include <wemo.h>


// OLED screen constants
const int SCREENWIDTH      = 128;  //  Width of screen in pixels
const int SCREENHEIGHT     = 64;   //  Height of screen in pixels
const int OLEDRESET        = -1;   //  OLED Reset shared with Teensy
byte      OLEDADDRESS      = 0x3C; //  Address of the OLED on the IIC bus
const int MENUPOSITIONS[]  = {5, 15, 25, 35, 45};  // Starting positions for all menu options
const int MENUCOLOR[]      = {0,1,2}; // Set three colors to print, in order: BLACK, WHITE, INVERSE
String    MENUHOME[][8][5] = {
                               {
                                 {"SMART SCANNER",       "* Smart Lights", "* Smart Outlets",      "* IP Address",  ""          },
                                 {"SMART LIGHTS ",       "* Address",      "* Delay",              "* Fire",        "* Manual"  },
                                 {"SMART LIGHTS ADDR",   "String n (x-Y)", "* Previous String",    "* Next String", ""          },
                                 {"SMART LIGHTS FIRE1",  "Current String", "* Create New String",  "",              ""          },
                                 {"SMART LIGHTS FIRE2",  "* Total Lights", "* Addresses in order", "",              ""          },
                                 {"SMART LIGHTS MANUAL", "* Address",      "* Hue",                "* Turn ON",     "* Turn OFF"},
                                 {"SMART LIGHTS DELAY",  "* Address",      "* Initialize",         "* Run",         ""          },
                                 {"SMART OUTLETS ",      "* Address",      "* Turn ON",            "* Turn OFF",    "* All Off" }
                               },
                               {
                                 {0, 1, 7, 0, 0},  //  indicies for next menu level
                                 {0, 2, 6, 3, 5},  //  indicies for next menu level
                                 {1, 2, 2, 2, 2},  //  indicies for next menu level
                                 {1, 3, 4, 3, 3},  //  indicies for next menu level
                                 {3, 4, 4, 4, 4},  //  indicies for next menu level
                                 {1, 5, 5, 5, 5},  //  indicies for next menu level
                                 {1, 6, 6, 6, 6},  //  indicies for next menu level
                                 {0, 7, 7, 7, 7}   //  indicies for next menu level
                               }
                             };
                                // {0, 0,   0,  0,  0}  Case statement indicies for each menu level
                                // {0, 0,   0,  0,  0}  Case statement indicies for each menu level
                                // {0, 21, 22, 23,  0}  Case statement indicies for each menu level
                                // {0, 31, 32,  0,  0}  Case statement indicies for each menu level
                                // {0, 41, 42,  0,  0}  Case statement indicies for each menu level
                                // {0, 51, 52, 53, 54}  Case statement indicies for each menu level
                                // {0, 61, 62, 63,  0}  Case statement indicies for each menu level
                                // {0, 71, 72, 73,  0}  Case statement indicies for each menu level

// ENCODER Pins
const int ENCAPIN     = 22;  // Encoder A pin
const int ENCBPIN     = 21;  // Encoder B pin
const int BUTTONPIN   = 23;  // Encoder button

const int MONTH        = 9;
const int DAY          = 10;
const int YEAR         = 1966;
const int DELAYTIME = 250;

int status;

  // Encoder Variables
int  buttonPress;                      // Initial state is button NOT pressed
int  buttonPressed;                      // Initial state is buttonhas NOT been pressed
int  encPos;                      // Set up encoder wheel
int  encPosOld;
int  encMin;                      // Minimum position for encoder
int  encMax;                     // Maximun position for encoder addressing
int  menuMin;                      // Minimum position for menu
int  menuMax;                     // Maximun position for menu
int  indexMax;                     // Maximun position for menu
int  menuPos;                     // Cursor location within a menu
int  menuLevel;                     // Address of new menu
bool firstTime;                   // indicates a new menu level has been requested
//bool fallEdge;

int smartLightAdd;              // Address of the current smart light
int smartLightHue;              // Hue of the current smart light

int smartOutletAdd;              // Address of the current smart light
int maxOutlets;                    // Maximum number of smart outlets on network

int caseIndex;

int startTime;
int endTime;
int delayTime;

int i;

// Constructors
Adafruit_SSD1306  displayOne(SCREENWIDTH, SCREENHEIGHT, &Wire, OLEDRESET);
Encoder           encOne(ENCAPIN, ENCBPIN);

//TEST CODE
int wemoDevice = 3;  // fan on my desk

void setup() {

//Start Serial  
  Serial.begin(9600);
  while(!Serial);
  Serial.printf("Serial Port running\n");

// Start Ethernet
//  Ethernet.begin(mac);
//  printIP();
//  Serial.printf("LinkStatus: %i  \n",Ethernet.linkStatus());

// Start OLED Display
  if(!displayOne.begin(SSD1306_SWITCHCAPVCC, OLEDADDRESS)) {
    Serial.printf("OLED display did not initialize correctly.  Please reset.\n");
    while(1);  //  You shall not pass 
  }
  displayOne.setTextSize(1);  // Set text size
  Serial.printf("OLED display running\n");

  pinMode(BUTTONPIN, INPUT_PULLUP);

  buttonPress    = 0;                     // Initial state is button NOT pressed
  buttonPressed  = 0;                     // Initial state is buttonhas NOT been pressed
//  encPos         = 0;                     // Set up encoder wheel
//  encPosOld      = 0;
//  encMin         = 0;                     // Minimum position for encoder
//  encMax         = 35;                    // Maximun position for encoder addressing
//  menuMin        = 0;                     // Minimum position for menu
//  menuMax        = 4;                     // Maximun position for menu
  indexMax        = 4;                     // Maximun position for menu
  menuLevel      = 0;                     // Staerting menu is top level
  caseIndex = 0;
  firstTime      = 0;

  smartLightAdd  = 1;
  smartOutletAdd = 0;
  maxOutlets = 10;  // Arbitrarily set to 10
  
  endTime        = 0;
}

void loop() {

  switch (caseIndex) {
    case 3:
     caseIndex = 70;  // Redirect
     break;
     
    case 14:
     caseIndex = 50;  //  Redirect
     break;
     
    case 50:
      buttonPress = digitalRead(BUTTONPIN);
      if (buttonPress && buttonPress != buttonPressed) {
        while(buttonPress) { // Button is NOT pressed)
          menuPos = doEncoder(firstTime,35,4);    //  Hard code of max values for default case  
          encoderButton();
          displayOne.clearDisplay();//  Clear the display before going further
          baseText(5, menuPos);
          displayOne.setCursor(70,MENUPOSITIONS[1]);
          displayOne.printf("%i",smartLightAdd);
          displayOne.setCursor(70,MENUPOSITIONS[2]);
          displayOne.printf("%i",smartLightHue);
          displayOne.display(); // Force display 
          if (menuPos == 0) {
            firstTime = 0;
          }
        }
      }
      break;
    
    case 51:  // Input indiviual Smart Light address
      buttonPress = digitalRead(BUTTONPIN);
      if (buttonPress && buttonPress != buttonPressed) {
        while(buttonPress) { // Button is NOT pressed)
          smartLightAdd = doEncoder(0,95,95);    //  Hard code of max values for default case  
          displayOne.clearDisplay();//  Clear the display before going further
          baseText(5, 1);
          displayOne.setCursor(70,MENUPOSITIONS[1]);
          displayOne.printf("%i",smartLightAdd);
          displayOne.setCursor(70,MENUPOSITIONS[2]);
          displayOne.printf("%i",smartLightHue);
          displayOne.display(); // Force display 
          buttonPress = digitalRead(BUTTONPIN);
          Serial.printf("(50)Case Index = %i \n",caseIndex);
          caseIndex = 50;
        }
      }
    break;

     case 52:  // Input indiviual Smart Light Hue
      buttonPress = digitalRead(BUTTONPIN);
      if (buttonPress && buttonPress != buttonPressed) {
        while(buttonPress) { // Button is NOT pressed)
          smartLightHue = doEncoder(0,95,95);    //  Hard code of max values for default case  
          displayOne.clearDisplay();//  Clear the display before going further
          baseText(5, 2);
          displayOne.setCursor(70,MENUPOSITIONS[1]);
          displayOne.printf("%i",smartLightAdd);
          displayOne.setCursor(70,MENUPOSITIONS[2]);
          displayOne.printf("%i",smartLightHue);
          displayOne.display(); // Force display 
          buttonPress = digitalRead(BUTTONPIN);
          caseIndex = 50;
        }
      }
    break;

    
    case 53:  // Turn ON Smart light at individual address
      setHue(smartLightAdd,true,smartLightHue,255,255);
    Serial.println("case 53");

      caseIndex = 50;
      break;

    
    case 54:  // Turn OFF Smart light at individual address
      setHue(smartLightAdd,false,0,0,0);
    Serial.println("case 54");

      caseIndex = 50;
      break;

    
    case 70:
      buttonPress = digitalRead(BUTTONPIN);
      if (buttonPress && buttonPress != buttonPressed) {
        while(buttonPress) { // Button is NOT pressed)
          menuPos = doEncoder(firstTime,35,4);    //  Hard code of max values for default case  
          encoderButton();
          displayOne.clearDisplay();//  Clear the display before going further
          baseText(7, menuPos);
          displayOne.setCursor(70,MENUPOSITIONS[1]);
          displayOne.printf("%i",smartOutletAdd);
          displayOne.display(); // Force display 
          if (menuPos == 0) {
            firstTime = 0;
          }
        }
      }
      break;
    
    case 71:  // Input indiviual Smart outlet address
      buttonPress = digitalRead(BUTTONPIN);
      if (buttonPress && buttonPress != buttonPressed) {
        while(buttonPress) { // Button is NOT pressed)
          smartOutletAdd = doEncoder(0,95,95);    //  Hard code of max values for default case  
          displayOne.clearDisplay();//  Clear the display before going further
          baseText(7, 1);
          displayOne.setCursor(70,MENUPOSITIONS[1]);
          displayOne.printf("%i",smartOutletAdd);
          displayOne.display(); // Force display 
          buttonPress = digitalRead(BUTTONPIN);
          Serial.printf("(50)Case Index = %i \n",caseIndex);
          caseIndex = 70;
        }
      }
    break;

    case 72:  // Turn ON Smart outlet at individual address
      switchON(smartOutletAdd);
//    Serial.println("case 73");
      caseIndex = 70;
      break;

    
    case 73:  // Turn OFF Smart outlet at individual address
      switchOFF(smartOutletAdd);
//    Serial.println("case 73");
      caseIndex = 70;
      break;

    
    case 74:  // Turn OFF ALL Smart outlets
      for (i=0; i<=maxOutlets; i++) {
      switchOFF(i);
      }
//    Serial.println("case 74");
      caseIndex = 70;
      break;

    
    default:
      menuPos = doEncoder(firstTime,35,4);    //  Hard code of max values for default case  
    Serial.println("default");

      encoderButton();
          Serial.printf("(Default)Case Index = %i \n",caseIndex);
      displayOne.clearDisplay();//  Clear the display before going further
      baseText(menuLevel, menuPos);
      displayOne.display(); // Force display 
      if (menuPos == 0) {
        firstTime = 0;
      }
      break;
    
  }
}

void baseText(int menuLvl, int menuCursor){  // Set up the recurring text and graphics
  displayOne.drawRect(2,2,SCREENWIDTH-2,SCREENHEIGHT-2,MENUCOLOR[1]);

  for (i=0; i<= indexMax; i++) { // cycle through all possible menu positions
    if (i == menuCursor) {
      displayOne.setTextColor(MENUCOLOR[0],MENUCOLOR[1]);
    } else {
      displayOne.setTextColor(MENUCOLOR[1]);
    }
    displayOne.setCursor(7,MENUPOSITIONS[i]);
    displayOne.println(MENUHOME[0][menuLvl][i]);
  }
}

int doEncoder (bool newLevel, int encMax, int menuMax) {  //  check for encoder rotary movement
  int encPos;
  int menuPosition;
    encPos = encOne.read();
  if (encPos < 0) {                    // check if encoder has gone below minimum value
    encPos = 0;                        // Set position to minimum value
    encOne.write(0);                   // force encoder to minimum value
  }
  else if (encPos >= encMax) {
    encPos = encMax;                         // Set position to minimum value
    encOne.write(encMax);                    // force encoder to minimum value
  }

    menuPosition = map (encPos,encMin,encMax,menuMin,menuMax); //  increase the encoder position width to reduce bounce

  if (menuPosition < 0) {                    // check if encoder has gone below minimum value
    menuPosition = 0;                        // Set position to minimum value
  }
  else if (menuPosition >= menuMax) {
    menuPosition = menuMax;                         // Set position to minimum value
  }

  if (newLevel) {
    menuPosition = 0; 
    encOne.write(0);                   // force encoder to minimum value
  }
  
  return menuPosition;
}

void printIP() {
  Serial.printf("My IP address: ");
  for (byte thisByte = 0; thisByte < 3; thisByte++) {
    Serial.printf("%i.",Ethernet.localIP()[thisByte]);
  }
  Serial.printf("%i\n",Ethernet.localIP()[3]);
}

void  encoderButton() {
      buttonPress = digitalRead(BUTTONPIN);
      if (!buttonPress && buttonPress != buttonPressed) {
        caseIndex = ((menuLevel) * 10) + menuPos;  // setting case statement index;
//        if (MENUHOME[1][menuLevel][menuPos].toInt() < 9) { // "9" is used to indicate that work needs to be done, not menu function
          menuLevel = MENUHOME[1][menuLevel][menuPos].toInt();
//        }
        buttonPressed = buttonPress;
        firstTime = 1;  // Reset menu cursor to top position in the new menu
      } else if (buttonPress && buttonPress != buttonPressed) {
        buttonPressed = buttonPress;
      } 
}


// TESTCODE

//  startTime = millis();
//  delayTime = startTime - endTime;
//  if (delayTime > DELAYTIME) { // Delaying to avoid overrunning the display MIGHT REMOVE IN FUTURE
//    endTime = millis();
//  }

//  TRYING TO AVOID REFRESHING SCREEN ALOT
//  if (encPosOld == encPos) {  //  check if encoder has moved, if not, let calling function know
//    return null;
//  } else {                    // Indicate that encoder has moved and update position
//    encPosOld = encPos;                        // Set value of old to current for comparison
//    return menuPosition;
//  }

//      Serial.printf("Menu Level = %i menu Pos %i\n",menuLevel, menuPos);
//      Serial.printf("Case index = %i \n",caseIndex);
