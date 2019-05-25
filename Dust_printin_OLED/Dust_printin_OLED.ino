/*
 * 본 저작물은 '한국환경공단'에서 실시간 제공하는 '한국환경공단_대기오염정보'를 이용하였습니다.
 * https://www.data.go.kr/dataset/15000581/openapi.do
 */
//oled정의
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//wifi정의
#include <ESP8266WiFi.h> // ESP 8266 와이파이 라이브러리
#include <ESP8266HTTPClient.h> // HTTP 클라이언트
#include <Adafruit_NeoPixel.h> // 네오픽셀 라이브러리
#define NUMPIXELS      19 // 네오픽셀 LED 수
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, D0, NEO_GRB + NEO_KHZ800); // 네오픽셀 객체

//oled정의
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16

//wifi정의
String city = "서울"; // 서울, 부산, 대구, 인천, 광주, 대전, 울산, 경기, 강원, 충북, 충남, 전북, 전남, 경북, 경남, 제주, 세종 중 입력
String gu = "용산구";
String key = "";
String url = "http://openapi.airkorea.or.kr/openapi/services/rest/ArpltnInforInqireSvc/getCtprvnMesureSidoLIst?sidoName=" + city + "&searchCondition=HOUR&pageNo=1&numOfRows=200&ServiceKey=" + key;
float pm10,pm25 = 0 ; 
int score = 0 ;

//출력할 문자
String text_display = "Dust value";

static const unsigned char PROGMEM logo_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };

void setup()
{
  // 시리얼 세팅
  Serial.begin(115200);
  Serial.println();
  
 // 네오픽셀 초기화
   #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  #endif
  // End of trinket special code
 pixels.begin();
  pixels.show(); 

//와이파이 정보 
#ifndef STASSID
#define STASSID "SOYA"
#define PASS  "aa11221122"
#endif

  // 와이파이 접속
  WiFi.begin(STASSID, PASS); // 공유기 이름과 비밀번호

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) // 와이파이 접속하는 동안 "." 출력
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP()); // 접속된 와이파이 주소 출력

//OLED 정보 // 
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();

  // Draw a single pixel in white
  display.drawPixel(10, 10, WHITE);

  // Show the display buffer on the screen. You MUST call display() after
  // drawing commands to make them visible on screen!
  display.display();
  delay(2000);
  // display.display() is NOT necessary after every single drawing command,
  // unless that's what you want...rather, you can batch up a bunch of
  // drawing operations and then update the screen all at once by calling
  // display.display(). These examples demonstrate both approaches...
  
  testscrolltext();    // Draw scrolling text

  // Invert and restore display, pausing in-between
  display.invertDisplay(true);
  delay(1000);
  display.invertDisplay(false);
  delay(1000);  
}


void loop() {
  if (WiFi.status() == WL_CONNECTED) // 와이파이가 접속되어 있는 경우
  {
    WiFiClient client; // 와이파이 클라이언트 객체
    HTTPClient http; // HTTP 클라이언트 객체

    if (http.begin(client, url)) {  // HTTP
      // 서버에 연결하고 HTTP 헤더 전송
      int httpCode = http.GET();

      // httpCode 가 음수라면 에러
      if (httpCode > 0) { // 에러가 없는 경우
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = http.getString(); // 받은 XML 데이터를 String에 저장
          int cityIndex = payload.indexOf(gu);
          pm10 = getNumber(payload, "<pm10Value>", cityIndex); 
          pm25 =getNumber(payload,"<pm25Value>", cityIndex);
          //Serial.println(payload);
          Serial.println("용산구");
          Serial.println( pm10);
          Serial.println( pm25 );
        }
      } else {
        Serial.printf("[HTTP] GET... 실패, 에러코드: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
    } else {
      Serial.printf("[HTTP] 접속 불가\n");
    }
    testscrolltext();
    Serial.println(gu +"의 미세먼지 수치");
    Serial.println(pm10);
    Serial.println(pm25);  
    //네오픽셀 출력 // 
    score = getScore(); // score 변수에 대기오염점수 저장
    Serial.println(score); // 시리얼로 출력
    setLEDColor(score); // 점수에 따라 LED 색상 출력
    delay(60000);    
  }
}

float getNumber(String str, String tag, int from) {
  float num = -1;
  int f = str.indexOf(tag, from) + tag.length();
  int t = str.indexOf("<", f);
  String s = str.substring(f,t);
  
  return s.toFloat();
}

//미세먼지 수치  // 
int getScore() {
  int s = -1;
  if(pm10 > 151 || pm25>=76) //4.최악
   s = 4;
  else if (pm10 >= 101 || pm25 >= 51 ) // 3.매우 나쁨
    s = 3;
  else if (pm10 >= 51 || pm25 >= 26) // 2.나쁨
    s = 2;
  else if (pm10 >= 41 || pm25 >= 21 ) //1. 보통
    s = 1;
  else  // 0.양호
    s = 0;
    
  return s;
}

void setLEDColor(int s) {
  int color;
  if (s == 0) // 양호 진파랑
      colorWipe(pixels.Color(0, 63, 255),30);
  else if (s == 1) // 보통 연초록
      colorWipe(pixels.Color(0, 127, 63),30);
  else if (s == 2) // 나쁨 주황
      colorWipe(pixels.Color(255, 127, 0),30);
  else if (s == 3) // 매우 나쁨 연빨강 
    colorWipe(pixels.Color(255, 31, 0),30);
   else  //최악 빨강
   colorWipe(pixels.Color(255, 0, 0),30);

}


void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<NUMPIXELS ; i++) {
    pixels.setPixelColor(i, c);
    pixels.show();
    delay(wait);
  }
}

void testscrolltext(void) {
  display.clearDisplay();

  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.setCursor(10, 0);
  display.println(( String("yongsanGu") + pm10 + " " + pm25 ));
  display.display();      // Show initial text
  delay(100);

  // Scroll in various directions, pausing in-between:
  display.stopscroll();
  delay(1000);
}


