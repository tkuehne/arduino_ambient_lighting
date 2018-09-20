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
#define WHITE_LED      1  // Neopixel with white LED? RGBW
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
  byte w;
} RGBW;

/*
  We need two global variables here, as the Arduino IDE doesn't
  support own types as function returns yet
  And for the white LED another two vari
*/
  RGBW resultColor = {0, 0, 0, 0};
  RGBW oldColor = {0, 0, 0, 0};



/*
  Set full LED strip to one particular color or white
*/
void setStripColorWithWhite(byte r, byte g, byte b, byte w)
{
  if (DEBUG)
  {
    Serial.println("Now setting color:");
    Serial.print("R: "); Serial.print(r); Serial.print(", ");
    Serial.print("G: "); Serial.print(g); Serial.print(", ");
    Serial.print("B: "); Serial.print(b); Serial.print(", ");
    Serial.print("W: "); Serial.print(w); Serial.println("");
  }

  for (int i = 0; i < NUMPIXELS; i++)
  {
    pixels.setPixelColor(i, pixels.Color(r, g, b, w));
  }

  pixels.show();
}


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
  uint16_t r[NUMAVG], g[NUMAVG], b[NUMAVG], highest, lowest;
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


  /* 
   * detect the highest value of r, g, b  
   */
  if(resultColor.r>resultColor.g){
    if(resultColor.r>resultColor.b) highest = resultColor.r;
    else highest = resultColor.b; 
  }else{
    if(resultColor.g>resultColor.b) highest = resultColor.g;
    else highest = resultColor.b;
  }


  /*
   * detect the lowest value of r, g, b
   */
  if(resultColor.r<resultColor.g){
    if(resultColor.r<resultColor.b) lowest = resultColor.r;
    else lowest = resultColor.b;
  }else{
    if(resultColor.g<resultColor.b) lowest = resultColor.g;
    else lowest = resultColor.b;
  }

  if(DEBUG){
     Serial.print("Highest: ");Serial.print(highest);Serial.print(", Lowest: ");Serial.println(lowest);
  }
  
  /*
   * detect difference between lowest and highest
   * if the difference is small only the white led will light
   */
  if(highest-lowest<10 && WHITE_LED) {
    resultColor.w = highest;
    resultColor.r = 0;
    resultColor.g = 0;
    resultColor.b = 0;
  }else{
    
    resultColor.r -= lowest;
    resultColor.g -= lowest;
    resultColor.b -= lowest;
  
    resultColor.r = 255 * resultColor.r / highest;
    resultColor.g = 255 * resultColor.g / highest;
    resultColor.b = 255 * resultColor.b / highest;
    resultColor.w = 0;
  }

  if (DEBUG)
  {
    Serial.println("Measured color:");
    Serial.print("R: "); Serial.print(resultColor.r); Serial.print(", ");
    Serial.print("G: "); Serial.print(resultColor.g); Serial.print(", ");
    Serial.print("B: "); Serial.print(resultColor.b); Serial.print(", ");
    Serial.print("W: "); Serial.print(resultColor.w); Serial.println("");
  }
}


/*
  Set the NeoPixel color, smooth transition from previous color
*/
void colorTransition()
{
  int wait = DELAYCHANGE / SMOOTH;

  RGBW tmp = {0, 0, 0, 0};

  for (int i = 1; i <= SMOOTH; i++)
  {
    tmp.r = (oldColor.r + ((resultColor.r - oldColor.r) / SMOOTH * i)) * R_FACTOR;
    tmp.g = (oldColor.g + ((resultColor.g - oldColor.g) / SMOOTH * i)) * G_FACTOR;
    tmp.b = (oldColor.b + ((resultColor.b - oldColor.b) / SMOOTH * i)) * B_FACTOR;
    tmp.w = (oldColor.w + ((resultColor.w - oldColor.w) / SMOOTH * i));

    if(WHITE_LED) setStripColorWithWhite(tmp.r, tmp.g, tmp.b, tmp.w);
    else setStripColor(tmp.r, tmp.g, tmp.b);
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
  if(WHITE_LED) pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRBW + NEO_KHZ800);
  else pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

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
