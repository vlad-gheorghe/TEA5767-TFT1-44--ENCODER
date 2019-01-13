/* name      : fm_tea5767_21.ino
   purpose   : FM stereo receiver using Arduino Uno / Arduino Nano and TEA5767
   author    : LZ2WSG Veselin Georgiev
   site      : http://kn34pc.com/construct/lz2wsg_arduino_fm_rx_tea5767.html
   date      : 09-02-2016
   revision  : v1.21
   tnx       : LZ2DVM Desislav Michev, Encho Hristov
   used info : https://youtu.be/WP7kT-ZUa4E
             : http://www.ad7c.com/projects/ad9850-dds-vfo/
  ---------------------------------------------------------------------------------------------------------*/
#include <Wire.h>
//#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <rotary.h>
#include <Adafruit_GFX.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <TFT_ILI9163C.h>
#include <Fonts/FreeSans18pt7b.h>
#define __CS 10
#define __DC 9


TFT_ILI9163C display = TFT_ILI9163C(__CS, __DC, 8);

// Color definitions
#define  BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define ORANGE          0xFD20
#define GREENYELLOW     0xAFE5
#define DARKGREEN       0x03E0

//LiquidCrystal lcd(12, 13, 7, 6, 5, 4);       // LCD display pins: RS(4), E(6), D4(11), D5(12), D6(13), D7(14)
Rotary r = Rotary(2, 3);                     // encoder pins: CLK, DT

int pin_sw = 7;                              // encoder push button: MANUAL Tune / MEMORY
//#define PIN_MUTE 6  
unsigned char frequencyH = 0;
unsigned char frequencyL = 0;
unsigned int frequencyB;
unsigned int frequency;
unsigned int old_frequency;
unsigned int old_manual_frequency;
double freq_ic = 1;                          // tea5767 frequency
byte ftm;                                    // relative frequency to save in EEPROM: (F - 82.5)[MHz] * 10
unsigned long mls = 0;                       // time variable
unsigned long vr = 0;                        // time variable
unsigned char buf[5];                        // i2c buffer
byte count;                                  // MEMORY counter
byte old_count;                              // MEMORY old state counter
//boolean mute = false;                        // MUTE flag
byte button_state = 0;                       // button flag
boolean mode;                                // TUNE [0] or MEMORY [1] mode
byte sm_buf;                                 // signal-meter buffer
byte sm_p;                                   // signal-meter
/*byte graph [8][8] = {                        // array of symbols for s-meter
  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x00},  // 1 line
  {0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x00},  // 2 lines
  {0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x00},  // 3 lines
  {0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x00},  // 4 lines
  {0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00},  // 5 lines
  {0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00},  // 6 lines
  {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00},  // 7 lines
  {0x1F, 0x1F, 0x1B, 0x11, 0x1B, 0x1F, 0x1F, 0x00}   // +
};
*/
//---------------------------------------------------------------------------------------------------------
void setup()
{
  Wire.begin();
  pinMode(pin_sw, INPUT);
 // pinMode(PIN_MUTE, INPUT);
  display.begin();
  display.fillScreen();
   display.setRotation(1);
  display.fillScreen();
   display.setTextSize(1);
  display.setTextColor(WHITE,BLACK);
  display.setCursor(0,0);
  display.println("Adaptat Vlad Gheorghe");
  
  delay(800);
   display.fillScreen();
  //lcd.begin(16, 2);
  //for (int k = 0; k < 8; k++)
  //  lcd.createChar(k, graph[k]);

  //lcd.clear();
  PCICR |= (1 << PCIE2);
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
  sei();

/*  if (digitalRead(pin_sw) == LOW) {          // reset default (feel 0xFF (108 MHz) in all cells in EEPROM)
    vr = millis();                           // push 'encoder button' and turn 'power on' over 5 sec
    while (1) {
      if (digitalRead(pin_sw) == HIGH)
        break;
      if ((millis() - vr) > 5500)
        break;
    }
    if ((millis() - vr) > 5000) {
    //  display.fillScreen();
      display.setCursor(0,0);
  display.println(" RESET DEFAULT! ");
   //   lcd.clear();
   //   lcd.setCursor(0, 0);
   //   lcd.print(" RESET DEFAULT! ");
      for (int i = 0; i < 17; i++)
        EEPROM.write(i, 0xFF);
      delay(2000);
    }
  }
  */
  mode = true;
  count = 1;
  old_count = 1;
  old_frequency = 0;
  old_manual_frequency = 1011;
  ftm = EEPROM.read(1);
  frequency = ftm + 825;
  mls = millis();
}
//---------------------------------------------------------------------------------------------------------
void loop()
{
  if (mode) {                                // receiver is in MEMORY mode
    if (count < 1)
      count = 16;
    if (count > 16)
      count = 1;

    if (count != old_count) {
      ftm = EEPROM.read(count);              // read frequency from EEPROM
      frequency = ftm + 825;                 // relative frequency, stored in EEPROM to real frequency * 10
      old_count = count;
    }
  }

  if (frequency != old_frequency) {
    set_freq();
    read_status();
    old_frequency = frequency;
  }
 //if (read_wbutton(PIN_MUTE) > 0) {          // MUTE button
  //  mute = !mute;                            // MUTE ON/OFF
  //  set_freq();
  //  delay(200);
 // //  read_status();
 // }

  if ((millis() - mls) < 3000) {             // several readings on the start
    set_freq();
    read_status();
    delay(200);
  }
  tune_mode();
}
//---------------------------------------------------------------------------------------------------------
ISR(PCINT2_vect)                             // encoder events
{
  unsigned char result = r.process();

  if (result) {
    if (result == DIR_CW) {
      if (mode)
        count++;                             // inc MEMORY + 1
      else
        frequency++;                         // inc frequency + 0.1 MHz
    }
    else {
      if (mode)
        count--;                             // dec MEMORY - 1
      else
        frequency--;                         // dec frequency - 0.1 MHz
    }
  }

  if (frequency < 876) frequency = 876;      // lower end of FM band
  if (frequency > 1080) frequency = 1080;    // top end of FM band
}
// --------------------------------------------------------------------------------------------------------
void set_freq()                              // set TEA5767 frequency by i2c bus
{
  frequencyB = 4 * (frequency * 100000 + 225000) / 32768;
  frequencyH = frequencyB >> 8;
  frequencyL = frequencyB & 0xFF;
  Wire.beginTransmission(0x60);
 // if (mute)
 //   frequencyH |= 0b10000000;                // MUTE ON, data byte 1, byte 7
 // else
 //   frequencyH &= ~0b10000000;               // MUTE OFF, data byte 1, byte 7
  Wire.write(frequencyH);
  Wire.write(frequencyL);
  Wire.write(0xB0);
  Wire.write(0x10);                          // XTAL 32.768 kHz
  Wire.write(byte(0x00));                    // deemphasis 50 us (Europe and Australia)
  //  Wire.write(byte(0x40));                  // deemphasis 75 us (North America)
  Wire.endTransmission();
}
//---------------------------------------------------------------------------------------------------------
void read_status()                           // read i2c bus: frequency, mono/stereo and signal (agc)
{
  Wire.requestFrom(0x60, 5);
  if (Wire.available()) {
    for (int i = 0; i < 5; i++)
      buf[i] = Wire.read();

    freq_ic = (((buf[0] & 0x3F) << 8) + buf[1]) * 32768 / 4 - 225000;
    //lcd.setCursor(0, 0);
    //lcd.print("FM ");
    display.setTextSize(1);
  display.setCursor(1, 0);
  display.setTextColor(WHITE, BLACK);
  display.print(" Radio FM cu TEA5767");
   display.setCursor(1, 115);
  display.setTextColor(WHITE, BLACK);
   display.print("Vlad Gheorghe -2019-");
  display.setTextSize(1);
  display.setFont(&FreeSans18pt7b);
    freq_ic = round(freq_ic / 100000);
   // lcd.print(freq_ic / 10, 1);
  //  lcd.print(" MHz  ");
  display.fillRect(0, 11, 109, 35, BLACK);
   display.setCursor(3, 39);
   display.setTextColor(CYAN);
 display.print(freq_ic / 10, 1);
 display.setFont();
 display.setTextSize(2);
 display.setCursor(90, 22);
 display.print("MHz");
   // lcd.setCursor(13, 0);                    // set STEREO/MONO indicator
 printpost(frequency);
  display.setTextSize(1);
  display.setCursor(71,70);
  display.setTextColor(GREEN, BLACK);
   
    if (buf[2] & 0x80)
     // lcd.print("[S]");
       display.print("Stereo"); 
    else
    //  lcd.print("[Mono  ]");
   // display.setTextColor(RED, BLACK);
display.print("Mono  "); 
display.fillRect(0, 69, 30, 11, BLACK);

 display.setTextSize(1);
  display.setCursor(5,100);
  display.setTextColor(CYAN, BLACK);
  sm_buf = (buf[3] >> 4);
    sm_p = byte(0.6 * sm_buf);
display.print("Semnal: "); display.print(sm_p); display.print(" /10");
show_signal_level(sm_p);
/*
    lcd.setCursor(3, 1);                     // draw signal-meter bar indicator
    sm_buf = (buf[3] >> 4);
    sm_p = byte(0.6 * sm_buf);
    for (int j = 0; j < sm_p; j++) {
      lcd.setCursor(3 + j, 1);
      if (j < 6)
        lcd.write(byte(j));
      else if (j < 7)
        lcd.write(byte(6));
      else
        lcd.write(byte(7));
    }
    for (int j = sm_p; j < 10; j++)          // blank left spaces
      lcd.print(' ');

    lcd.setCursor(13, 1);                    // signal-meter indicator in a number
    lcd.print('[');
    lcd.print(sm_p);
    lcd.print(']');
    */
  }
}



//---------------------------------------------------------------------------------------------------------
void tune_mode()                             // MEMORY/TUNE (manual) or SAVE mode
{
  if (mode) {
   // lcd.setCursor(0, 1);
    display.setTextSize(1);
  display.setCursor(1, 50);
  display.setTextColor(GREEN, BLACK);
 display.print("Mem:");
    if (count < 10)
     // lcd.print('0');
      display.print('0');
display.print(count);
 display.print(' ');     
   // lcd.print(count);
   // lcd.print(' ');
  }
  else  {
   // lcd.setCursor(0, 1);
  //  lcd.print("TU ");
  display.setCursor(1, 70);
  display.setTextColor(WHITE, BLACK);
   display.print("TUNE");  
  }

  button_state = read_wbutton(pin_sw);       // MEMORY or TUNE (manual) mode
  if (button_state == 1) {
    mode = !mode;
    if (mode) {
      old_manual_frequency = frequency;
      frequency = ftm + 825;
    }
    else
      frequency = old_manual_frequency;
  }

  if ((button_state == 2) && (!mode)) {      // save current TUNE frequency to MEMORY(COUNT)
    ftm = byte(frequency - 825);             // 108.0 MHz - 255 steps * 0,1 MHz = 82.5 MHz
    EEPROM.write(count, ftm);

   // lcd.setCursor(0, 1);
   // lcd.print("   SAVE to ");
   display.setCursor(1, 50);
  display.setTextColor(WHITE, BLACK);
   display.print("   SAVE to ");
    if (count < 10)
    //  lcd.print('0');
   // lcd.print(count);
   // lcd.print("   ");
 display.setTextColor(WHITE, BLACK);
   display.print('0');
    display.print(count);
     display.print("   ");
    
    delay(2000);
    display.fillRect(19, 49, 90, 10, BLACK);
    read_status();
    mode = HIGH;
    old_manual_frequency = frequency;
  }
}
//---------------------------------------------------------------------------------------------------------
int read_wbutton (int pin_wbutton)           // push button state, return: 0(very fast), 1(fast), 2(slow)
{
  unsigned long vreme_start = 0;
  unsigned long vreme_button = 0;
  int wposition = 0;

  vreme_start = millis();
  while (digitalRead(pin_wbutton) == LOW) {
    vreme_button = millis() - vreme_start;
    if (vreme_button > 800)
      break;
  }
  if (vreme_button < 20)                     // 'nothing' for debounce
    wposition = 0;
  else if (vreme_button > 300)               // slow push button
    wposition = 2;
  else wposition = 1;                        // fast push button
  return wposition;
}

void printpost(float frequency)
{
 // switch(current_freq)
  {
    
    if (frequency==1011) { display.setCursor(25,85);
   display.setTextSize(1);
   display.setTextColor(YELLOW,BLACK);display.print("Actualitati");}
   
    if (frequency==1031) { display.setCursor(25,85);
   display.setTextSize(1);
   display.setTextColor(YELLOW,BLACK);display.print("Cultural   ");}
   
    if (frequency==1050) { display.setCursor(25,85);
   display.setTextSize(1);
   display.setTextColor(YELLOW,BLACK);display.print("Viva       ");}
   
    if (frequency==1065) { display.setCursor(25,85);
   display.setTextSize(1);
   display.setTextColor(YELLOW,BLACK);display.print("Europa FM  ");}
   
    
if (frequency==884) { display.setCursor(25,85);
   display.setTextSize(1);
   display.setTextColor(YELLOW,BLACK);display.print("Impact FM   ");}
   
   if (frequency==9110) { display.setCursor(25,85);
   display.setTextSize(1);
   display.setTextColor(YELLOW,BLACK);display.print("Magic FM   ");}
   
   if (frequency==919) { display.setCursor(25,85);
   display.setTextSize(1);
   display.setTextColor(YELLOW,BLACK);display.print("KISS FM     ");}
   
   if (frequency==949) { display.setCursor(25,85);
   display.setTextSize(1);
   display.setTextColor(YELLOW,BLACK);display.print("Radio HIT   ");}
   
   if (frequency==963) { display.setCursor(25,85);
   display.setTextSize(1);
   
   display.setTextColor(YELLOW,BLACK);display.print("Radio Iasi  ");}
   
   if (frequency==979) { display.setCursor(25,85);
   display.setTextSize(1);
   display.setTextColor(YELLOW,BLACK);display.print("RFI          ");}
   
   if (frequency==992) { display.setCursor(25,85);
   display.setTextSize(1);
   display.setTextColor(YELLOW,BLACK);display.print("DIGI FM      ");}
   
   //else
   //{ display.setCursor(5,80);
  // display.setTextSize(1);
   
   //display.setTextColor(YELLOW,BLACK);display.print("Necunoscut");}
    
 
 } 
}

void show_signal_level(int sm_p){
  
  byte xs=100;
  byte ys=105;
  for(int i=0;i<10;i++){
    if(i%2!=0)display.drawLine(xs+i,ys,xs+i,ys-2-i,sm_p>=i?WHITE:BLACK);
  }}
//---------------------------------------------------------------------------------------------------------
