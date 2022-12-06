#include <LiquidCrystal.h>
#include <ESP32Time.h>
#include "SD.h"

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
LiquidCrystal lcd(12,13,17,16,27,14); /*setup lcd keypad for WeMos D1 R32 ESP32 */
ESP32Time rtc; /* setup ESP32 onboard rtc */

char options[7][16] = {"Year:", "Month:", "Day:", "Hour:", "Minute:", "Second:" };
int adc_key_val[5] = {500, 800, 1200, 1500, 1800 };
int NUM_KEYS = 5;
int adc_key_in;
int key=-1;
int oldkey=-1;
int tkey = -1;
int nowtime[6] ={2022, 1, 1, 0, 0, 0 };
int uplimit[6] ={2030, 12, 31, 23, 59, 59 };
int downlimit[6] ={2022, 1, 1, 0, 0, 0 };
int interval = 5; /*wake up from deep sleep every 5 minutes */
int TIME_TO_SLEEP = 5;  /* Time ESP32 will go to sleep (in seconds) */
// JSN-SR04T RX = 25,  TX = 26
const byte RxPin = 25;
const byte TxPin = 26;
const int maxDist = 250;
const int minDist = 25;

void wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch (wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : 
      Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : 
      Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : 
      work();
      delay(1000);
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Going to sleep");
      delay(500);
      settimer();
      esp_deep_sleep_start();
      break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : 
      Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : 
      Serial.println("Wakeup caused by ULP program"); break;
    default :
      gettime();
      Serial.println(rtc.getTime("%Y-%m-%d %H:%M:%S"));
      Serial.printf(" Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
      showtime();
      break;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(RxPin, OUTPUT);
  pinMode(TxPin, INPUT);                                                                                                  
  lcd.begin(16, 2);
  lcd.clear();
  if(!SD.begin()){
      Serial.println("Card Mount Failed");
      return;
  }
  wakeup_reason();
}

void loop() {
  edittime();
}

// Convert ADC value to key number
int get_key(unsigned int input)
{
  int k;
  for (k = 0; k < NUM_KEYS; k++)
  {
    if (input < adc_key_val[k])
    {
      return k;
    }
  }
    
    if (k >= NUM_KEYS)
        k = -1;     // No valid key pressed
    
    return k;
}

void gettime() {
  nowtime[0] = rtc.getYear();
  nowtime[1] = rtc.getMonth()+1;
  nowtime[2] = rtc.getDay();
  nowtime[3] = rtc.getHour(true);
  nowtime[4] = rtc.getMinute();
  nowtime[5] = rtc.getSecond();
}

void edittime() {
  delay(50);    // wait for debounce time
  adc_key_in = analogRead(2);    // read the value from the sensor  
  //Serial.println(adc_key_in);
  key = get_key(adc_key_in);            // convert into key press
  if (key != oldkey)        
  {     
    oldkey = key;
    if ( key > 0 && key < 3){
      gettime();
      if( key == 1){
        nowtime[tkey] += 1;
      }
      else if(key == 2){
        nowtime[tkey] -= 1;
      }
      if(nowtime[tkey] < downlimit[tkey]){
        nowtime[tkey] = uplimit[tkey];
      } 
      else if(nowtime[tkey] > uplimit[tkey]){
        nowtime[tkey] = downlimit[tkey];
      }
      rtc.setTime(nowtime[5], nowtime[4], nowtime[3], nowtime[2], nowtime[1], nowtime[0]);
      lcd.setCursor(0,1);
      lcd.print(nowtime[tkey]);
      lcd.print("        ");
    }
    else if ( key == 4){
      tkey += 1;
      if(tkey > 6){
        tkey = 0;
      }
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(options[tkey]);
      lcd.setCursor(0,1);
      if (tkey  < 6){
        lcd.print(nowtime[tkey]);
      }
      else{
        lcd.print(interval);
      }
    }
    else if ( key == 3){
      showtime();
    }
    else if ( key == 0){
      work();
      lcd.print("Going to sleep");
      delay(500);
      settimer();
      esp_deep_sleep_start();
    }
  }
}

void showtime(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(rtc.getTime("%Y-%m-%d %H:%M"));
  lcd.setCursor(0,1);
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  lcd.print("SD Size: ");
  lcd.print(cardSize);
  lcd.print("MB");
}

void settimer(){
  gettime();
  TIME_TO_SLEEP = (1+(int)(nowtime[4]/interval))*interval*60-(nowtime[4]*60+nowtime[5]);
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
}

void work(){
  Serial.print("Wakeup at ");
  Serial.println(rtc.getTime("%Y-%m-%d %H:%M:%S"));
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(rtc.getTime("%Y-%m-%d %H:%M:%S"));
  float d = getDistance();
  File file = SD.open("/waterlevel.txt", FILE_APPEND);
  if(!file){
    Serial.println("Failed to open file for writing");
    lcd.setCursor(0,1);
    lcd.print("SD card failed");
  }else{
    file.print(d, 0);
    file.print(",");
    file.println(rtc.getTime("%Y-%m-%d %H:%M:%S"));
    file.close();
  }
  lcd.setCursor(0,1);
  lcd.print("Dist.= ");
  lcd.print(d, 0);
  lcd.print(" cm     ");
}


float getDistance() {
  int i = 0;
  int j = 0;
  float distance = 0;
  float mind = maxDist;
  float maxd = minDist;
  int tt = 10; /* 10 times tried for average ranging */
  for(int i=0;i<tt;i++){
    delay(150);
    digitalWrite(RxPin, HIGH);
    delayMicroseconds(125); //因為線長2.5m, 造成傳輸延遲, 需要加長trig時間, 否則一般用10~20就夠了
    digitalWrite(RxPin, LOW);
    long duration = pulseIn(TxPin, HIGH);
    float d = duration*0.034/2;
    Serial.println(d, 0);
    if( d < minDist || d > maxDist){ //deBug
      i -= 1;
    }else{
      distance += d;
      if(d < mind){
        mind = d;
      }
      if(d > maxd){
        maxd = d;
      }
    }
    if(j > 19){
      return -9999;
    }
    j += 1;
  }
  return (distance-mind-maxd)/(tt-2);
}
