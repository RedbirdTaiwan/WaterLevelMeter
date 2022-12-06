# 自動水位計 Water Level Meter
自動水位計 Water Level Meter based on WeMos D1 R32 ESP32
![Water Level Meter](./imgs/photos.jpg)

## Hardwares
- [WeMos D1 R32 ESP32](https://www.taiwansensor.com.tw/product/wemos-d1-r32-esp-32-%E9%96%8B%E7%99%BC%E7%89%88-%E8%97%8D%E8%8A%BD-wifi-uno-r3-%E4%B8%80%E9%AB%94-4mb-%E8%A8%98%E6%86%B6%E9%AB%94/): 360 NTD
- [LCD Keypad Shield](https://create.arduino.cc/projecthub/electropeak/using-1602-lcd-keypad-shield-w-arduino-w-examples-e02d95): 150 NTD
- [SD/Micro SD Card Module](https://create.arduino.cc/projecthub/electropeak/sd-card-module-with-arduino-how-to-read-write-data-37f390): 25 NTD
- [JSN SR-04T](https://www.jmaker.com.tw/products/jsn-sr04t): 270 NTD
- 10kΩ resistor

## Wiring
![Water Level Meter layout](./imgs/layout.png)

### LCD Keypad Shield - WeMos D1 R32 ESP32
 1. Cut out pins: 3.3V, 5V, GND(1st), RESET, GND(3rd), GPIO 18,19,23,5,25,26,1,3
 2. Wiring GND & GPIO 12 with 10kΩ resistor
 3. Wiring A0 & A2
 4. Fit together the LCD Keypad Shield & the WeMos D1 R32 ESP32
 5. Connect the 5V power pin of the WeMos D1 R32 ESP32 to the ICSP power pin of the LCD Keypad Shield with a switch

### SD/Micro SD Card Module - WeMos D1 R32 ESP32
| SD Card | D1 R32 |
| -------- | -------- |
| GND | GND |
| 3.3V | 3.3V |
| CS | GPIO 5 |
| MOSI | GPIO 23 |
| SCK | GPIO 18 |
| MISO | GPIO 19 |

### JSN SR-04T - WeMos D1 R32 ESP32
| SR-04T | D1 R32 |
| -------- | -------- |
| 5V | 5V |
| RX | GPIO 25 |
| TX | GPIO 26 |
| GND | GND |

## Code

### Code 1: [waterlevel_code1.ino](waterlevel_code1.ino)
 - Adjust RTC with buttons of the LCD Keypad Shield
 - Detect the range between the JSN SR-04T and the water surface every 5 minutes
 - Go deep sleeping after detection