#include <Wire.h>
#include "Adafruit_TCS34725.h"

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

Adafruit_TCS34725 tcs;
Adafruit_NeoPixel pixels;

#define PIN            6	// Pin the NeoPixels are connected to
#define NUMPIXELS      71	// Number of LEDs on NeoPixel strip

#define DEBUG          0	// Debug mode
#define LED            13	// Arduino internal LED
#define NUMAVG         3	// Number of RGB measures to calculate average color
#define DELAYMEASURE   10	// Delay between average measurements
#define DELAYCHANGE    160	// Delay between changing NeoPixel colors
#define SMOOTH         8	// Smoothness for NeoPixel transitions
#define R_FACTOR       1	//
#define G_FACTOR       1	// Factors for reducing colors,
#define B_FACTOR       0.7	// I prefer a little less bluish colors

typedef struct {
  byte r;
  byte g;
  byte b;
} RGB;

/*
  We need two global variables here, as the Arduino IDE doesn't
  support own types as function returns yet
*/
RGB resultColor = {0, 0, 0};
RGB oldColor = {0, 0, 0};


/*
  Set full LED strip to one particular color
*/
void setStripColor(byte r, byte g, byte b)
{
  if (DEBUG)
  {
    Serial.println("Now setting color:");
    Serial.print("R: "); Serial.print(r); Serial.print(", ");
    Serial.print("G: "); Serial.print(g); Serial.print(", ");
    Serial.print("B: "); Serial.print(b); Serial.println("");
  }

  for (int i = 0; i < NUMPIXELS; i++)
  {
    pixels.setPixelColor(i, pixels.Color(r, g, b));
  }

  pixels.show();
}


/*
  Measure color multiple times, calculate average color
*/
void measureColor()
{
  uint16_t r[NUMAVG], g[NUMAVG], b[NUMAVG];
  unsigned long avgR = 0, avgG = 0, avgB = 0, total = 0;
  
  for (int i = 0; i < NUMAVG; i++)  
  {
    r[i] = tcs.read16(TCS34725_RDATAL);
    g[i] = tcs.read16(TCS34725_GDATAL);
    b[i] = tcs.read16(TCS34725_BDATAL);

    delay(DELAYMEASURE);

    total += r[i] + g[i] + b[i];
    avgR += r[i];
    avgG += g[i];
    avgB += b[i];
  }

  resultColor.r = avgR * 255.0 / total;
  resultColor.g = avgG * 255.0 / total;
  resultColor.b = avgB * 255.0 / total;

  if (DEBUG)
  {
    Serial.println("Measured color:");
    Serial.print("R: "); Serial.print(resultColor.r); Serial.print(", ");
    Serial.print("G: "); Serial.print(resultColor.g); Serial.print(", ");
    Serial.print("B: "); Serial.print(resultColor.b); Serial.println("");
  }
}


/*
  Set the NeoPixel color, smooth transition from previous color
*/
void colorTransition()
{
  int wait = DELAYCHANGE / SMOOTH;

  RGB tmp = {0, 0, 0};

  for (int i = 1; i <= SMOOTH; i++)
  {
    tmp.r = (oldColor.r + ((resultColor.r - oldColor.r) / SMOOTH * i)) * R_FACTOR;
    tmp.g = (oldColor.g + ((resultColor.g - oldColor.g) / SMOOTH * i)) * G_FACTOR;
    tmp.b = (oldColor.b + ((resultColor.b - oldColor.b) / SMOOTH * i)) * B_FACTOR;

    setStripColor(tmp.r, tmp.g, tmp.b);
    delay(wait);
  }
}


/*
  Configure TCS34725, NeoPixels and run LED init function
*/
void setup(void)
{
  Serial.begin(9600);

  tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_4X);
  pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

  if (tcs.begin())
  {
    Serial.println("Found TCS34725 sensor.");
    pixels.begin();
    pinMode(LED, OUTPUT);
  }
  else
  {
    digitalWrite(LED, HIGH);
    Serial.println("No TCS34725 sensor found - check your wiring.");
    delay(50);
    digitalWrite(LED, LOW);
    while (1);
  }
}


/*
  Main loop: Get color from TCS34725, then pass it to the NeoPixels
*/
void loop(void)
{
  measureColor();
  colorTransition();

  // Store current color for next transition
  oldColor = resultColor;
}
