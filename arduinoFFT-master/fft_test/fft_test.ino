/* Arduino Code */

#include "arduinoFFT.h"
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

arduinoFFT FFT = arduinoFFT(); /* Create FFT object */


#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02

#define Theta 6.2831 //2*Pi

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN    6
#define BTN_PIN    9

// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 81
#define LED_MODE_TOWER_MAX 2
#define LED_MODE_FLOOR_MAX 2

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)


/* FFT variable */
/* These values can be changed in order to evaluate the functions */

//const uint16_t samples = 128; //This value MUST ALWAYS be a power of 2
#define samples 128
//double signalFrequency = 1023;
//double samplingFrequency = 5000;
//uint8_t amplitude = 100;

/*
  These are the input and output vectors
  Input vectors receive computed results from FFT
*/
double vReal[samples];
double vImag[samples];


/* LED strip variable */

/*
 MODE TABLE
 
 rainbowTower = 00;

 halfRainbowFloor = 10;

*/
uint8_t ledColorMode = 0;

uint8_t ledArray[9][9] = {0,};

uint8_t ledLogo[9][9] = {
  {0, 0, 0, 0, 0, 0, 0, 0, 0},
  {1, 0, 1, 0, 1, 1, 1, 0, 1},
  {1, 1, 1, 1, 1, 1, 1, 0, 1},
  {1, 1, 1, 1, 0, 0, 1, 0, 1},
  {1, 0, 1, 1, 0, 0, 1, 0, 1},
  {1, 0, 1, 1, 1, 1, 1, 1, 1},
  {1, 0, 1, 0, 1, 1, 1, 1, 1},
  {0, 0, 0, 0, 0, 0, 0, 0, 0},
  {3, 3, 3, 3, 3, 3, 3, 3, 3}
};
uint8_t colorArrayTower[LED_MODE_TOWER_MAX][9][3] = {
  //rainbow
  {
    //{255,255,255},
    {255,   0, 169},
    {255,   0,   0},
    {255, 125,   0},
    {255, 215,   0},
    {  0, 255,   0},
    {  0, 125, 255},
    {  0,   0, 255},
    {169,   0, 255},
    {255,   0, 101}
  },
  //purple to skyblue
  {
    {255,   0, 255},
    {230,   0, 255},
    {210,  50, 255},
    {150, 150, 255},
    {255, 255, 255},
    {150, 150, 255},
    { 50, 210, 255},
    {  0, 230, 255},
    {  0, 255, 255}
  }
};

uint8_t colorArrayFloor[LED_MODE_FLOOR_MAX][9][3] = {
  //blue to purple
  {
    {  0, 125, 255},
    {  0, 125, 255},
    { 25, 170, 255},
    { 45,   0, 255},
    { 90,   0, 255},
    {169,   0, 255},
    {255,   0, 170},
    {255,   0,  25},
    {255,   0,   0}
  },
  //green to red
  {
    { 38, 255,   0},
    { 94, 255,   0},
    {116, 255,   0},
    {172, 255,   0},
    {255, 247,   0},
    {255, 146,   0},
    {255,  56,   0},
    {255,  41,   0},
    {229,  32,  32}
  }/*,
  //red to orange (fire)
  {
    {255,   0,   0},
    {255,  17,   3},
    {255,  35,   7},
    {255,  53,  11},
    {255,  71,  15},
    {255,  89,  19},
    {255, 107,  23},
    {255, 125,  27},
    {255, 143,  31}
  }*/
};

// BUTTON

uint8_t buttonFlag = 0;

void setup()
{
  //Serial.begin(115200);
  //  Serial.println("Ready");

  // These lines are specifically to support the Adafruit Trinket 5V 16 MHz.
  // Any other board, you can remove this part (but no harm leaving it):
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  // END of Trinket-specific code.
  Serial.begin(9600);
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(255); // Set BRIGHTNESS to about 1/5 (max = 255)

  //showLogo();
  rainbow(1);             // Flowing rainbow cycle along the whole strip
  //theaterChaseRainbow(50); // Rainbow-enhanced theaterChase variant
}

void loop()
{
  //uint8_t sum = 0;
  uint16_t signalMax = 0;           // 최대 크기를 초기에는 0으로 설정
  uint16_t signalMin = 1024;
  uint8_t buttonValue = digitalRead(9);
  for (uint8_t i = 0; i < samples; i++)
  {
    double analogData = analogRead(A0);
    if (analogData < 1024) {               // 받아온 데이터의 값이 1024 이하일 때
      if (analogData > signalMax)         // 최대 크기 측정
        signalMax = analogData;          // 최대 크기 signalMax에 저장
      else if (analogData < signalMin)    // 최소 크기 측정
        signalMin = analogData;          // 최소 크기 sigmalMin에 저장
    }

    int peakToPeak = signalMax - signalMin;   // 최대- 최소 = 진폭값
    double volts = (peakToPeak * 5.0) / 1024;  // 전압 단위로 변환 = 소리 크기로 변환

    //Serial.println(volts);

    if (volts >= 0.69) //최저 음량 조절 기본값 0.64
    {
      vReal[i] = analogData;
    }
    else
    {
      vReal[i] = 0;
    }
    delayMicroseconds(100);
    vImag[i] = 0;
  }

  FFT.Windowing(vReal, samples, FFT_WIN_TYP_HAMMING, FFT_FORWARD);  /* Weigh data */
  FFT.Compute(vReal, vImag, samples, FFT_FORWARD); /* Compute FFT */
  FFT.ComplexToMagnitude(vReal, vImag, samples); /* Compute magnitudes */
  //PrintVector(vReal, (samples >> 1), SCL_FREQUENCY); //for processing

  uint8_t *led_val = SoundValueChangeForLED(vReal, (samples >> 1), SCL_FREQUENCY);

  //이 아래로 코드 작성 (Write led control code below here)

  if(buttonValue == 1 && buttonFlag == 1)
  {
    buttonFlag = 0;

    if (ledColorMode % 10 == LED_MODE_TOWER_MAX - 1 && ledColorMode / 10 == 0)
    {
      ledColorMode = 10; // 10: floor mode (10: floormode0, 11: floormode1, ...) 
    }
    else if (ledColorMode % 10 == LED_MODE_FLOOR_MAX - 1 && ledColorMode / 10 == 1)
    {
      ledColorMode = 0; // 00: tower mode (00: towerrmode0, 01: towermode1, ...)
    }
    else
    {
      ledColorMode += 1;
    }
  }
  else if(buttonValue == 0 && buttonFlag == 0)
  {
    buttonFlag = 1;
  }

  for (int i = 0; i < 9; i++)
  {
    clearBar(i, led_val[i]);
    setBar(i, led_val[i]);
    //Serial.print(led_val[i]);
    //Serial.print(", ");
  }
  //Serial.println();
  showLed(strip.Color(255, 255, 255));
}


/* ================== FFT function START ================== */

void PrintVector(double *vData, uint8_t bufferSize, uint8_t scaleType) //for processing
{
  for (uint16_t i = 2; i < bufferSize; i++)
  {
    uint8_t val_temp = map(vData[i], 0, 1023, 0, 255);
    Serial.write(val_temp);
  }
  Serial.write('\n');
}

uint8_t* SoundValueChangeForLED(double *vData, uint8_t bufferSize, uint8_t scaleType)
{
  static uint8_t tmp_floorVal[9] = {0,};
  for (uint16_t i = 4; i < bufferSize; i += 4)
  {
    uint8_t val_temp = map(vData[i], 0, 1023, 0, 255);
    if (i / 4 <= 9)
      tmp_floorVal[i / 4 - 1] = val_temp;
    //Serial.print(tmp_floorVal[i/7]);
    //Serial.print(", ");
  }
  //Serial.println();

  static uint8_t led_floorVal[9] = {0,};
  for (int i = 0; i < 9; i++)
  {
    led_floorVal[i] = map(tmp_floorVal[i], 0, 255, 0, 9);
    //Serial.print(led_floorVal[i]);
    //Serial.print(", ");
  }
  //Serial.println();

  return led_floorVal;
}

/* ================== FFT function END ================== */



/* ================== LED strip function START ================== */

void showLogo()
{
  rainbow(5);
  /*
  for (int color = 0; color <= 255; color++)
  {
    for (int i = 0; i < 9; i++)
    {
      for (int j = 0; j < 9; j++)
      {
        if (ledLogo[i][j] == 1)
          strip.setPixelColor(80 - (i + j * 9), color, color % 128, 255 - color);
        else
          strip.setPixelColor(80 - (i + j * 9), 0, 0, 0);
      }
    }
    strip.show();
    delay(100);
  }

  for (int i = 0; i < 9; i++)
  {
    for (int j = 0; j < 9; j++)
    {
      strip.setPixelColor(72 - i + j * 9, 0, 0, 0);
    }
  }
  strip.show();
  */
}

void setBar(int tower, int floor)
{
  for (int i = 0; i <= floor; i++)
  {
    ledArray[tower][i] = 1;
  }
}
void clearBar(int tower, int floor)
{
  for (int i = 0; i < 9; i++)
  {
    ledArray[tower][i] = 0;
  }
}
void showLed(uint32_t color) {

  int mode = ledColorMode/10;
  int model = ledColorMode%10;
  
  for (int i = 0; i < 9; i++) { // For each pixel in strip...
    for (int j = 0; j < 9; j++) {
      if (ledArray[i][j] == 1)
      {
        if ((ledArray[i][j + 1] == 0 || j == 8) && mode == 0)
          strip.setPixelColor(72 - i * 9 + j, 255, 255, 255); //  Set pixel's color (in RAM)
        else if (mode == 0)
          strip.setPixelColor(72 - i * 9 + j, colorArrayTower[model][i][0], colorArrayTower[model][i][1], colorArrayTower[model][i][2]);   //  Set pixel's color (in RAM)
        else if (mode == 1)
          strip.setPixelColor(72 - i * 9 + j, colorArrayFloor[model][j][0], colorArrayFloor[model][j][1], colorArrayFloor[model][j][2]);   //  Set pixel's color (in RAM)
      }
      else
        strip.setPixelColor(72 - i * 9 + j, 0, 0, 0); //  Set pixel's color (in RAM)
    }
  }
  strip.show();
}


// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
void rainbow(int wait) {
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this loop:
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
    // strip.rainbow() can take a single argument (first pixel hue) or
    // optionally a few extras: number of rainbow repetitions (default 1),
    // saturation and value (brightness) (both 0-255, similar to the
    // ColorHSV() function, default 255), and a true/false flag for whether
    // to apply gamma correction to provide 'truer' colors (default true).
    
    //strip.rainbow(firstPixelHue);

    rainbow2(firstPixelHue, 1, 255, 255, true);

    
    // Above line is equivalent to:
    // strip.rainbow(firstPixelHue, 1, 255, 255, true);
    strip.show(); // Update strip with new contents
    delay(wait);  // Pause for a moment
  }
}

// Rainbow-enhanced theater marquee. Pass delay time (in ms) between frames.
void rainbow2(uint16_t first_hue, int8_t reps,
  uint8_t saturation, uint8_t brightness, bool gammify) {
  for (uint16_t i=0; i<81; i++) {
    uint16_t hue = first_hue + (i * reps * 65536) / 81;
    uint32_t color = strip.ColorHSV(hue, saturation, brightness);
    if (gammify) color = strip.gamma32(color);


    if (ledLogo[i/9][i%9] == 0)
      strip.setPixelColor(80 - (i/9 + i%9 * 9), 0,0,0); //배경색
    else if (ledLogo[i/9][i%9] == 1)
      strip.setPixelColor(80 - (i/9 + i%9 * 9), 255,0,101); //글씨색
    else if (ledLogo[i/9][i%9] == 3)
      strip.setPixelColor(80 - (i/9 + i%9 * 9), color); //무지개
  }
}

/* ================== LED strip function END ================== */
