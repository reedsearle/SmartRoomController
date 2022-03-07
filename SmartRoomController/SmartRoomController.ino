
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

// OLED screen constants
const int SCREENWIDTH  = 128;  //  Width of screen in pixels
const int SCREENHEIGHT = 64;   //  Height of screen in pixels
const int OLEDRESET    = -1;   //  OLED Reset shared with Teensy
byte      OLEDADDRESS  = 0x3C; //  Address of the OLED on the IIC bus
const int MENUPOSITIONS[] = {5, 15, 25, 35, 45};  // Starting positions for all menu options
const int MENUCOLOR[]     = {0,1,2}; // Set three colors to print, in order: BLACK, WHITE, INVERSE
String    MENUHOME[]      = {"SMART SCANNER", "* Smart Lights", "* Smart Outlets", "* IP Address", "* 4th option"};

// ENCODER Pins
const int ENCAPIN     = 22;  // Encoder A pin
const int ENCBPIN     = 21;  // Encoder B pin

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
int  menuPos;

int startTime;
int endTime;
int delayTime;

int i;

// Constructors
Adafruit_SSD1306  displayOne(SCREENWIDTH, SCREENHEIGHT, &Wire, OLEDRESET);
Encoder           encOne(ENCAPIN, ENCBPIN);

//TEST CODE

void setup() {
  Serial.begin(9600);
  while(!Serial);
  Serial.printf("Serial Port running\n");
  

  if(!displayOne.begin(SSD1306_SWITCHCAPVCC, OLEDADDRESS)) {
    Serial.printf("OLED display did not initialize correctly.  Please reset.\n");
    while(1);  //  You shall not pass 
  }

  Serial.printf("OLED display running\n");
//  Serial.println(MENUCOLOR[0]);
//  Serial.println(MENUCOLOR[1]);

// ADD ETHERNET HERE

  buttonPress   = 0;                      // Initial state is button NOT pressed
  buttonPressed = 0;                      // Initial state is buttonhas NOT been pressed
  encPos        = 0;                      // Set up encoder wheel
  encPosOld     = 0;
  encMin        = 0;                      // Minimum position for encoder
  encMax        = 35;                     // Maximun position for encoder addressing
  menuMin        = 0;                      // Minimum position for menu
  menuMax        = 4;                     // Maximun position for menu
  endTime = 0;
}

void loop() {
//  startTime = millis();
//  delayTime = startTime - endTime;
//  if (delayTime > DELAYTIME) { // Delaying to avoid overrunning the display MIGHT REMOVE IN FUTURE

    menuPos = doEncoder();
    baseText(menuPos);
    displayOne.display(); // Force display

//    endTime = millis();
//  }
  
  
}

void baseText(int menuCursor){  // Set up the recurring text and graphics
  displayOne.clearDisplay();//  Clear the display before going further
  displayOne.drawRect(0,0,SCREENWIDTH,SCREENHEIGHT,SSD1306_INVERSE);
  displayOne.setTextSize(1);

  for (i=0; i<= menuMax; i++) { // cycle through all possible menu positions
    if (i == menuCursor) {
      displayOne.setTextColor(MENUCOLOR[0],MENUCOLOR[1]);
    } else {
      displayOne.setTextColor(MENUCOLOR[1]);
    }
    displayOne.setCursor(7,MENUPOSITIONS[i]);
    displayOne.println(MENUHOME[i]);
  }
}

int doEncoder () {  //  check for encoder rotary movement
  int menuPosition;
    encPos = encOne.read();
  if (encPos < encMin) {                    // check if encoder has gone below minimum value
    encPos = encMin;                        // Set position to minimum value
    encOne.write(encMin);                   // force encoder to minimum value
  }
  else if (encPos >= encMax) {
    encPos = encMax;                         // Set position to minimum value
    encOne.write(encMax);                    // force encoder to minimum value
  }

    menuPosition = map (encPos,encMin,encMax,menuMin,menuMax); //  increase the encoder position width to reduce bounce
//    menuPosition = map (encPos,0,95,0,4); //  increase the encoder position width to reduce bounce

  if (menuPosition < menuMin) {                    // check if encoder has gone below minimum value
    menuPosition = menuMin;                        // Set position to minimum value
  }
  else if (menuPosition >= menuMax) {
    menuPosition = menuMax;                         // Set position to minimum value
  }
    return menuPosition;

//  TRYING TO AVOID REFRESHING SCREEN ALOT
//  if (encPosOld == encPos) {  //  check if encoder has moved, if not, let calling function know
//    return null;
//  } else {                    // Indicate that encoder has moved and update position
//    encPosOld = encPos;                        // Set value of old to current for comparison
//    return menuPosition;
//  }

}
