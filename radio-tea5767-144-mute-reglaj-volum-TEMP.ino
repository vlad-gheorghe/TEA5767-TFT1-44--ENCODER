// TEA5767 und Nokia 5110 LCD Display
//http://blog.simtronyx.de/en/simple-fm-stereo-radio-with-a-tea5767-breakout-module-and-an-arduino/

#include <SPI.h>

#include <Adafruit_GFX.h>
//#include <Adafruit_PCD8544.h>
#include <TFT_ILI9163C.h>
#include <Fonts/FreeSans18pt7b.h>
#define __CS 10
#define __DC 9
// D7 - Serial clock out (CLK oder SCLK)
// D6 - Serial data out (DIN)
// D5 - Data/Command select (DC oder D/C)
// D4 - LCD chip select (CE oder CS)
// D3 - LCD reset (RST)
//Adafruit_PCD8544 lcd = Adafruit_PCD8544(7, 6, 5, 4, 3);
TFT_ILI9163C lcd = TFT_ILI9163C(__CS, __DC, 8);

#include <Wire.h>

#define button_frequency_up     7
#define button_frequency_down   6
#define button_mute             5
#define button_vplus             4 //VOLUM UP TDA7496L
#define button_vminus             2 //VOLUM DOWN TDA7496L

#define TEA5767_mute_left_right  0x06
#define TEA5767_MUTE_FULL        0x80
#define TEA5767_ADC_LEVEL_MASK   0xF0
#define TEA5767_STEREO_MASK      0x80

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

int old_frequency=-1;
//int frequency=10650;
int frequency=9630;

byte old_stereo=0;
byte stereo=1;

byte old_mute=1;
byte mute=0;

byte old_signal_level=1;
byte signal_level=0;

unsigned long last_pressed;
int h=50;
int prevh = 0;
//https://create.arduino.cc/projecthub/microst/thermometer-diode-based-524613
const int in = A1;          // used to bias the diode  anode
const int t0 = 20.3;
const float vf0 = 573.44;
int i;
float dtemp, dtemp_avg, t;
//
void setup(void) {
  //Serial.begin(9600);
  analogReference(INTERNAL);
  pinMode(in, INPUT_PULLUP); 
  pinMode(button_frequency_up, INPUT);
  pinMode(button_frequency_down, INPUT);
  pinMode(button_mute, INPUT);
  pinMode(button_vplus, INPUT);
  pinMode(button_vminus, INPUT);
  digitalWrite(button_frequency_up, HIGH);
digitalWrite(button_frequency_down, HIGH);
digitalWrite(button_mute, HIGH);
digitalWrite(button_vplus, HIGH);
digitalWrite(button_vminus, HIGH);
pinMode(3, OUTPUT); //FOR PWM TDA7496L

  analogWrite(3, 50);
  Wire.begin();
  
  TEA5767_set_frequency();

  lcd.begin();
 // lcd.setContrast(60);
  lcd.fillScreen();
   lcd.setRotation(1);
  set_text(3,2,"  FM Radio TEA5767",WHITE,1);  
  //set_text(14,147,"blog.simtronyx.de",BLACK,1);
  
}


void loop() {
// Serial.println(h);
tempp();
    if(frequency!=old_frequency){
     // set_text(old_frequency>=10000?6:14,17,value_to_string(old_frequency),WHITE,2);
     // set_text(frequency>=10000?6:14,17,value_to_string(frequency),BLACK,2);
      
      lcd.setTextSize(1);
  lcd.setFont(&FreeSans18pt7b);
  lcd.setCursor(1, 41);
   lcd.setTextColor(CYAN);
   lcd.fillRect(0, 13, 119, 33, BLACK);
   lcd.print(value_to_string(frequency));
  lcd.setFont();
  //lcd.setTextSize(2);
 lcd.setCursor(107, 26);
 lcd.print("MHz");
 
      old_frequency=frequency;
      
    }
    
    TEA5767_read_data();
      
    if(old_stereo!=stereo){ 
        set_text(old_stereo?22:53,56,old_stereo?"Stereo":"    ",BLACK,1);
        set_text(stereo?22:53,56,stereo?"Stereo":"    ",ORANGE,1);
        old_stereo=stereo;
       
    }
    
    if(old_signal_level!=signal_level){
      //  set_text(old_signal_level<10?76:90,89,String((int)old_signal_level),WHITE,1);
      //  set_text(signal_level<10?76:90,89,String((int)signal_level),BLACK,1);
        old_signal_level=signal_level;
        lcd.setTextSize(1);lcd.setTextColor(YELLOW,BLACK);
 lcd.setCursor(56, 75);
 lcd.fillRect(55, 74, 40, 10, BLACK);
 lcd.print(old_signal_level);lcd.print(" /15");
 lcd.setCursor(3, 75);lcd.print("Semnal:");
        show_signal_level(signal_level);
    }
    
    if(old_mute!=mute){
        set_text(1,109,old_mute?"Mute":"    ",BLACK,1);
        set_text(1,109,mute?"Mute":"    ",RED,1);
        old_mute=mute;
    }
      
    delay(50);
      lcd.setTextSize(1);lcd.setTextColor(GREENYELLOW,BLACK);
 lcd.setCursor(57, 114); 
 lcd.print("Volum:");  
  lcd.fillRect(90, 112, 35, 11, BLACK);
 lcd.print(h/10); 
// lcd.setCursor(100, 109);
 lcd.print("/25"); 
  show_volum_level(h);
  if(digitalRead(button_frequency_down)==LOW){
    
    frequency=frequency-1;
    if(frequency<8750)frequency=10800;
    TEA5767_set_frequency();
  }
  if(digitalRead(button_frequency_up)==LOW){
    
    frequency=frequency+5;
    if(frequency>10800)frequency=8750;
    TEA5767_set_frequency();
  }

  if(digitalRead(button_mute)==LOW){
    
    TEA5767_mute();
  }
  
  delay(50);

 if (digitalRead(button_vplus) == LOW)
    {    
    h = h+10;
   
     if (h > 255) {h=254;}
analogWrite(3, h);
    delay(50);
 //Serial.println(h); 
}

 if (digitalRead(button_vminus) == LOW)
    {    
    h = h-10;
   
     if (h < 0) {h=0;}
analogWrite(3, h);
    delay(50);
  //Serial.println(h);
}
}
unsigned char frequencyH = 0;
unsigned char frequencyL = 0;

unsigned int frequencyB;

unsigned char TEA5767_buffer[5]={0x00,0x00,0xB0,0x10,0x00};

void TEA5767_write_data(byte data_size){
   
  delay(50);
  
  Wire.beginTransmission(0x60);
  
  for(byte i=0;i<data_size;i++)
    Wire.write(TEA5767_buffer[i]);
  
  Wire.endTransmission();
  
  delay(50);
}

void TEA5767_mute(){ 
  
  if(!mute){   
    mute = 1;   
    TEA5767_buffer[0] |= TEA5767_MUTE_FULL;
    TEA5767_write_data(2);
//    TEA5767_buffer[0] &= ~TEA5767_mute;
//    TEA5767_buffer[2] |= TEA5767_mute_left_right;
  }   
  else{
    mute = 0;   
    TEA5767_buffer[0] &= ~TEA5767_MUTE_FULL;
    TEA5767_write_data(2);
//    TEA5767_buffer[0] |= TEA5767_mute;
//    TEA5767_buffer[2] &= ~TEA5767_mute_left_right;
  }
    
//  TEA5767_write_data(3);
}

void TEA5767_set_frequency()
{
  frequencyB = 4 * (frequency * 10000 + 225000) / 32768;
  TEA5767_buffer[0] = frequencyB >> 8;
  if(mute)TEA5767_buffer[0] |= TEA5767_MUTE_FULL;
  TEA5767_buffer[1] = frequencyB & 0XFF;
  
  TEA5767_write_data(5);
}

int TEA5767_read_data() {
  
  unsigned char buf[5];
  memset (buf, 0, 5);
  
  Wire.requestFrom (0x60, 5); 

  if (Wire.available ()) {
    for (int i = 0; i < 5; i++) {
      buf[i] = Wire.read ();
    }
        
    stereo = (buf[2] & TEA5767_STEREO_MASK)?1:0;
    signal_level = ((buf[3] & TEA5767_ADC_LEVEL_MASK) >> 4);
    
    return 1;
  } 
  else return 0;
}

void show_signal_level(int level){
  
  byte xs=105;
  byte ys=80;
  for(int i=0;i<15;i++){
    if(i%2!=0)lcd.drawLine(xs+i,ys,xs+i,ys-i,level>=i?YELLOW:BLACK);
  }
}

void show_volum_level(int h){
  
  byte xs=1;
  byte ys=125;
  for(int i=0;i<25;i++){
   
   if(i%2!=0)lcd.drawLine(xs+2*i,ys,xs+2*i,ys-i,h/10>=i?GREENYELLOW:BLACK);
  
  }
}

void set_text(int x,int y,String text,int color,int textsize){
  
  lcd.setTextSize(textsize);
  lcd.setTextColor(color); 
  lcd.setCursor(x,y);     
  lcd.println(text);      
  //lcd.display();         
}

void tempp(){
//https://create.arduino.cc/projecthub/microst/thermometer-diode-based-524613
  dtemp_avg = 0;
  for (i = 0; i < 1024; i++) {
   // float vf = analogRead(A0) * (4976.30 / 1023.000);
   //for (int j = 0; i < 10; j++) {
     float vf = analogRead(A1) * (1079 / 1023.000);
    //Serial.println(vf);
    dtemp = (vf - vf0) * 0.4545454;
    dtemp_avg = dtemp_avg + dtemp;
  }
  t = t0 - dtemp_avg / 1024;
 // t=t;
   lcd.setCursor(75, 97);lcd.setTextColor(GREEN,BLACK);
 lcd.print(t-16);
 lcd.setCursor(60, 97);lcd.print("T:");lcd.drawCircle(109,97,2,GREEN);
 lcd.setCursor(115, 97);lcd.print("C");
 // }
}
String value_to_string(int value){
  
  String value_string = String(value / 100);
  value_string = value_string + '.' + ((value%100<10)?"0":"") + (value % 100);
  return value_string;
}

