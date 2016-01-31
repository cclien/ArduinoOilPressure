#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

// LCD Configuration
#define BACKLIGHT_PORT 5
#define LCD_DC_PORT 6
#define LCD_CS_PORT 7
#define LCD_RST_PORT 8
Adafruit_PCD8544 display = Adafruit_PCD8544(LCD_DC_PORT, LCD_CS_PORT, LCD_RST_PORT);

#define LCD_CONTRAST 50

// Analog port the pressure sensor
#define GAUGE_PORT A0

// Sensor calibration
// 0-100 psi, 0.5-4.5v
#define PRESSURE_MIN_VALUE 0.0
#define VOLT_MIN_VALUE 0.5
#define PRESSURE_MAX_VALUE 6.89475729
#define VOLT_MAX_VALUE 4.5

// Scale of the history pressure chart
#define BAR_UPPER_PRESSURE 6.0
#define BAR_LOWER_PRESSURE 1.0

// Magic!
#define PEAK_DISPLAY_THRESHOLD 0.3
#define HIST_SAMPLES 84
#define BAR_HEIGHT 18
#define PEAK_SAMPLES HIST_SAMPLES

// generate with http://manytools.org/hacker-tools/image-to-byte-array/
static const unsigned char PROGMEM oil_pressure_icon [] = { 
0xf7, 0x0, 0x7, 0x80, 0x0, 0x0, 0x0, 0x0, 0x92, 0x0, 0x6, 0xc0, 0x0, 0x0, 0x0, 
0x0, 0xff, 0xc3, 0xc6, 0xda, 0x71, 0xce, 0xdb, 0x4e, 0x10, 0x7e, 0x6, 0xde, 0xdb, 0x18, 
0xdb, 0xdb, 0x10, 0x4, 0x47, 0x98, 0xfb, 0xde, 0xdb, 0x1f, 0x10, 0x8, 0x46, 0x18, 0xc0, 
0xc6, 0xdb, 0x18, 0x1f, 0xf0, 0x6, 0x18, 0x7b, 0x9c, 0x7b, 0xf};

//Byte array of bitmap of 24 x 7 px:
static const unsigned char PROGMEM peak_icon [] = { 
0xff, 0xff, 0x80, 0x88, 0x8a, 0x80, 0xab, 0xaa, 0x80, 0x89, 0x89, 0x80, 0xbb, 0xaa, 0x80, 
0xb8, 0xaa, 0x80, 0xff, 0xff, 0x80, 
};

int hist_bar[HIST_SAMPLES];
float hist_raw[HIST_SAMPLES];
int hist_index;

void setup() {
  // put your setup code here, to run once:
  pinMode(BACKLIGHT_PORT, OUTPUT);
  analogWrite(BACKLIGHT_PORT, 255);
  Serial.begin(9600);

  display.begin();
  display.setContrast(LCD_CONTRAST);

  for(int i=0; i<HIST_SAMPLES; i++) {
    hist_bar[i] = 0;
    hist_raw[i] = 0.0;
  }

  hist_index=0;
}

void hist_insert(float value) {
  hist_index++;
  if (hist_index >= HIST_SAMPLES)
    hist_index = 0;
  hist_bar[hist_index] = bar_scale(value);
  hist_raw[hist_index] = value;
}

int bar_scale(float pressure) {
  int value = (int) (BAR_HEIGHT*(pressure-BAR_LOWER_PRESSURE))/(BAR_UPPER_PRESSURE-BAR_LOWER_PRESSURE);
  if(value > BAR_HEIGHT)
    value = BAR_HEIGHT;
  return value;
}

float peak_value() {
  float peak=0;
  float max_diff=0;
  for (int i=0; i<PEAK_SAMPLES; i++) {
    int data_index=hist_index-i;
    if (data_index < 0)
      data_index+=HIST_SAMPLES;
      
    if (hist_raw[data_index] > peak)
      peak=hist_raw[data_index];

    if( abs(hist_raw[data_index]-peak) > max_diff) {
      max_diff = abs(hist_raw[data_index]-peak);
    }
  }

  if (max_diff > PEAK_DISPLAY_THRESHOLD) {
    return peak;
  } else {
    return 0;
  }
}

float last_peak=0;

void loop() {
  // put your main code here, to run repeatedly:
  float volt=(analogRead(GAUGE_PORT)+1)/204.8; // 1024/5.0 = 204.8
  float pressure=(volt-VOLT_MIN_VALUE)*(PRESSURE_MAX_VALUE-PRESSURE_MIN_VALUE)/(VOLT_MAX_VALUE-VOLT_MIN_VALUE);

  display.clearDisplay();
  display.drawBitmap(8, 0,  oil_pressure_icon, 64, 7, BLACK);
  display.setTextColor(BLACK);
  display.setFont(&FreeSansBold12pt7b);
  display.setCursor(5,26);
  display.print(pressure);

  hist_insert(pressure);

  float peak = peak_value();
  if (peak > 0 && peak >= last_peak) {
    display.drawBitmap(60,10, peak_icon, 17, 7, BLACK);
    display.setFont(NULL); // default font
    display.setCursor(57,18);
    display.print(peak);
  } else {
    display.setFont(&FreeSans9pt7b);
    display.setCursor(55,26);
    display.println("bar");
  }
  last_peak=peak;

  for(int i=1;i<=HIST_SAMPLES;i++) {
    int data_index=(hist_index+i) % HIST_SAMPLES;
    int data=hist_bar[data_index];
    display.drawFastVLine(i,48-data,data,BLACK);
  }

  display.display();
  delay(50);

}
