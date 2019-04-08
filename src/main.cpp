#include <Arduino.h>

/* Modified and commented by ai.rs
 * t4a_boblight
 * (C) 2014 Hans Luijten, www.tweaking4all.com
 *
 * t4a_boblight is free software and can be distributed and/or modified
 * freely as long as the copyright notice remains in place.
 * Nobody is allowed to charge you for this code.
 * Use of this code is entirely at your own risk.
 */

#include "Adafruit_NeoPixel.h"

// DEFINITIONS

#define STARTCOLOR 0x333333  // LED colors at start
#define BLACK      0x000000  // LED color BLACK

#define DATAPIN    14        // Datapin nodemcu D5--14, D6--12, D7--13
#define LEDCOUNT   112       //  Number of LEDs used for boblight
// LEDCOUNT value is local value in Arduino sketch, for hyperion it doesn't matter it sends prefx characters according to hyperion config
#define SHOWDELAY  200       // Delay in micro seconds before showing default 200
#define BAUDRATE   115200// Serial port speed, 460800 tested with Arduino Uno R3 23400 za MEGA, 115200 nano

#define BRIGHTNESS 70        // Max. brightness in %
//Hyperion sends prefix characters based on number of LEDs in config file
// e.g. for 181 LEDs it will send 0xB4 and cheksum 0xE1
// keep in mind if you are using boblight config to calculate prefix that Boblight counts diodes from 1 and Hyperion from 0
// if you have problems try +1 or -1 diodes when generating prefix characters
// values to save some time: 178 B1 E4, 180 B3E6, 181 B4E1, 182 B5E0 
//hyperion code
//_ledBuffer[3] = ((ledValues.size() - 1) >> 8) & 0xFF; // LED count high byte
//		_ledBuffer[4] = (ledValues.size() - 1) & 0xFF;        // LED count low byte
//		_ledBuffer[5] = _ledBuffer[3] ^ _ledBuffer[4] ^ 0x55; // Checksum
const char prefix[] = {0x41, 0x64, 0x61, 0x00, 0x6F, 0x3A};  // Start prefix ADA
char buffer[sizeof(prefix)]; // Temp buffer for receiving prefix data 

// Init LED strand, WS2811/WS2912 specific

// These might work for other configurations:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)

Adafruit_NeoPixel strip = Adafruit_NeoPixel(LEDCOUNT, DATAPIN, NEO_GRB + NEO_KHZ800);

int state;                   // Define current state
#define STATE_WAITING   1    // - Waiting for prefix
#define STATE_DO_PREFIX 2    // - Processing prefix
#define STATE_DO_DATA   3    // - Handling incoming LED colors

int readSerial;           // Read Serial data (1)
int currentLED;           // Needed for assigning the color to the right LED

void setAllLEDs(uint32_t color, int wait);

void setup()
{
  strip.begin();            // Init LED strand, set all black, then all to startcolor
  
  strip.setBrightness( (255 / 100) * BRIGHTNESS );
  
  setAllLEDs(BLACK, 0);
  setAllLEDs(STARTCOLOR, 5);
  setAllLEDs(BLACK, 0);
  
  Serial.begin(BAUDRATE);   // Init serial speed
  
  state = STATE_WAITING;    // Initial state: Waiting for prefix
}


void loop()
{
  switch(state)
  {
    case STATE_WAITING:                  // *** Waiting for prefix ***
      if( Serial.available()>0 )
      {
        readSerial = Serial.read();      // Read one character
        
        if ( readSerial == prefix[0] )   // if this character is 1st prefix char
          { state = STATE_DO_PREFIX; }   // then set state to handle prefix
      }
      break;
      
      
    case STATE_DO_PREFIX:                // *** Processing Prefix ***
      if( Serial.available() > sizeof(prefix) - 2 ) 
      {
          Serial.readBytes(buffer, sizeof(prefix) - 1);
          
          for( int Counter = 0; Counter < sizeof(prefix) - 1; Counter++) 
          {
            if( buffer[Counter] == prefix[Counter+1] ) 
            {
              state = STATE_DO_DATA;     // Received character is in prefix, continue
              currentLED = 0;            // Set current LED to the first one
            }
            else 
            {
              state = STATE_WAITING;     // Crap, one of the received chars is NOT in the prefix
              break;                     // Exit, to go back to waiting for the prefix
            } // end if buffer
          } // end for Counter
      } // end if Serial
      break;
      
      
    case STATE_DO_DATA:                  // *** Process incoming color data ***
      if( Serial.available() > 2 )       // if we receive more than 2 chars
      {
        Serial.readBytes( buffer, 3 );   // Abuse buffer to temp store 3 charaters
        strip.setPixelColor( currentLED++, buffer[0], buffer[1], buffer[2]);  // and assing to LEDs
      }
  
      if( currentLED > LEDCOUNT )        // Reached the last LED? Display it!
      {
          strip.show();                  // Make colors visible
          delayMicroseconds(SHOWDELAY);  // Wait a few micro seconds
          
          state = STATE_WAITING;         // Reset to waiting ...
          currentLED = 0;                // and go to LED one
          
          break;                         // and exit ... and do it all over again
      }
      break; 
  } // switch(state)
  
} // loop


// Sets the color of all LEDs in the strand to 'color'
// If 'wait'>0 then it will show a swipe from start to end
void setAllLEDs(uint32_t color, int wait)
{
  for ( int Counter=0; Counter < LEDCOUNT; Counter++ )      // For each LED
  {
    strip.setPixelColor( Counter, color );      // .. set the color

    if( wait > 0 )                        // if a wait time was set then
    {
      strip.show();                     // Show the LED color
      delay(wait);                      // and wait before we do the next LED
    } // if wait
    
  } // for Counter
  
  strip.show();                         // Show all LEDs
} // setAllLEDs

