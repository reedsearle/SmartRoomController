
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
#include <OneButton.h>


// OLED screen constants
const int SCREENWIDTH      = 128;  //  Width of screen in pixels
const int SCREENHEIGHT     = 64;   //  Height of screen in pixels
const int OLEDRESET        = -1;   //  OLED Reset shared with Teensy
byte      OLEDADDRESS      = 0x3C; //  Address of the OLED on the IIC bus
const int MENUPOSITIONS[]  = {5, 15, 25, 35, 45};  // Starting positions for all menu options
const int MENUCOLOR[]      = {0,1,2}; // Set three colors to print, in order: BLACK, WHITE, INVERSE
String    MENUHOME[][5] = {
                            {"SMART SCANNER",       "* Smart Lights", "* Smart Outlets",      "",              ""           },
                            {"SMART LIGHTS ",       "* Address",      "* Delay",              "* Fire",        "* Manual"   },
                            {"SMART LIGHTS ADDR",   "",               "* Previous String",    "* Next String", "FLASH"      },
                            {"SMART LIGHTS FIRE",   "FIRE String",    "* Create New String",  "* Test",        ""           },
                            {"SMART LIGHTS FIRE2",  "* Total Lights", "* Addresses in order", "",              ""           },
                            {"SMART LIGHTS MANUAL", "* Address",      "* Hue",                "* Turn ON",     "* Turn OFF" },
                            {"SMART LIGHTS DELAY",  "* Address",      "* Initialize",         "* Run Delay",   ""           },
                            {"SMART OUTLETS ",      "* Address",      "* Turn ON",            "* Turn OFF",    "* All Off"  }
                          };

// ENCODER Pins
const int ENCAPIN     = 23;  // Encoder A pin
const int ENCBPIN     = 22;  // Encoder B pin
const int BUTTONPIN   = 21;  // Encoder button

// PHOTOSENSOR Pin
const int SENSORPIN   = 20;  // Photosensor input

// FIRE BUTTON Pin
const int FIREBUTTONPIN   = 16;  // Fire button

  // Encoder Variables
int  buttonPress;          // Initial state is button NOT pressed
int  buttonPressed;        // Initial state is buttonhas NOT been pressed
int  menuPos;              // Cursor location within a menu
int  menuLevel;            // Address of new menu
bool firstTime;            // indicates a new menu level has been requested
int  throwAway;            //  Variable used to call doEncoder to force initial values

// Photosensor variables
int lightIn;               //  value of the light being received by the photoresistor

// Fire Button variables
bool fireButtonState;      //  Determines if the FIRE button has been pressed

int smartLightAddD;        // Address of the current smart light in DELAY
int smartLightAddM;        // Address of the current smart light in MANUAL
int smartLightHue;         // Hue of the current smart light
int lightBatch;            //  Integer number representing an index of strings of lights
int startBatch;            //  Starting address of a batch of lights to flash in orer to determine their address
int holdHue;               //  Temporary register to hold HUE values when lights are being changed
int fireBatch[3];          //  Address register for the lights in a fire string

int smartOutletAdd;        // Address of the current smart light
int maxOutlets;            // Maximum number of smart outlets on network

int caseIndex;             //  Variable to move around the State machine

// Smart Light delay variables
int delayStart;
int delayStop;
int deltaDelay;

int i,j;                   //  Indicies for loops

// Constructors
Adafruit_SSD1306  displayOne(SCREENWIDTH, SCREENHEIGHT, &Wire, OLEDRESET);
Encoder           encOne(ENCAPIN, ENCBPIN);
OneButton         fireButton(FIREBUTTONPIN, true, true);





//**********************************************************//
//**********************************************************//
//
//                            SETUP
//
//**********************************************************//
//**********************************************************//

void setup() {
  
//**********************************************************//
// Start OLED Display
//**********************************************************//
  if(!displayOne.begin(SSD1306_SWITCHCAPVCC, OLEDADDRESS)) {
    Serial.printf("OLED display did not initialize correctly.  Please reset.\n");
    while(1);  //  You shall not pass 
  }
  displayOne.display(); // show the Adafruit logo stored in memory
  delay(1000);  //  delay to see logo but get rid of this later
  Serial.printf("OLED display running\n");


//**********************************************************//
//Start Serial  
//**********************************************************//
  Serial.begin(9600);
  Serial.printf("Serial Port running\n");

// Start Ethernet
  Ethernet.begin(mac);
  printIP();
  Serial.printf("LinkStatus: %i  \n",Ethernet.linkStatus());

  pinMode(BUTTONPIN,     INPUT_PULLUP);

//**********************************************************//
// Start OneButton
//**********************************************************//
  fireButton.attachClick(click1);             // Set CLICK1 as function for single click
  fireButton.attachLongPressStart(LPstart);   // Set LPstart as function for double click
  fireButtonState = 0;  //  Fire button has NOT been pressed

//**********************************************************//
//     Initialize Variables
//**********************************************************//

  buttonPress     = 0;      //  Initial state is button NOT pressed
  buttonPressed   = 0;      //  Initial state is buttonhas NOT been pressed
  menuLevel       = 0;      //  Starting menu is top level
  caseIndex       = 0;      //  State machine Starts at TOP LEVEL
  firstTime       = 0;      //  First time through a menu  ?? Still Needed??

  smartLightAddD  = 1;      //  First Address for Smart Light Delay is 1
  smartLightAddM  = 1;      //  First Address for Smart Light Manual control is 1
  smartOutletAdd  = 0;      //  First Address for Smart Outlet is 1
  lightBatch      = 1;      //  Address for Smart Light Delay is 1
  fireBatch[0]    = {1};    //  First Address for Fire String is 1
  fireBatch[1]    = {2};    //  Second Address for Fire String is 2
  fireBatch[2]    = {3};    //  Third Address for Fire Stringy is 3
  
  maxOutlets      = 10;     // Arbitrarily set to 10
}





//**********************************************************//
//**********************************************************//
//
//                             LOOP
//
//**********************************************************//
//**********************************************************//

void loop() {
  
  fireButton.tick();                                //  Check state of the Fire Button
  if (fireButtonState) {                            //  Fire Button has been clicked
    fireFireFire();                                 //  Call the function to prodive exit lighting
  } else {                                          //  Otherwise, just start the show
  switch (caseIndex) {
//**********************************************************//
//**********************************************************//
//
//     SMART ROOM CONTROLLER
//
//**********************************************************//
//**********************************************************//

    case 0:                                                // Home State
      fireButton.tick();                                   //  Check state of the Fire Button
      menuPos = doEncoder(firstTime,0,35,0,4,-1);          //  Get menu position from the encoder
      menuLevel = 0;                                       //  Set menu level 0 for the screen text
      encoderButton();                                     //  Check value of the encoder button and change state
      displayOne.clearDisplay();                           //  Clear the display before going further
      baseText(menuLevel, menuPos);                        //  Print the background text and graphics for this menu level
      displayOne.setTextColor(MENUCOLOR[1]);               //  Set OLED color to white to print text
      displayOne.setCursor(7,MENUPOSITIONS[3]);            //  Move to the IP ADDRESS position
      displayOne.println("  IP Address");                  //  Print the current address
      displayOne.setCursor(7,MENUPOSITIONS[4]);            //  Move to the print the IP ADDRESS 
      displayOne.printf("  ");                             //  move cursor to line up with above text
      for (byte thisByte = 0; thisByte < 3; thisByte++) {  //  Print the IPaddress
        displayOne.printf("%i.",Ethernet.localIP()[thisByte]);
      }
      displayOne.printf("%i\n",Ethernet.localIP()[3]);
      displayOne.display();                                 // Force display 
      if (menuPos == 0) {
        firstTime = 0;
      }
      break;

 


//**********************************************************//
//**********************************************************//
//
//     SMART LIGHT 
//
//**********************************************************//
//**********************************************************//
    case 10:          //  Redirect up one level. 
     caseIndex = 0;  //  Redirect
     break;
     
//**********************************************************//
//**********************************************************//
    case 1:                                                // Smart Light State
      fireButton.tick();                                   //  Check state of the Fire Button
      buttonPress = digitalRead(BUTTONPIN);                //  Read the encoder Button but do not change state
      menuLevel = 1;                                       //  Set menu level 1 for the screen text
      if (buttonPress && buttonPress != buttonPressed) {   //  Check for rising edge of encoder button to be certain finger is OFF of button
      throwAway = doEncoder(0, 0, 35 , 0, 4, 0);           //  force initial values to the encoder  
        while(buttonPress) {                               //  make CERTAIN encoder button is not being pressed.  Pressing button SELECTS
          fireButton.tick();                               //  Check state of the Fire Button
          menuPos = doEncoder(firstTime,0,35,0,4,-1);      //  Find new value for the menu position  
          encoderButton();                                 //  Check value of the encoder button and change state
          displayOne.clearDisplay();                       //  Clear the display before going further
          baseText(menuLevel, menuPos);                    //  Print the background text and graphics for this menu level
          displayOne.display();                            //  Force display 
          if (menuPos == 0) {                              //  Let cursor move freely
            firstTime = 0;
          }
        }
      delay(250); // Debounce
      }
      break;





//**********************************************************//
//**********************************************************//
//
//     SMART LIGHT ADDRESSING - FLASHING FOR EFFECT
//
//**********************************************************//
//**********************************************************//

//**********************************************************//
//         Return Case
//**********************************************************//
    case 20:          //  Redirect up one level.  
     caseIndex = 1;  //  Redirect
     break;
     
//**********************************************************//
//**********************************************************//
    case 11:
      fireButton.tick();                                   //  Check state of the Fire Button
      buttonPress = digitalRead(BUTTONPIN);                //  Read the encoder Button but do not change state
      menuLevel = 2;                                       //  Set menu level 2 for the screen text
      if (buttonPress && buttonPress != buttonPressed) {   //  Check for rising edge of encoder button to be certain finger is OFF of button
      throwAway = doEncoder(0, 0, 35 , 0, 4, 0);           //  force initial values to the encoder  
        while(buttonPress) {                               //  make CERTAIN encoder button is not being pressed.  Pressing button SELECTS
          fireButton.tick();                               //  Check state of the Fire Button
          menuPos = doEncoder(firstTime,0,35,0,4,-1);      //  Find new value for the menu position  
          encoderButton();                                 //  Check value of the encoder button and change state
          startBatch = 3*(lightBatch-1)+1;                 //  Create starting address for batch of flashing lights
          displayOne.clearDisplay();                       //  Clear the display before going further
          baseText(menuLevel, menuPos);                    //  Print the background text and graphics for this menu level
          displayOne.setTextColor(MENUCOLOR[1]);           //  Set OLED color to white to print text
          displayOne.setCursor(7,MENUPOSITIONS[1]);        //  Move to the position to print the display string below
          displayOne.printf("String %i (%i-%i)",lightBatch, startBatch, startBatch+2);  //  Print String
          displayOne.display();                            // Force display 
          if (menuPos == 0) {                              //  Let cursor move freely
            firstTime = 0;
          }
        }
      }
      break;

//**********************************************************//
//**********************************************************//
    case 21:          //  Redirect up one level.  This case does nothing
     caseIndex = 11;  //  Redirect
     break;
     
//**********************************************************//
//**********************************************************//
    case 22:          //  Redirect up one level.  This case does nothing
      if (lightBatch > 1) {
        lightBatch--;
      } else {
        lightBatch = 1;
      }
      caseIndex = 11;  //  Redirect
      break;
     
//**********************************************************//
//**********************************************************//
    case 23:          //  Redirect up one level.  This case does nothing
      lightBatch++;
      caseIndex = 11;  //  Redirect
      break;
     
//**********************************************************//
//**********************************************************//
    case 24:          //  Redirect up one level.  This case does nothing
      fireButton.tick();                                   //  Check state of the Fire Button
      buttonPress = digitalRead(BUTTONPIN);                //  Read the encoder Button but do not change state
      if (buttonPress && buttonPress != buttonPressed) {   //  Check for rising edge of encoder button to be certain finger is OFF of button
//  Insert method of reading initial hues (and brightness?)
        while(buttonPress) {                               //  make CERTAIN encoder button is not being pressed.  Pressing button SELECTS
            setHue(startBatch ,   true,HueRed,255,255);    //  Set initial color of first light in batch 
            setHue(startBatch +1, true,HueRed,255,255);    //  Set initial color of second light in batch
            setHue(startBatch +2, true,HueRed,255,255);    //  Set initial color of third light in batch
          delay(1000);
            setHue(startBatch ,   true,HueYellow,255,255); //  Set second color of first light in batch
            setHue(startBatch +1, true,HueGreen,255,255);  //  Set second color of second light in batch
            setHue(startBatch +2, true,HueBlue,255,255);   //  Set second color of third light in batch
          delay(1000);
        }
          for (j=0; j<=2; j++) {                           // Index through the string
            setHue(startBatch +j,true,HueRed,255,0);       // Turn all lights in current string to white. This is where the lights should return to their initial values
          }
        }
  
     caseIndex = 11;  //  Redirect
     break;



     
//**********************************************************//
//**********************************************************//
//
//     SMART LIGHT FIRE - VIEW, EDIT, AND TEST STRINGS
//
//**********************************************************//
//**********************************************************//

//**********************************************************//
//         Return Case
//**********************************************************//
    case 30:          //  Redirect up one level. 
     caseIndex = 1;  //  Redirect
     break;
     
//**********************************************************//
//**********************************************************//
    case 13:
      fireButton.tick();                                   //  Check state of the Fire Button
      buttonPress = digitalRead(BUTTONPIN);                //  Read the encoder Button but do not change state
      menuLevel = 3;                                       //  Set menu level 3 for the screen text
      if (buttonPress && buttonPress != buttonPressed) {   //  Check for rising edge of encoder button to be certain finger is OFF of button
      throwAway = doEncoder(0, 0, 35 , 0, 4, 0);           //  force initial values to the encoder   
        while(buttonPress) {                               //  make CERTAIN encoder button is not being pressed.  Pressing button SELECTS
          fireButton.tick();                               //  Check state of the Fire Button
          menuPos = doEncoder(firstTime,0,35,0,4,-1);      //  Find new value for the menu position   
          encoderButton();                                 //  Check value of the encoder button and change state
          displayOne.clearDisplay();                       //  Clear the display before going further
          baseText(menuLevel, menuPos);                    //  Print the background text and graphics for this menu level
          displayOne.setTextColor(MENUCOLOR[1]);           //  Set OLED color to white to print text
          displayOne.setCursor(80,MENUPOSITIONS[1]);       //  Move to the position to print the display string below
          displayOne.printf("%i-%i-%i",fireBatch[0], fireBatch[1], fireBatch[2]);  // Print Fire light addresses
          displayOne.display();                            // Force display 
          if (menuPos == 0) {                              //  Let cursor move freely
            firstTime = 0;
          }
        }
      }
      break;

//**********************************************************//
//        RETURN
//**********************************************************//
    case 31:          //  Redirect up one level.  This case does nothing
     caseIndex = 13;  //  Redirect
     break;
     
//**********************************************************//
//**********************************************************//
    case 32:                                                  //  
       fireButton.tick();                                     //  Check state of the Fire Button
     j = 0;
      while (j<=2) {                                          //  go through all three addresses
      buttonPress = digitalRead(BUTTONPIN);                   //  Read the encoder Button but do not change state
      if (buttonPress && buttonPress != buttonPressed) {      //  Check for rising edge of encoder button to be certain finger is OFF of button
      throwAway = doEncoder(0,0,95,1,23, fireBatch[j]);       //  force initial values to the encoder   
          while(buttonPress) {                                //  make CERTAIN encoder button is not being pressed.  Pressing button SELECTS
            fireButton.tick();                                //  Check state of the Fire Button
            fireBatch[j] = doEncoder(0,0,95,1,23,-1);         //  Get address value from the encoder, -1 indicates do not preset the value 
            displayOne.clearDisplay();                        //  Clear the display before going further
            baseText(menuLevel, 1);                           //  Print the background text and graphics for this menu level
            displayOne.setCursor(80,MENUPOSITIONS[1]);        //  Move to the position to print the display string below
            displayOne.printf("%i-%i-%i",fireBatch[0], fireBatch[1], fireBatch[2]);  // Print Fire light addresses
            displayOne.display();                             // Force display 
            buttonPress = digitalRead(BUTTONPIN);             //  Check if the button has been pressed to select the address
            caseIndex = 13;
          }
        Serial.printf("i %i firebatch %i\n",j, fireBatch[j]);
          j++;
        }
      }
     break;
     
//**********************************************************//
//**********************************************************//
    case 33:          //  
      fireButton.tick();                                   //  Check state of the Fire Button
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
     
//**********************************************************//
//**********************************************************//
    case 34:          //  Redirect up one level.  This case does nothing
     caseIndex = 13;  //  Redirect
     break;




     
//**********************************************************//
//**********************************************************//
//
//     SMART LIGHT MANUAL CONTROL
//
//**********************************************************//
//**********************************************************//

//**********************************************************//
//         Return Case
//**********************************************************//
    case 50:          //  Redirect up one level
     caseIndex = 1;  //  Redirect
     break;

//**********************************************************//
//     SMART LIGHT MANUAL CONTROL ---  MENU TOP LEVEL
//**********************************************************//
    case 14:
      fireButton.tick();                                   //  Check state of the Fire Button
      buttonPress = digitalRead(BUTTONPIN);                //  Read the encoder Button but do not change state
      menuLevel = 5;                                       //  Set menu level 5 for the screen text
      if (buttonPress && buttonPress != buttonPressed) {   //  Check for rising edge of encoder button to be certain finger is OFF of button
      throwAway = doEncoder(0, 0, 35 , 0, 4, 0);           //  force initial values to the encoder   
        while(buttonPress) {                                //  make CERTAIN encoder button is not being pressed.  Pressing button SELECTS
          fireButton.tick();                                   //  Check state of the Fire Button
          menuPos = doEncoder(firstTime,0,35,0,4,-1);       //  Find new value for the menu position   
          encoderButton();                                     //  Check value of the encoder button and change state
          displayOne.clearDisplay();//  Clear the display before going further
          baseText(menuLevel, menuPos);                    //  Print the background text and graphics for this menu level
          displayOne.setTextColor(MENUCOLOR[1]);               //  Set OLED color to white to print text
          displayOne.setCursor(70,MENUPOSITIONS[1]);        //  Move to the position to print the display string below
          displayOne.printf("%i",smartLightAddM);
          displayOne.setCursor(70,MENUPOSITIONS[2]);        //  Move to the position to print the display string below
          displayOne.printf("0x%04X",smartLightHue);
          displayOne.display(); // Force display 
          if (menuPos == 0) {                              //  Let cursor move freely
            firstTime = 0;
          }
        }
      }

//**********************************************************//
//         SMART LIGHT MANUAL CONTROL - ADDRESS
//**********************************************************//
    case 51:                                              // Input indiviual Smart Light address
      fireButton.tick();                                  //  Check to see if the Fire Button has been pressed
      buttonPress = digitalRead(BUTTONPIN);               //  Check to see status of the encoder Button has been pressed
      if (buttonPress && buttonPress != buttonPressed) {  //  Check for the rising edge of the encoder button (finger coming off the button
      throwAway = doEncoder(0,0,95,1,23, smartLightAddM); //  Hard code of max values for default case  
        while(buttonPress) {                              //  Button is NOT pressed
          fireButton.tick();                              //  Check to see if the Fire Button has been pressed
          smartLightAddM = doEncoder(0,0,95,1,23, -1);    //  Get address value from the encoder, -1 indicates do not preset the value  
          displayOne.clearDisplay();                      //  Clear the display before going further
          baseText(menuLevel, 1);                         //  Print the background text and graphics for this menu level 
          displayOne.setCursor(70,MENUPOSITIONS[1]);      //  Move to the ADDRESS position
          displayOne.printf("%i",smartLightAddM);         //  Print the current address
          displayOne.setCursor(70,MENUPOSITIONS[2]);      //  Move to the HUE position
          displayOne.printf("0x%04X",smartLightHue);      //  Print the current HUE
          displayOne.display();                           //  Force display 
          buttonPress = digitalRead(BUTTONPIN);           //  Check if the button has been pressed to select the address
          caseIndex = 14;                                 //  Go back to the top of this menu level
        }
      }
    break;

//**********************************************************//
//  SMART LIGHT MANUAL CONTROL - HUE
//**********************************************************//
     case 52:  // Input indiviual Smart Light Hue
      fireButton.tick();                                   //  Check state of the Fire Button
      buttonPress = digitalRead(BUTTONPIN);                //  Read the encoder Button but do not change state
      if (buttonPress && buttonPress != buttonPressed) {   //  Check for rising edge of encoder button to be certain finger is OFF of button
      throwAway = doEncoder(0,0,255 ,0,65353, smartLightHue);           //  force initial values to the encoder   
        while(buttonPress) {                                //  make CERTAIN encoder button is not being pressed.  Pressing button SELECTS
          fireButton.tick();                                   //  Check state of the Fire Button
          smartLightHue = doEncoder(0,0,255 ,0,65353, -1);     //  Get address value from the encoder, -1 indicates do not preset the value 
          displayOne.clearDisplay();//  Clear the display before going further
          baseText(menuLevel, 2);                    //  Print the background text and graphics for this menu level
          displayOne.setCursor(70,MENUPOSITIONS[1]);        //  Move to the position to print the display string below
          displayOne.printf("%i",smartLightAddM);
          displayOne.setCursor(70,MENUPOSITIONS[2]);        //  Move to the position to print the display string below
          displayOne.printf("0x%04X",smartLightHue);
          displayOne.display(); // Force display 
          buttonPress = digitalRead(BUTTONPIN);           //  Check if the button has been pressed to select the hue
          caseIndex = 14;
        }
      }
    delay(250); // Debounce
    break;

    
//**********************************************************//
//  SMART LIGHT MANUAL CONTROL - TURN ON
//**********************************************************//
    case 53:  // Turn ON Smart light at individual address
      setHue(smartLightAddM,true,smartLightHue,255,255);
      delay(250); // Debounce
      caseIndex = 14;
      break;

    
//**********************************************************//
//  SMART LIGHT MANUAL CONTROL - TURN OFF
//**********************************************************//
    case 54:  // Turn OFF Smart light at individual address
      setHue(smartLightAddM,false,0,0,0);
      caseIndex = 14;
      delay(250); // Debounce
      break;




//**********************************************************//
//**********************************************************//
//
//     SMART LIGHT DELAY CONTROL
//
//**********************************************************//
//**********************************************************//

//**********************************************************//
//         Return Case
//**********************************************************//
    case 60:
     caseIndex = 1;  //  Redirect
     break;
     
//**********************************************************//
//**********************************************************//
    case 12:
      fireButton.tick();                                   //  Check state of the Fire Button
      buttonPress = digitalRead(BUTTONPIN);                //  Read the encoder Button but do not change state
      menuLevel = 6;                                       //  Set menu level 6 for the screen text
      if (buttonPress && buttonPress != buttonPressed) {   //  Check for rising edge of encoder button to be certain finger is OFF of button
      throwAway = doEncoder(0, 0, 35 , 0, 4, 0);           //  force initial values to the encoder   
        while(buttonPress) {                                //  make CERTAIN encoder button is not being pressed.  Pressing button SELECTS
          fireButton.tick();                                   //  Check state of the Fire Button
          menuPos = doEncoder(firstTime, 0, 35, 0, 4, -1);      //  Find new value for the menu position   
          encoderButton();                                     //  Check value of the encoder button and change state
          displayOne.clearDisplay();//  Clear the display before going further
          baseText(menuLevel, menuPos);                    //  Print the background text and graphics for this menu level
          displayOne.setTextColor(MENUCOLOR[1]);               //  Set OLED color to white to print text
          displayOne.setCursor(80,MENUPOSITIONS[1]);        //  Move to the position to print the display string below
          displayOne.printf("%i",smartLightAddD);
          displayOne.setCursor(80,MENUPOSITIONS[3]);        //  Move to the position to print the display string below
          displayOne.printf("%i",deltaDelay);
          displayOne.display(); // Force display 
          if (menuPos == 0) {                              //  Let cursor move freely
            firstTime = 0;
          }
        }
      }
      break;
    
//**********************************************************//
//  SMART LIGHT DELAY - ADDRESS
//**********************************************************//
    case 61:  // Input indiviual Smart Light address
      fireButton.tick();                                   //  Check state of the Fire Button
      buttonPress = digitalRead(BUTTONPIN);                //  Read the encoder Button but do not change state
      if (buttonPress && buttonPress != buttonPressed) {   //  Check for rising edge of encoder button to be certain finger is OFF of button
      throwAway = doEncoder(0,0,95,1,23, smartLightAddD);            //  force initial values to the encoder   
        while(buttonPress) {                               //  make CERTAIN encoder button is not being pressed.  Pressing button SELECTS
          fireButton.tick();                                   //  Check state of the Fire Button
          smartLightAddD = doEncoder(0,0,95,1,23,-1);    //  Get address value from the encoder, -1 indicates do not preset the value  
          displayOne.clearDisplay();//  Clear the display before going further
          baseText(menuLevel, 1);                    //  Print the background text and graphics for this menu level
          displayOne.setCursor(80,MENUPOSITIONS[1]);        //  Move to the position to print the display string below
          displayOne.printf("%i",smartLightAddD);
          displayOne.display(); // Force display 
          buttonPress = digitalRead(BUTTONPIN);           //  Check if the button has been pressed to select the address
          Serial.printf("(61)Case Index = %i \n",caseIndex);
          caseIndex = 12;
        }
      }
    break;

//**********************************************************//
//  SMART LIGHT DELAY - INITIALIZE (turn off to start)
//**********************************************************//
    case 62:  // Turn OFF Smart light at individual address
      setHue(smartLightAddD,false,0,0,0);
      caseIndex = 12;
      Serial.println("case 62");
      break;

//**********************************************************//
//  SMART LIGHT DELAY - RUN (turn on, measure delay, turn offf, measure delay)
//**********************************************************//
    case 63:  // Turn ON Smart light at individual address
      setHue(smartLightAddD,true,0x0000,255,255);  // turn on light with white color
      delayStop = millis();  // Set up delay counter
      delayStart = millis();
      lightIn = 0;           // Initialize light sensor
      while(lightIn < 100 && delayStart - delayStop <= 10000) {  // Loop while not seeing light and 10 sec timer
        fireButton.tick();                                   //  Check state of the Fire Button
        lightIn = analogRead(SENSORPIN);
        delayStart = millis();
      }
      deltaDelay = delayStart - delayStop;
//    Serial.println("case 53");
      caseIndex = 12;
      break;

    
//**********************************************************//
//**********************************************************//
    case 64:  // empty menu index, redirect
      caseIndex = 12;
      break;

//**********************************************************//
//**********************************************************//
//
//     SMART OULTETS MANUAL CONTROL
//
//**********************************************************//
//**********************************************************//

//**********************************************************//
//         Return Case
//**********************************************************//
    case 70:
    caseIndex = 0;  // Redirect up one menu level
    break;
     


   
//**********************************************************//
//  SMART OUTLET MANUAL CONTROL
//**********************************************************//
    case 2:
      fireButton.tick();                                   //  Check state of the Fire Button
      buttonPress = digitalRead(BUTTONPIN);                //  Read the encoder Button but do not change state
      menuLevel = 7;                                       //  Set menu level 7 for the screen text
      if (buttonPress && buttonPress != buttonPressed) {   //  Check for rising edge of encoder button to be certain finger is OFF of button
      throwAway = doEncoder(0, 0, 35 ,0, 4, 0);            //  force initial values to the encoder  
        while(buttonPress) {                                //  make CERTAIN encoder button is not being pressed.  Pressing button SELECTS
          fireButton.tick();                                   //  Check state of the Fire Button
          menuPos = doEncoder(firstTime,0,35,0,4,-1);      //  Find new value for the menu position   
          encoderButton();                                     //  Check value of the encoder button and change state
          displayOne.clearDisplay();//  Clear the display before going further
          baseText(menuLevel, menuPos);                    //  Print the background text and graphics for this menu level
          displayOne.setTextColor(MENUCOLOR[1]);               //  Set OLED color to white to print text
          displayOne.setCursor(70,MENUPOSITIONS[1]);        //  Move to the position to print the display string below
          displayOne.printf("%i",smartOutletAdd);
          displayOne.display(); // Force display 
          if (menuPos == 0) {                              //  Let cursor move freely
            firstTime = 0;
          }
        }
      }
          Serial.printf("(2)Case Index = %i \n",caseIndex);
      break;
    
//**********************************************************//
//  SMART OUTLET MANUAL CONTROL - ADDRESS
//**********************************************************//
    case 71:  // Input indiviual Smart outlet address
      fireButton.tick();                                   //  Check state of the Fire Button
      buttonPress = digitalRead(BUTTONPIN);                //  Read the encoder Button but do not change state
      if (buttonPress && buttonPress != buttonPressed) {   //  Check for rising edge of encoder button to be certain finger is OFF of button
      throwAway = doEncoder(0,0,95,0,23, smartOutletAdd);           //  force initial values to the encoder 
        while(buttonPress) {                                //  make CERTAIN encoder button is not being pressed.  Pressing button SELECTS)
          fireButton.tick();                                   //  Check state of the Fire Button
          smartOutletAdd = doEncoder(0,0,95,0,23,-1);    //  Get address value from the encoder, -1 indicates do not preset the value  
          displayOne.clearDisplay();                           //  Clear the display before going further
          baseText(menuLevel, 1);                    //  Print the background text and graphics for this menu level
          displayOne.setCursor(70,MENUPOSITIONS[1]);        //  Move to the position to print the display string below
          displayOne.printf("%i",smartOutletAdd);
          displayOne.display(); // Force display 
          buttonPress = digitalRead(BUTTONPIN);           //  Check if the button has been pressed to select the address
          Serial.printf("(71)Case Index = %i \n",caseIndex);
          caseIndex = 2;
        }
      }
    break;

//**********************************************************//
//  SMART OUTLET MANUAL CONTROL - TURN ON
//**********************************************************//
    case 72:  // Turn ON Smart outlet at individual address
      switchON(smartOutletAdd);
    Serial.println("case 72");
      caseIndex = 2;
      break;

    
//**********************************************************//
//  SMART OUTLET MANUAL CONTROL - TURN OFF
//**********************************************************//
    case 73:  // Turn OFF Smart outlet at individual address
      switchOFF(smartOutletAdd);
    Serial.println("case 73");
      caseIndex = 2;
      break;

    
//**********************************************************//
//  SMART OUTLET MANUAL CONTROL - TURN ALL OFF
//**********************************************************//
    case 74:  // Turn OFF ALL Smart outlets
      for (i=0; i<=maxOutlets; i++) {
      switchOFF(i);
      }
    Serial.println("case 74");
      caseIndex = 2;
      break;

    
//**********************************************************//
//**********************************************************//
//
//     DEFAULT
//
//**********************************************************//
//**********************************************************//
    default:
      fireButton.tick();
      displayOne.clearDisplay();//  Clear the display before going further
      displayOne.drawRect(2,2,SCREENWIDTH-2,SCREENHEIGHT-2,MENUCOLOR[1]);
      displayOne.setCursor(7,30);
      displayOne.println("Default Screen");
      displayOne.display(); // Force display 
      delay(1000);
      caseIndex = 0;
      break;
    
  }
}
}





//**********************************************************//
//**********************************************************//
//
//     Function: baseText
//
//**********************************************************//
//**********************************************************//
//  This function prints to the OLED display the background
//  text and graphics for a given menu level

void baseText(int menuLvl, int menuCursor){                             //  Set up the recurring text and graphics
  int indexMax = 4;                                                     //  5 possible menu address on OLED screen
  displayOne.drawRect(2,2,SCREENWIDTH-2,SCREENHEIGHT-2,MENUCOLOR[1]);   //  Draw the screen border

  for (i=0; i<= indexMax; i++) {                                        //  cycle through all possible menu positions
    if (i == menuCursor) {                                              //  Check if current menu option is where the cursor is
      displayOne.setTextColor(MENUCOLOR[0],MENUCOLOR[1]);               //  Set color to inverse for cursor
    } else {
      displayOne.setTextColor(MENUCOLOR[1]);                            //  Set color to white for all else
    }
    displayOne.setCursor(7,MENUPOSITIONS[i]);                           //  Move cursor to location to print text
    displayOne.println(MENUHOME[menuLvl][i]);                           //  print text for this line
  }
}





//**********************************************************//
//**********************************************************//
//
//     Function: encoderButton
//
//**********************************************************//
//**********************************************************//
//  This function reads the encoder button and determines
//  if this is the falling (first) edge of the button.  If so,
//  the variable caseIndex is calculated to determine where in
//  state machine the program needs to go

void  encoderButton() {
      buttonPress = digitalRead(BUTTONPIN);                     //  Read the button
      if (!buttonPress && buttonPress != buttonPressed) {       //  If this is the falling edge, the button has just been pressed
        caseIndex = ((menuLevel) * 10) + menuPos;               //  setting case statement index;
        buttonPressed = buttonPress;                            //  make certain to avoid this code until the next button press
        firstTime = 1;                                          // Reset menu cursor to top position in the new menu
      } else if (buttonPress && buttonPress != buttonPressed) { //  If this is the rising edge, do nothing
        buttonPressed = buttonPress;
      } 
}




//**********************************************************//
//**********************************************************//
//
//     Function: doEncoder
//
//**********************************************************//
//**********************************************************//
//  This function read the encoder button and returns a value
//  within a predetermined range from sent variables.  
//  The function also does all error checking for the encoder
//  as well as the output variable.  Although written for menu 
//  position, this is a generic function anytime the encoder is needed

int doEncoder (bool newLevel, int encMin, int encMax, int menuMin, int menuMax, int menuSet) {  
  int encPos;
  int menuPosition;

  if (menuSet >= 0) {                          // Reset encoder to current menu selection
    menuPosition = menuSet;                    // Set menuPosition to the preset value
    encOne.write(menuSet * encMax / menuMax);  // force encoder to its value to produce preset value
  } 
  
  // Normal encoder read operation
    encPos = encOne.read();                    // Get current encoder value
    if (encPos < 0) {                          // check if encoder has gone below minimum value
      encPos = 0;                              // Set position to minimum value
      encOne.write(0);                         // force encoder to minimum value
    }
    else if (encPos >= encMax) {               // check if encoder has gone above maximum value
      encPos = encMax;                         // Set position to maximum value
      encOne.write(encMax);                    // force encoder to maximum value
    }
  
    menuPosition = map (encPos,encMin,encMax,menuMin,menuMax); //  map the encoder value onto the output value

  if (menuPosition < 0) {                      // check if output value has gone below minimum value
    menuPosition = 0;                          // Set output value to minimum value
  }
  else if (menuPosition >= menuMax) {          // check if output value has gone above maximum value
    menuPosition = menuMax;                    // Set output value to maximum value
  }

  return menuPosition;                         //  Return the output Value
}





//**********************************************************//
//**********************************************************//
//
//     Function: printIP
//
//**********************************************************//
//**********************************************************//
//  Print the TEENSY's IP address to the serial monitor

void printIP() {                         
  Serial.printf("My IP address: ");
  for (byte thisByte = 0; thisByte < 3; thisByte++) {  // Print the first three numbers
    Serial.printf("%i.",Ethernet.localIP()[thisByte]);
  }
  Serial.printf("%i\n",Ethernet.localIP()[3]);         //  Print the lasst number
}





//**********************************************************//
//**********************************************************//
//
//     Function: fireFireFire
//
//**********************************************************//
//**********************************************************//
void  fireFireFire() {                                 //  make fire lights run!
        for (j=0; j<=2; j++) {                         // Index through the current string
          setHue(fireBatch[j],true,HueRed,255,255);    // Set current light RED
          delay(500);                                  // Delay until next light
          fireButton.tick();                           // Check for FIREBUTTON
        }
        for (j=0; j<=2; j++) {                         // Index through the current string
          setHue(fireBatch[j],false,HueRed,255,255);   // Set current light OFF
          delay(500);                                  // Delay until next light
          fireButton.tick();                           // Check for FIREBUTTON
        }
}





//**********************************************************//
//**********************************************************//
//
//     Function: click1
//
//**********************************************************//
//**********************************************************//
void click1() {                                        // set fire lights
  fireButtonState = 1;                                 // Set indicator for FIRE
}





//**********************************************************//
//**********************************************************//
//
//     Function: LPstart
//
//**********************************************************//
//**********************************************************//
void LPstart() {                                       // clear fire lights
  fireButtonState = 0;                                 // Set indicator for FIRE
  caseIndex = 0;                                       // Go to TOP MENU
  for (j=0; j<=2; j++) {                               // Index through the current string
    setHue(fireBatch[j],true,HueRed,255,0);            // Set lights in fire string to white
  }
}
