
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
                                 {"SMART SCANNER",       "* Smart Lights", "* Smart Outlets",      "* IP Address",  ""           },
                                 {"SMART LIGHTS ",       "* Address",      "* Delay",              "* Fire",        "* Manual"   },
                                 {"SMART LIGHTS ADDR",   "",               "* Previous String",    "* Next String", "FLASH"      },
                                 {"SMART LIGHTS FIRE",   "FIRE String",    "* Create New String",  "* Test",        ""           },
                                 {"SMART LIGHTS FIRE2",  "* Total Lights", "* Addresses in order", "",              ""           },
                                 {"SMART LIGHTS MANUAL", "* Address",      "* Hue",                "* Turn ON",     "* Turn OFF" },
                                 {"SMART LIGHTS DELAY",  "* Address",      "* Initialize",         "* Run Delay",   ""           },
                                 {"SMART OUTLETS ",      "* Address",      "* Turn ON",            "* Turn OFF",    "* All Off"  }
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
//                                 {"SMART LIGHTS ADDR",   "String n (x-Y)", "* Previous String",    "* Next String", "FLASH"     },
//                                 {"SMART LIGHTS FIRE1",  "Current String", "* Create New String",  "",              ""          },
//                                 {"SMART LIGHTS FIRE2",  "* Total Lights", "* Addresses in order", "",              ""          },
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

// PHOTOSENSOR Pin
const int SENSORPIN   = 20;  // Encoder button

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

// Photosensor variables
int lightIn;

int smartLightAdd;              // Address of the current smart light
int smartLightHue;              // Hue of the current smart light
int lightBatch;
int startBatch;
int fireBatch[3];

int smartOutletAdd;              // Address of the current smart light
int maxOutlets;                    // Maximum number of smart outlets on network

int caseIndex;
int caseState;

//TIMER Variables
int startTime;
int endTime;
int delayTime;

// Smart Light delay variables
int delayStart;
int delayStop;
int deltaDelay;

int i,j;

// Constructors
Adafruit_SSD1306  displayOne(SCREENWIDTH, SCREENHEIGHT, &Wire, OLEDRESET);
Encoder           encOne(ENCAPIN, ENCBPIN);

//TEST CODE
int wemoDevice = 3;  // fan on my desk

// *******************  SETUP  *********************** //
void setup() {

//Start Serial  
  Serial.begin(9600);
//  while(!Serial);
  Serial.printf("Serial Port running\n");

// Start Ethernet
  Ethernet.begin(mac);
  printIP();
  Serial.printf("LinkStatus: %i  \n",Ethernet.linkStatus());

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
  caseState = 0;
  firstTime      = 0;

  smartLightAdd  = 1;
  smartOutletAdd = 0;
  lightBatch = 1;
  fireBatch[0] = {1};
  fireBatch[1] = {2};
  fireBatch[2] = {3};
  
  maxOutlets = 10;  // Arbitrarily set to 10
  
  endTime        = 0;
}

// *******************  LOOP  *********************** //
void loop() {

  switch (caseIndex) {
    case 0:  // Home State
      menuPos = doEncoder(firstTime,35,4);    //  Hard code of max values for default case
      menuLevel = 0;  
      encoderButton();
  Serial.printf("(0)Case Index = %i \n",caseIndex);
      displayOne.clearDisplay();//  Clear the display before going further
      baseText(menuLevel, menuPos);
      displayOne.display(); // Force display 
      if (menuPos == 0) {
        firstTime = 0;
      }
      break;


//  SMART LIGHTS
    case 10:          //  Redirect up one level. 
     caseIndex = 0;  //  Redirect
     break;
     
    case 1:
      buttonPress = digitalRead(BUTTONPIN);
      menuLevel = 1;
      if (buttonPress && buttonPress != buttonPressed) {
        while(buttonPress) { // Button is NOT pressed)
          menuPos = doEncoder(firstTime,35,4);    //  Hard code of max values for default case  
          encoderButton();
          displayOne.clearDisplay();//  Clear the display before going further
          baseText(menuLevel, menuPos);
          displayOne.display(); // Force display 
          if (menuPos == 0) {
            firstTime = 0;
          }
        }
      }
      break;

//  SMART LIGHTS ADDRESSING - FLASHING FOR EFFECT
    case 20:          //  Redirect up one level.  This case does nothing
     caseIndex = 1;  //  Redirect
     break;
     
    case 11:
      buttonPress = digitalRead(BUTTONPIN);
      menuLevel = 2;
      if (buttonPress && buttonPress != buttonPressed) {
        while(buttonPress) { // Button is NOT pressed)
          menuPos = doEncoder(firstTime,35,4);    //  Hard code of max values for default case  
          encoderButton();
          startBatch = 3*(lightBatch-1)+1;
          displayOne.clearDisplay();//  Clear the display before going further
          baseText(menuLevel, menuPos);
          displayOne.setCursor(7,MENUPOSITIONS[1]);
          displayOne.printf("String %i (%i-%i)",lightBatch, startBatch, startBatch+2);
          displayOne.display(); // Force display 
          if (menuPos == 0) {
            firstTime = 0;
          }
        }
      }
      break;

    case 21:          //  Redirect up one level.  This case does nothing
     caseIndex = 11;  //  Redirect
     break;
     
    case 22:          //  Redirect up one level.  This case does nothing
      if (lightBatch > 1) {
        lightBatch--;
      } else {
        lightBatch = 1;
      }
      caseIndex = 11;  //  Redirect
      break;
     
    case 23:          //  Redirect up one level.  This case does nothing
      lightBatch++;
      caseIndex = 11;  //  Redirect
      break;
     
    case 24:          //  Redirect up one level.  This case does nothing
      for(i=0; i<=5; i++) {  //flash for 10 times
        for (j=0; j<=2; j++) { // flash the string
          setHue(startBatch +j,true,HueRed,255,255);
        }
        delay(500);
          setHue(startBatch ,   true,HueYellow,255,255);
          setHue(startBatch +1, true,HueGreen,255,255);
          setHue(startBatch +2, true,HueBlue,255,255);
        delay(500);
      }
        for (j=0; j<=2; j++) { // flash the string
          setHue(startBatch +j,false,HueRed,255,255);
        }
     caseIndex = 11;  //  Redirect
     break;
     
//  SMART LIGHTS FIRE - VIEW, EDIT, AND TEST STRINGS
    case 30:          //  Redirect up one level. 
     caseIndex = 1;  //  Redirect
     break;
     
    case 13:
      buttonPress = digitalRead(BUTTONPIN);
      menuLevel = 3;
      if (buttonPress && buttonPress != buttonPressed) {
        while(buttonPress) { // Button is NOT pressed)
          menuPos = doEncoder(firstTime,35,4);    //  Hard code of max values for default case  
          encoderButton();
          displayOne.clearDisplay();//  Clear the display before going further
          baseText(menuLevel, menuPos);
          displayOne.setCursor(80,MENUPOSITIONS[1]);
          displayOne.printf("%i-%i-%i",fireBatch[0], fireBatch[1], fireBatch[2]);
          displayOne.display(); // Force display 
          if (menuPos == 0) {
            firstTime = 0;
          }
        }
      }
      break;

    case 31:          //  Redirect up one level.  This case does nothing
     caseIndex = 13;  //  Redirect
     break;
     
    case 32:          //  Redirect up one level.  This case is covered below
      j = 0;
      while (j<=2) {  //  go through all three addresses
      buttonPress = digitalRead(BUTTONPIN);
      if (buttonPress && buttonPress != buttonPressed) {
          while(buttonPress) { // Button is NOT pressed)
            fireBatch[j] = doEncoder(0,95,95);    //  Hard code of max values for default case  
            displayOne.clearDisplay();//  Clear the display before going further
            baseText(menuLevel, 5);
            displayOne.setCursor(80,MENUPOSITIONS[1]);
            displayOne.printf("%i-%i-%i",fireBatch[0], fireBatch[1], fireBatch[2]);
            displayOne.display(); // Force display 
            buttonPress = digitalRead(BUTTONPIN);
            Serial.printf("(50)Case Index = %i \n",caseIndex);
            caseIndex = 13;
          }
        Serial.printf("i %i firebatch %i\n",j, fireBatch[j]);
          j++;
        }
      }
     break;
     
    case 33:          //  Redirect up one level.  This case does nothing
      for(i=0; i<=1; i++) {  //flash for 10 times
        for (j=0; j<=2; j++) { // flash the string
          setHue(fireBatch[j],true,HueRed,255,255);
          delay(500);
        }
        for (j=0; j<=2; j++) { // flash the string
          setHue(fireBatch[j],false,HueRed,255,255);
          delay(500);
        }
        delay(1000);
      }
     caseIndex = 13;  //  Redirect
     break;
     
    case 34:          //  Redirect up one level.  This case does nothing
     caseIndex = 13;  //  Redirect
     break;
     
//  SMART LIGHTS FIRE - SET UP STRINGS ?? NOT USED ANYMORE ??
//    case 40:          //  Redirect up one level. 
//     caseIndex = 13;  //  Redirect
//     break;
//     
//    case 32:
//      buttonPress = digitalRead(BUTTONPIN);
//      menuLevel = 4;
//      if (buttonPress && buttonPress != buttonPressed) {
//        while(buttonPress) { // Button is NOT pressed)
//          menuPos = doEncoder(firstTime,35,4);    //  Hard code of max values for default case  
//          encoderButton();
//          displayOne.clearDisplay();//  Clear the display before going further
//          baseText(menuLevel, menuPos);
//          displayOne.display(); // Force display 
//          if (menuPos == 0) {
//            firstTime = 0;
//          }
//        }
//      }
//      break;
//
//    case 41:          //  Redirect up one level.  This case does nothing
//     caseIndex = 32;  //  Redirect
//     break;
//     
//    case 42:          //  Redirect up one level.  This case does nothing
//     caseIndex = 32;  //  Redirect
//     break;
//     
//    case 43:          //  Redirect up one level.  This case does nothing
//     caseIndex = 32;  //  Redirect
//     break;
//     
//    case 44:          //  Redirect up one level.  This case does nothing
//     caseIndex = 32;  //  Redirect
//     break;
     
//  SMART LIGHT MANUAL CONTROL
    case 50:          //  Redirect up one level
     caseIndex = 1;  //  Redirect
     break;

    case 14:
      buttonPress = digitalRead(BUTTONPIN);
      menuLevel = 5;
      if (buttonPress && buttonPress != buttonPressed) {
        while(buttonPress) { // Button is NOT pressed)
          menuPos = doEncoder(firstTime,35,4);    //  Hard code of max values for default case  
          encoderButton();
          displayOne.clearDisplay();//  Clear the display before going further
          baseText(menuLevel, menuPos);
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

//  SMART LIGHT MANUAL CONTROL - ADDRESS
    case 51:  // Input indiviual Smart Light address
      buttonPress = digitalRead(BUTTONPIN);
      if (buttonPress && buttonPress != buttonPressed) {
        while(buttonPress) { // Button is NOT pressed)
          smartLightAdd = doEncoder(0,95,95);    //  Hard code of max values for default case  
          displayOne.clearDisplay();//  Clear the display before going further
          baseText(menuLevel, 1);
          displayOne.setCursor(70,MENUPOSITIONS[1]);
          displayOne.printf("%i",smartLightAdd);
          displayOne.setCursor(70,MENUPOSITIONS[2]);
          displayOne.printf("%i",smartLightHue);
          displayOne.display(); // Force display 
          buttonPress = digitalRead(BUTTONPIN);
          Serial.printf("(50)Case Index = %i \n",caseIndex);
          caseIndex = 14;
        }
      }
    break;

//  SMART LIGHT MANUAL CONTROL - HUE
     case 52:  // Input indiviual Smart Light Hue
      buttonPress = digitalRead(BUTTONPIN);
      if (buttonPress && buttonPress != buttonPressed) {
        while(buttonPress) { // Button is NOT pressed)
          smartLightHue = doEncoder(0,65353 ,65353);    //  Hard code of max values for default case  
          displayOne.clearDisplay();//  Clear the display before going further
          baseText(menuLevel, 2);
          displayOne.setCursor(70,MENUPOSITIONS[1]);
          displayOne.printf("%i",smartLightAdd);
          displayOne.setCursor(70,MENUPOSITIONS[2]);
          displayOne.printf("%i",smartLightHue);
          displayOne.display(); // Force display 
          buttonPress = digitalRead(BUTTONPIN);
          caseIndex = 14;
        }
      }
    break;

    
//  SMART LIGHT MANUAL CONTROL - TURN ON
    case 53:  // Turn ON Smart light at individual address
      setHue(smartLightAdd,true,smartLightHue,255,255);
      caseIndex = 14;
      break;

    
//  SMART LIGHT MANUAL CONTROL - TURN OFF
    case 54:  // Turn OFF Smart light at individual address
      setHue(smartLightAdd,false,0,0,0);
      caseIndex = 14;
      break;


//  SMART LIGHT DELAY
    case 60:
     caseIndex = 1;  //  Redirect
     break;
     
    case 12:
      buttonPress = digitalRead(BUTTONPIN);
      menuLevel = 6;
      if (buttonPress && buttonPress != buttonPressed) {
        while(buttonPress) { // Button is NOT pressed)
          menuPos = doEncoder(firstTime,35,4);    //  Hard code of max values for default case  
          encoderButton();
          displayOne.clearDisplay();//  Clear the display before going further
          baseText(menuLevel, menuPos);
          displayOne.setCursor(80,MENUPOSITIONS[1]);
          displayOne.printf("%i",smartLightAdd);
          displayOne.setCursor(80,MENUPOSITIONS[3]);
          displayOne.printf("%i",deltaDelay);
          displayOne.display(); // Force display 
          if (menuPos == 0) {
            firstTime = 0;
          }
        }
      }
      break;
    
//  SMART LIGHT DELAY - ADDRESS
    case 61:  // Input indiviual Smart Light address
      buttonPress = digitalRead(BUTTONPIN);
      if (buttonPress && buttonPress != buttonPressed) {
        while(buttonPress) { // Button is NOT pressed)
          smartLightAdd = doEncoder(0,95,95);    //  Hard code of max values for default case  
          displayOne.clearDisplay();//  Clear the display before going further
          baseText(menuLevel, 1);
          displayOne.setCursor(80,MENUPOSITIONS[1]);
          displayOne.printf("%i",smartLightAdd);
          displayOne.display(); // Force display 
          buttonPress = digitalRead(BUTTONPIN);
          Serial.printf("(61)Case Index = %i \n",caseIndex);
          caseIndex = 12;
        }
      }
    break;

//  SMART LIGHT DELAY - INITIALIZE (turn off to start)
    case 62:  // Turn OFF Smart light at individual address
      setHue(smartLightAdd,false,0,0,0);
      caseIndex = 12;
      Serial.println("case 62");
      break;

//  SMART LIGHT DELAY - RUN (turn on, measure delay, turn offf, measure delay)
    case 63:  // Turn ON Smart light at individual address
      setHue(smartLightAdd,true,0x0000,255,255);  // turn on light with white color
      delayStop = millis();  // Set up delay counter
      delayStart = millis();
      lightIn = 0;           // Initialize light sensor
      while(lightIn < 100 && delayStart - delayStop <= 10000) {  // Loop while not seeing light and 10 sec timer
        lightIn = analogRead(SENSORPIN);
        delayStart = millis();
      }
      deltaDelay = delayStart - delayStop;
//    Serial.println("case 53");
      caseIndex = 12;
      break;

    
    case 64:  // empty menu index, redirect
      caseIndex = 12;
      break;

//  SMART OULTETS 
    case 70:
    caseIndex = 0;  // Redirect up one menu level
    break;
     


   
//  SMART OUTLET MANUAL CONTROL
    case 2:
      buttonPress = digitalRead(BUTTONPIN);
      menuLevel = 7;
      if (buttonPress && buttonPress != buttonPressed) {
        while(buttonPress) { // Button is NOT pressed)
          menuPos = doEncoder(firstTime,35,4);    //  Hard code of max values for default case  
          encoderButton();
          displayOne.clearDisplay();//  Clear the display before going further
          baseText(menuLevel, menuPos);
          displayOne.setCursor(70,MENUPOSITIONS[1]);
          displayOne.printf("%i",smartOutletAdd);
          displayOne.display(); // Force display 
          if (menuPos == 0) {
            firstTime = 0;
          }
        }
      }
          Serial.printf("(2)Case Index = %i \n",caseIndex);
      break;
    
//  SMART OUTLET MANUAL CONTROL - ADDRESS
    case 71:  // Input indiviual Smart outlet address
      buttonPress = digitalRead(BUTTONPIN);
      if (buttonPress && buttonPress != buttonPressed) {
        while(buttonPress) { // Button is NOT pressed)
          smartOutletAdd = doEncoder(0,95,95);    //  Hard code of max values for default case  
          displayOne.clearDisplay();//  Clear the display before going further
          baseText(menuLevel, 1);
          displayOne.setCursor(70,MENUPOSITIONS[1]);
          displayOne.printf("%i",smartOutletAdd);
          displayOne.display(); // Force display 
          buttonPress = digitalRead(BUTTONPIN);
          Serial.printf("(71)Case Index = %i \n",caseIndex);
          caseIndex = 2;
        }
      }
    break;

//  SMART OUTLET MANUAL CONTROL - TURN ON
    case 72:  // Turn ON Smart outlet at individual address
      switchON(smartOutletAdd);
    Serial.println("case 72");
      caseIndex = 2;
      break;

    
//  SMART OUTLET MANUAL CONTROL - TURN OFF
    case 73:  // Turn OFF Smart outlet at individual address
      switchOFF(smartOutletAdd);
    Serial.println("case 73");
      caseIndex = 2;
      break;

    
//  SMART OUTLET MANUAL CONTROL - TURN ALL OFF
    case 74:  // Turn OFF ALL Smart outlets
      for (i=0; i<=maxOutlets; i++) {
      switchOFF(i);
      }
    Serial.println("case 74");
      caseIndex = 2;
      break;

    
    default:
      displayOne.clearDisplay();//  Clear the display before going further
  displayOne.drawRect(2,2,SCREENWIDTH-2,SCREENHEIGHT-2,MENUCOLOR[1]);
    displayOne.setCursor(7,30);
    displayOne.println("Default Screen");
      displayOne.display(); // Force display 
      delay(1000);
      caseIndex = 0;


//      menuPos = doEncoder(firstTime,35,4);    //  Hard code of max values for default case  
//    Serial.println("default");
//
//      encoderButton();
//          Serial.printf("(Default)Case Index = %i \n",caseIndex);
//      displayOne.clearDisplay();//  Clear the display before going further
//      baseText(menuLevel, menuPos);
//      displayOne.display(); // Force display 
//      if (menuPos == 0) {
//        firstTime = 0;
//      }
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

void  encoderButton() {
      buttonPress = digitalRead(BUTTONPIN);
      if (!buttonPress && buttonPress != buttonPressed) {
        caseIndex = ((menuLevel) * 10) + menuPos;  // setting case statement index;
        buttonPressed = buttonPress;
        firstTime = 1;  // Reset menu cursor to top position in the new menu
      } else if (buttonPress && buttonPress != buttonPressed) {
        buttonPressed = buttonPress;
      } 
}

int doEncoder (bool newLevel, int encMax, int menuMax) {  //  check for encoder rotary movement
  int encPos;
  int menuPosition;

  if (encMax == 0 && menuMax == 0) {  // Reset encoder to zero
    encOne.write(0);                   // force encoder to minimum value
    encPos = encOne.read();
  } else {                             // Normal encoder read operation
    encPos = encOne.read();
    if (encPos < 0) {                    // check if encoder has gone below minimum value
      encPos = 0;                        // Set position to minimum value
      encOne.write(0);                   // force encoder to minimum value
    }
    else if (encPos >= encMax) {
      encPos = encMax;                         // Set position to minimum value
      encOne.write(encMax);                    // force encoder to minimum value
    }
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

void click1() 
{
   caseState = ((menuLevel) * 10) + menuPos;  // setting case statement index;
}

void click2() {
   caseIndex = ((menuLevel) * 10) + menuPos;  // setting case statement index;
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
