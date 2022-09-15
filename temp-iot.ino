#include <Wire.h>
#include <WiFi.h>
#include "AE_SHT35.h"
#include <Adafruit_BMP085.h>


const char ssid[]  = "senser-wifi";   // Wifi SSID
const char pass[] = "12345678";  // Wifi PW
const IPAddress ip(192,168,11,3);
const IPAddress subnet(255,255,255,0);

WiFiClient client;
WiFiServer server(80);

Adafruit_BMP085 bmp;

float temp;
float humi;
int pressure;
String HtmlSet(void);

// SHT35のI2Cアドレスを設定
AE_SHT35 SHT35 = AE_SHT35(0x45);

void setup() {
  Serial.begin(9600);  // シリアル通信を9600bpsに設定
  SHT35.SoftReset();  // SHT35リセット
  SHT35.Heater(0);  // 内蔵ヒーター 0:OFF 1:ON

  WiFi.softAP(ssid, pass); //WiFiのアクセスポイントの設定
  WiFi.softAPConfig(ip, ip, subnet); //アクセスポイントのIP及びサブネットマスク
  server.begin(); //サーバーの開始 
  bmp.begin();
}



void loop()
{

  SHT35.GetTempHum();  // SHT35から温湿度データを取得
  Serial.println("--------------------------");
  Serial.println("温度 ");  // SHT35.Temperature()　より温度データ取得
  Serial.print(SHT35.Temperature());
  temp = SHT35.Temperature();
  Serial.println("℃");
  Serial.println("湿度　");  // SHT35.Humidity()　より相対湿度データ取得
  Serial.print(SHT35.Humidity());
  humi = SHT35.Humidity();
  Serial.println("％");
  Serial.println("気圧　"); 
  Serial.print(bmp.readPressure()/100);
  Serial.println(" hPa");
  pressure = bmp.readPressure()/100;

  WiFiClient client = server.available();

  if( client ){
    String str = "";
    Serial.println("New Client.");

    while(client.connected()){
      if(client.available()){
        char c = client.read();
        Serial.write(c);
        if( c == '\n'){ //改行コード 0x0A
          if( str.length() == 0 ){ //クライアントからの文字列が終わった後、以下の処理
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            client.print(HtmlSet()); //Htmlを送信
            client.println();
            client.stop(); //クライアント接続を停止
            Serial.println("Client Disconnected.");
          }else{
            str = "";
          }
        }
        else if( c != '\r'){ //復帰コード 0x0D
          str += c;
        }
      }
    }
  }
  delay(3000);



}


//HTML
String HtmlSet(void){
  String str = "";

  str += "<html lang=\"ja\">";
  str += "<head>";
  str += "<meta http-equiv=\"refresh\" content=\"5\">";
  str += "<meta charset=\"UTF-8\">";
  str += "<title>Sensor SHT35</title>";
  str += "</head>";
  str += "<body>";
  str += "<h1>温湿度センサ</h1>";
  str += "<h2>温度: ";
  str += temp;
  str += "℃";
  str += "</h2>";
  str += "<h2>湿度: ";
  str += humi;
  str += "%RH";
  str += "<h2>大気圧: ";
  str += pressure;
  str += "hPa";
  str += "</h2>";
  str += "</body>";
  str += "</html>";

  return str;
}
