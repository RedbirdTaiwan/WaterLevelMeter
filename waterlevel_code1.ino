// 引入函式庫
#include "RTClib.h"
#include <LCD4Bit_mod.h>
#include <SPI.h>
#include <SD.h>

RTC_DS1307 rtc;
LCD4Bit_mod lcd = LCD4Bit_mod(2);
Sd2Card card;
SdVolume volume;

//超音波測距模組JSN-SR04T
const byte trigPin = 3; //超音波測距的 觸發腳
const byte echoPin = 2; //超音波測距的 回應腳
const int maxDist = 600; //設定超音波偵測最長有效正常值, 傳回值大於此值的資料視為錯誤
const int minDist = 25; //設定超音波偵測最短有效正常值, 傳回值小於此值的資料視為錯誤

char buffrt_datetime[20];
char distance[10];
char cardsize[4] = "";
char options[7][16] = {"Year:", "Month:", "Day:", "Hour:", "Minute:", "Second:" };
int adc_key_val[5] ={30, 150, 360, 535, 760 };
int NUM_KEYS = 5;
int adc_key_in;
int key=-1;
int oldkey=-1;
int tkey = -1;
int nowtime[6] ={2023, 1, 1, 0, 0, 0 }; //RTC初始預設日期時間
int uplimit[6] ={2030, 12, 31, 23, 59, 59 }; //設定時間設定循環的最大日期
int downlimit[6] ={2023, 1, 1, 0, 0, 0 }; //設定時間設定循環的最小日期
const byte DonePin = A1; //通知TPL5110定時器關掉電源

// 取得現在時間
void gettime() {
  DateTime now = rtc.now();
  nowtime[0] = now.year();
  nowtime[1] = now.month();
  nowtime[2] = now.day();
  nowtime[3] = now.hour();
  nowtime[4] = now.minute();
  nowtime[5] = now.second();
}

//超音波測距函式
float getDistance() {
  int i = 0;
  int j = 0;
  float distance = 0;
  float mind = maxDist;
  float maxd = minDist;
  int tt = 5; // 取 5 次正常值的平均值
  for(int i=0;i<tt;i++){
    delay(150);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(125); //因為線長2.5m, 造成傳輸延遲, 需要加長trig時間, 否則一般用10~20就夠了
    digitalWrite(trigPin, LOW);
    long duration = pulseIn(echoPin, HIGH);
    float d = duration*0.034/2;
    Serial.println(d, 0);
    if( d < minDist || d > maxDist){ //檢查是否為正常值
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
    if(j > 49){ //測50次還測不到5次正常值就放棄, 輸出-9999
      return -9999;
    }
    j += 1;
  }
  return (distance-mind-maxd)/(tt-2);
}

// 測距並寫入SD卡
void logging () {
    DateTime now = rtc.now();
    sprintf(buffrt_datetime, "%04d-%02d-%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
    float d = getDistance();
    File dataFile = SD.open("water.txt", FILE_WRITE);
    if (! dataFile) {
      Serial.println("error opening water.txt");
      while (1) ;
    }
    dataFile.print(d, 1);
    dataFile.print(",");
    dataFile.println(buffrt_datetime);
    dataFile.close();
    Serial.print(d, 0);
    Serial.print(",");
    Serial.println(buffrt_datetime);
    
    lcd.clear();
    lcd.cursorTo(1,0);
    lcd.printIn(buffrt_datetime);
    lcd.cursorTo(2,0);
    (String(d)+" cm").toCharArray(distance, 10);
    lcd.printIn(distance);
    delay(1000);
}

void setup () {  
  Serial.begin(9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(DonePin, OUTPUT);
  digitalWrite(DonePin, LOW);
  lcd.init();
  
  if (! rtc.begin()) {
    lcd.clear();
    lcd.cursorTo(1,0);
    lcd.printIn("Find NO RTC");
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  DateTime now = rtc.now();
  sprintf(buffrt_datetime, "%04d-%02d-%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  
  if (!SD.begin()) {
    Serial.println("Card failed, or not present");
    lcd.clear();
    lcd.cursorTo(1,0);
    lcd.printIn("SD Card failed.");
    while (1) ;
  }
  while (!card.init(SPI_HALF_SPEED)) {
    Serial.println("Card initialization failed.");
    lcd.clear();
    lcd.cursorTo(1,0);
    lcd.printIn("SD Card failed.");
  } 
  if (!volume.init(card)) {
    Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
    return;
  }
  Serial.println("Card initialized.");
  uint32_t volumesize;
  volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
  volumesize *= volume.clusterCount();       // we'll have a lot of clusters
  volumesize *= 512;                            // SD card blocks are always 512 bytes
  Serial.print("Volume size: ");
  volumesize /= 1024;
  volumesize /= 1024;
  Serial.print(volumesize);
  Serial.println(" Mb");
  
  lcd.clear();
  lcd.cursorTo(1,0);
  lcd.printIn(buffrt_datetime);
  lcd.cursorTo(2,0);
  lcd.printIn("Card: ");
  lcd.cursorTo(2,5);
  dtostrf(volumesize, 4, 0, cardsize);
  lcd.printIn(cardsize);
  lcd.cursorTo(2,13);
  lcd.printIn(" Mb");
  logging();
  digitalWrite(DonePin, HIGH);
}

// LED按鍵訊號轉換
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

void loop () {
  delay(50);    // wait for debounce time
  adc_key_in = analogRead(0);    // read the value from the sensor  
  Serial.println(adc_key_in);
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
      rtc.adjust(DateTime(nowtime[0], nowtime[1], nowtime[2], nowtime[3], nowtime[4], nowtime[5]));
      lcd.cursorTo(2,0);
      char charBuf[4];
      sprintf(charBuf, "%d", nowtime[tkey]);
      lcd.printIn(charBuf);
    }
    else if ( key == 4){
      gettime();
      tkey += 1;
      if(tkey > 5){
        tkey = 0;
      }
      lcd.clear();
      lcd.cursorTo(1,0);
      lcd.printIn(options[tkey]);
      lcd.cursorTo(2,0);
      char charBuf[4];
      sprintf(charBuf, "%d", nowtime[tkey]);
      lcd.printIn(charBuf);
    }
    else if ( key == 3){
      showtime();
    }
    else {
      //logging();
      //digitalWrite(DonePin, HIGH);
    }
  }
}

// 於LED顯示現在時間
void showtime(){
  DateTime now = rtc.now();
  sprintf(buffrt_datetime, "%04d-%02d-%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  lcd.clear();
  lcd.cursorTo(1,0);
  lcd.printIn(buffrt_datetime);
}