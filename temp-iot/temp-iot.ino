#include <Wire.h>
#include <WiFi.h>
#include "AE_SHT35.h"
#include <Adafruit_BMP085.h>
#include <WebServer.h>
#include <ESPmDNS.h>

static constexpr int tape_LED = D3;
static constexpr int LED_RED = D8;
static constexpr int LED_YEL = D9;
static constexpr int LED_BLU = D10;
static constexpr int POMP = D7;

//const char ssid[]  = "senser-wifi";   // Wifi SSID
//const char pass[] = "12345678";  // Wifi PW
const char ssid[]  = "Buffalo-G-2B98";   // ルータ SSID
const char pass[] = "peibmpvxutakr";  // ルータ PW

const IPAddress ip(192, 168, 11, 41);
//const IPAddress gateway(192,168,11,1);
const IPAddress subnet(255, 255, 255, 0);
//const IPAddress dns(192,168,11,1);

//WiFiClient client;
WebServer server(80);

Adafruit_BMP085 bmp;

float temp;
float humi;
double pressure;
double seapressure;
String HtmlSet(void);

// SHT35のI2Cアドレスを設定
AE_SHT35 SHT35 = AE_SHT35(0x45);

void setup() {
  pinMode(tape_LED, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YEL, OUTPUT);
  pinMode(LED_BLU, OUTPUT);
  pinMode(POMP, OUTPUT);
  Serial.begin(9600);  // シリアル通信を9600bpsに設定
  SHT35.SoftReset();  // SHT35リセット
  SHT35.Heater(0);  // 内蔵ヒーター 0:OFF 1:ON  

  delay(100);
  //WiFi.softAP(ssid, pass); //WiFiのアクセスポイントの設定
  //WiFi.softAPConfig(ip, ip, subnet); //アクセスポイントのIP及びサブネットマスク
  WiFi.mode(WIFI_STA); //STAモード
  WiFi.begin(ssid, pass);
  MDNS.begin("esp32");
  server.on("/", handleRoot);
  server.begin(); //サーバーの開始
  bmp.begin();
  configTime(9 * 3600L, 0, "ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp");//NTPの設定

struct tm timeInfo;//時刻を格納するオブジェクト

}

void loop() {

  server.handleClient(); //出力
  watering();
  lighting();


}


struct tm timeInfo;

void lighting(){
  int lighttime = 0;
  getLocalTime(&timeInfo);
  lighttime = timeInfo.tm_hour;
  //Serial.println(lighttime);
  if(lighttime >= 8 && lighttime <= 19){ //8時～19時
    digitalWrite(tape_LED, HIGH);
    //Serial.println("lightON");
    delay(100);
  }
  else{
    digitalWrite(tape_LED, LOW);
    //Serial.println("lightOFF");
  }
  
}

int mois = 0;

void watering(){
  int noon = 0;
  getLocalTime(&timeInfo);
  noon = timeInfo.tm_hour;
  //Serial.println(noon);
  
  if(noon > 12){
    Moisture();
    if(mois == 1){
      digitalWrite(POMP, HIGH);
     //Serial.println("ON");
      delay(5000);
      digitalWrite(POMP, LOW);
      //Serial.println("OFF");
      mois = 0;
    }
  }
  delay(10000);
}


String Moisture() {
  String moisture = "\n";
  const int AirValue = 3800;
  const int WaterValue = 1100;
  int intervals = (AirValue - WaterValue) / 3;
  int soilMoistureValue = 0;
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_YEL, LOW);
  digitalWrite(LED_BLU, LOW);
  soilMoistureValue = analogRead(A0); //土壌にセンサーを差し込みます。
  
  if (soilMoistureValue > WaterValue && soilMoistureValue < (WaterValue + intervals)) {
    moisture = "とても湿っています";
    digitalWrite(LED_BLU, HIGH);
  }
  else if (soilMoistureValue > (WaterValue + intervals) && soilMoistureValue < (AirValue - intervals)) {
    moisture = "湿っています";
    digitalWrite(LED_YEL, HIGH);
  }
  else if (soilMoistureValue < AirValue && soilMoistureValue > (AirValue - intervals)) {
    moisture = "乾いています";
    digitalWrite(LED_RED, HIGH);
    mois = 1;
  }
  return moisture;
}



char s[20];//文字格納用
struct tm timeInfo2;

//HTML
void handleRoot() {
  SHT35.GetTempHum();  // SHT35から温湿度データを取得
  temp = SHT35.Temperature();
  humi = SHT35.Humidity();
  pressure = double(bmp.readPressure()) / 100; //BMP180　現在地データ取得
  seapressure = double(bmp.readSealevelPressure(8.2)) / 100; //BMP180　海面気圧データ取得
  getLocalTime(&timeInfo2);

  String mes = "\
  <html lang=\"ja\">\n\
  <meta charset=\"utf-8\">\n\
  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n\
  <head>\n\
   <meta http-equiv=\"Refresh\" content=\"60\">\n\
   <title>環境ロガー</title>\n\
  </head>\n\
  <body style=\"font-family: sans-serif; background-color ;\" >\n\
  <h1>"
               + String(timeInfo2.tm_year + 1900)
               + ":"
               + String(timeInfo2.tm_mon + 1)
               + ":"
               + String(timeInfo2.tm_mday)
               + "  "
               + String(timeInfo2.tm_hour)
               + ":"
               + String(timeInfo2.tm_min)
               + "</h1>\n\
  <h1>温度　"
               + String(temp)
               +"℃</h1>\n\
  <h1>湿度　"
               + String(humi)
               +"%</h1>\n\
  <h1>現地気圧　"
               + String(pressure)
               +"hPa</h1>\n\
  <h1>海面気圧　"
               + String(seapressure)
               + "hPa</h1>\n\
  <h1>土壌湿度　"
               + Moisture()
               + "</h1>\n\
    </body>\n\
  </html>\n";
  server.send(200, "text/html", mes);

}
