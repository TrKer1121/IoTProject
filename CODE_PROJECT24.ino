/***************KHAI BÁO THƯ VIỆN***************/
#include "DHT.h"
#include <SoftwareSerial.h>

/***************CÀI ĐẶT WIFi***************/
String ssid = "userwf";
String pass = "passwf";

/***************KHAI BÁO API THINGSPKEAK***************/
String ThingspeakAdd = "api.thingspeak.com"; 
String APIwrite = "4K67TA4TRMJJY7O8"; 
String ip = "184.106.153.149";

/***************KẾT NỐI ESP-UNO R3***************/
int rxPin = 11;
int txPin = 10;

SoftwareSerial esp(rxPin, txPin);

/***************ĐỊNG NGHĨA CẢM BIẾN ĐỘ ẨM, NHIỆT ĐỘ DHT11***************/
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
float temp, humi;

/***************ĐỊNH NGHĨA CẢM BIẾN SIÊU ÂM HC-SR04***************/
#define TRIG 8
#define ECHO 7
#define TIME_OUT 5000
int distance;

/***************ĐỊNH NGHĨA CẢM BIẾN HÔNG NGOẠI***************/
#define irPIN 4
int ir_data = 0;

/***************ĐỊNH NGHĨA CẢM BIẾN ÁNH SÁNG***************/
#define ldrPIN A0
int light = 0;
int ldr_raw;

/****************/
#define led 9

/***************KHAI BÁO BIẾN KHÁC***************/
long writeTime = 10; 
long start_WriteTime = 0;
long elapsed_WriteTime = 0;
boolean error;
int wfstt;


void setup() {
  Serial.begin(9600);
  dht.begin();
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode (led, OUTPUT);
  
  Serial.println("START");

  esp.begin(115200);
  start_WriteTime = millis();
  espData("AT", 0);
  while (!esp.find("OK")) {                                  
    esp.println("AT");
    Serial.println("ESP8266 Not Find.");
  }
  Serial.println("OK Command Received");
  espData("AT+RST", 0);
  espData("AT+CWMODE=1", 0);                           
  while (!esp.find("OK")) {                                  
    esp.println("AT+CWMODE=1");
    Serial.println("Setting is ....");
  }
  
  Serial.println("Connect to the Network");
  ConnectWIFI();
}

void loop() {
  start:
  error = 0;
  elapsed_WriteTime = millis() - start_WriteTime;
  if (elapsed_WriteTime > (writeTime * 1000))
  {
    read_dht11();
    read_hcsr04();
    read_ir();
    read_ldr();
    startThingspeak();
    updateThingspeak();
    start_WriteTime = millis();
  }
  if (error == 1) {
    Serial.println(" <<<< RECONNECT >>>>");
    delay (1000);
    goto start; 
  }
} 

/***************TẬP LỆNG AT CHO ESP01***************/
String espData(String command, const int timeout)
{
  Serial.print("AT Command ==> ");
  Serial.print(command);
  Serial.println("     ");

  String response = "";
  esp.println(command);
  long int time = millis();
  while ( (time + timeout) > millis())
  {
    while (esp.available())
    {
      char c = esp.read();
      response += c;
    }
  }
  return response;
}

/***************KẾT NỐI WIFI***************/
void ConnectWIFI(void) {
  espData("AT+CWJAP=\"" + ssid + "\",\"" + pass + "\"", 500);
  delay(2000);
  Serial.print("Connecting..");
  while(!esp.find("OK")){
     Serial.print('.');
     digitalWrite(led, HIGH);
     delay(100);
     digitalWrite(led, LOW);
     delay(100);
  }
  Serial.println("Connected");
  digitalWrite(led, HIGH);
  wfstt = 1;
}

/*-------------ĐỌC DỮ LIỆU CẢM BIẾN-------------*/
/***************CẢM BIẾN DHT11***************/
void read_dht11(void) {
  temp = dht.readTemperature();
  humi = dht.readHumidity();
}

/***************CẢM BIẾN SIÊU ÂM HC-SR04***************/
void read_hcsr04() {
  unsigned long duration;
  digitalWrite(TRIG, LOW);  
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); 
  delayMicroseconds(10);   
  digitalWrite(TRIG, LOW);
  duration = pulseIn(ECHO, HIGH, TIME_OUT);
  distance = int(duration / 2 / 29.412);
}

/***************CẢM BIẾN HỒNG NGOẠI***************/
void read_ir() {
  ir_data = digitalRead(irPIN);
}

/***************CẢM BIẾN ÁNH SÁNG***************/
void read_ldr() {
  ldr_raw = analogRead(ldrPIN);
  light = map(ldr_raw, 1023, 400, 0, 100);
}

/***************HIỂN THỊ DỮ LIỆU QUA SERIAL***************/
void display_serial() {
  Serial.println("==================");
  Serial.print("Temperature: "); Serial.print(temp); Serial.println("*C");
  Serial.print("Humidity: "); Serial.print(humi); Serial.println("%");
  Serial.print("Distance: "); Serial.print(distance); Serial.println("Cm");
  Serial.print("Infrared: "); Serial.println(ir_data);
  Serial.print("Light: "); Serial.print(light); Serial.println("%");
  
}

/***************KHỞI TẠO THINGSPEAK***************/
void startThingspeak() {
  esp.flush();
  esp.println("AT+CIPSTART=\"TCP\",\"" + ip + "\",80");
  if (esp.find("Error")) {
    Serial.println("AT+CIPSTART ERROR");
    return;
  }
}

/***************CẬP NHẬT DỮ LIỆU THINGSPEAK***************/
void updateThingspeak() {
  String sendData = "GET https://api.thingspeak.com/update?api_key=" + APIwrite;
  sendData += "&field1=";
  sendData += String(temp);
  sendData += "&field2=";
  sendData += String(humi);
  sendData += "&field3=";
  sendData += String(distance);
  sendData += "&field4=";
  sendData += int(wfstt);
  sendData += "&field5=";
  sendData += int(light);
  sendData += "&field6=";
  sendData += int(ir_data);
  sendData += "\r\n\r\n";
  esp.print("AT+CIPSEND=");
  esp.println(sendData.length() + 2);
  delay(1000);
  if (esp.find(">")) {                                      
    esp.print(sendData);                                         
    display_serial();
    digitalWrite(led, LOW);
    delay(500);
    digitalWrite(led, HIGH);
    Serial.println(sendData);
    Serial.println("Data sent.");
    error = 0;
    delay(1000);
  }  else {
    error = 1;
  }
  Serial.println("Connection Closed.");
  espData("AT+CIPCLOSE",0);                              
  delay(1000);
  Serial.println("==================");
}


