#include <Serial.h>

// scrolltext demo for Adafruit RGBmatrixPanel library.
// Demonstrates double-buffered animation on our 16x32 RGB LED matrix:
// http://www.adafruit.com/products/420

// Written by Limor Fried/Ladyada & Phil Burgess/PaintYourDragon
// for Adafruit Industries.
// BSD license, all text above must be included in any redistribution.

#include <Adafruit_GFX.h>   // Core graphics library
#include <RGBmatrixPanel.h> // Hardware-specific library
#include <SPI.h>
#include <Ethernet.h>

// Similar to F(), but for PROGMEM string pointers rather than literals
#define F2(progmem_ptr) (const __FlashStringHelper *)progmem_ptr

#define CLK 8  // MUST be on PORTB! (Use pin 11 on Mega)
#define LAT A3
#define OE  9
#define A   A0
#define B   A1
#define C   A2


// Last parameter = 'true' enables double-buffering, for flicker-free,
// buttery smooth animation.  Note that NOTHING WILL SHOW ON THE DISPLAY
// until the first call to swapBuffers().  This is normal.
RGBmatrixPanel matrix(A, B, C, CLK, LAT, OE, false);

const char str[] PROGMEM = " disapproving headshakes";
int    textX   = matrix.width(),
textMin = sizeof(str) * -14,
hue     = 0;

byte mac[] = { 
  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };
char server[] = "10.0.2.39";
EthernetClient client;

void setup() {
  Serial.begin(9600);

  matrix.begin();
  matrix.setTextWrap(false); // Allow text to run off right edge
  matrix.setTextSize(2);
  Serial.println(Ethernet.begin(mac));
  delay(1000); //hold in order to allow dhcp to complete


}

int update() {
  int val = -2;

  //retrieve the current headshake count
  if(client.connect(server, 80)) {
    client.println("GET /headshake");
    client.println("Connection: close");
    client.println();

    delay(500); //allow spi to sync the data over

    if(client.available()) {
      val = 0;
      int temp = 0;

      while((temp = client.read()) != '\n') {
        val = val * 10 + temp - '0';
      }
//      return val; //return the int of how many headshakes have occured
    } 
    else {
      val = -1;
    }
  } 
  client.stop();
  return val;
}

#define SLEEP_MILLIS       12

#define WAITING_STATE      0
#define CONNECTING_STATE   1
#define FAILURE_STATE      2

#define WAITING_SECS       5
#define WAITING_LOOPS      ((1000 * (WAITING_SECS)) / (SLEEP_MILLIS))

#define CONNECTING_SECS    1
#define CONNECTING_LOOPS   ((1000 * (CONNECTING_SECS)) / (SLEEP_MILLIS))

int state = 0;
int loopCount = 0;
int displayedValue = 0;

void loop() {
  if(state == WAITING_STATE) {
    if(++loopCount >= WAITING_LOOPS) {
      loopCount = 0;

      if(client.connect(server, 80)) {
        client.println("GET /headshake\nConnection: close\n\n");
        state = CONNECTING_STATE;
      } else {
        state = FAILURE_STATE;
      }
    }
  } else if(state == CONNECTING_STATE) {
    if(++loopCount >= CONNECTING_LOOPS) {
      loopCount = 0;

      if(client.available()) {
        displayedValue = 0;
        int temp = 0;

        while((temp = client.read()) != '\n') {
          displayedValue = displayedValue * 10 + temp - '0';
        }

        state = WAITING_STATE;
        client.stop();
      } else {
        state = FAILURE_STATE;
      }
    }
  } else if(state == FAILURE_STATE) {
    displayedValue = -1;
    //we should handle this
  }

  // Clear background
  matrix.fillScreen(0);

  // Draw big scrolly text on top
  matrix.setTextColor(matrix.ColorHSV(hue, 255, 150, true));
  matrix.setCursor(textX, 1);
  matrix.print(displayedValue);
  matrix.print(F2(str));
  //matrix.print(Ethernet.localIP());

  // Move text left (w/wrap), increase hue
  if((--textX) < textMin) textX = matrix.width();
  hue += 7;
  if(hue >= 1536) hue -= 1536;

  //sleep for a few ms to smooth out the redraw
  delay(SLEEP_MILLIS);
}

