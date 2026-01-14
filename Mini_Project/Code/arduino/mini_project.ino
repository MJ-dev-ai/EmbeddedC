#define DEBUG

#include <WiFiEsp.h>
#include <SoftwareSerial.h>
#include <MsTimer2.h>

// [네트워크 설정]
#define AP_SSID "robotA" 
#define AP_PASS "robotA1234"
#define SERVER_NAME "10.10.141.78"
#define SERVER_PORT 5000
#define LOGID "WIFI" 
#define PASSWD "PASSWD"

// [핀 설정]
const int SENSOR_PINS[3] = {2, 3, 4}; // 센서 입력 핀
const int LED_PINS[3] = {8, 9, 10};   // LED 출력 핀

#define WIFIRX 6          // 아두이노 6(RX) <-> ESP8266 TX
#define WIFITX 7          // 아두이노 7(TX) <-> ESP8266 RX
#define CDS_PIN A0
#define CMD_SIZE 50

// 변수 선언
char sendBuf[CMD_SIZE];
bool timerIsrFlag = false;
unsigned int secCount;
int cds = 0;

// Active LOW 센서이므로 평상시 상태인 HIGH로 초기화
bool lastSensorStates[3] = {HIGH, HIGH, HIGH};    
bool currentSensorStates[3] = {HIGH, HIGH, HIGH}; 

SoftwareSerial wifiSerial(WIFIRX, WIFITX);
WiFiEspClient client;

// 함수 프로토타입
void wifi_Setup();
void wifi_Init();
void server_Connect();
void printWifiStatus();
boolean debounce(int pin, boolean last);

void setup() {
  for (int i = 0; i < 3; i++) {
    pinMode(SENSOR_PINS[i], INPUT);
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW); // 초기 LED 상태 OFF
  }
  pinMode(CDS_PIN, INPUT);
  MsTimer2::set(1000, timerIsr);  // 1000ms period
  MsTimer2::start();

#ifdef DEBUG
  Serial.begin(9600); 
#endif

  wifi_Setup(); 
}

void loop() {
  if (client.available()) {
    char c = client.read();
    #ifdef DEBUG
      Serial.write(c);
    #endif
  }

  if (!client.connected()) {
    server_Connect();
  }
  if (timerIsrFlag == true) {
    timerIsrFlag = false;
    if (secCount % 5 == 0) {
      cds = map(analogRead(CDS_PIN), 0, 250, 0, 100);
      if (cds > 50) {
        for (int i = 0; i < 3; i++) {
          digitalWrite(LED_PINS[i], LOW);
        }
      }
      else {
        for (int i = 0; i < 3; i++) {
          digitalWrite(LED_PINS[i], HIGH);
        }
      }
    }
    
    
  }

  for (int i = 0; i < 3; i++) {
    int sensorId = i + 1;
    int sensorPin = SENSOR_PINS[i];
    int ledPin = LED_PINS[i];

    currentSensorStates[i] = debounce(sensorPin, lastSensorStates[i]);

    if (lastSensorStates[i] != currentSensorStates[i]) {
      
      // 1. 물체 감지 시 (LOW) -> IN 전송 및 LED ON
      if (currentSensorStates[i] == LOW) { 
        sprintf(sendBuf, "[SQL]IN@%d@\n", sensorId);
        client.write(sendBuf, strlen(sendBuf));
        client.flush();
        
        #ifdef DEBUG
          Serial.print("Sensor "); Serial.print(sensorId);
          Serial.println(" Detected (LOW)! LED ON & Sent [SQL]IN");
        #endif
      }
      // 2. 물체 사라짐 (HIGH) -> OUT 전송 및 LED OFF
      else {
        sprintf(sendBuf, "[SQL]OUT@%d@\n", sensorId);
        client.write(sendBuf, strlen(sendBuf));
        client.flush();
        
        #ifdef DEBUG
          Serial.print("Sensor "); Serial.print(sensorId);
          Serial.println(" Cleared (HIGH). LED OFF & Sent [SQL]OUT");
        #endif
      }
      lastSensorStates[i] = currentSensorStates[i];
    }
  }
}

boolean debounce(int pin, boolean last) {
  boolean current = digitalRead(pin);
  if (last != current) {
    delay(200); 
    current = digitalRead(pin);
  }
  return current;
}

void wifi_Setup() {
  wifiSerial.begin(9600); // ESP8266과 9600으로 통신
  wifi_Init();
  server_Connect();
}

void wifi_Init() {
  do {
    WiFi.init(&wifiSerial);
    if (WiFi.status() == WL_NO_SHIELD) {
      #ifdef DEBUG
        Serial.println("WiFi shield not present");
      #endif
    } else break;
  } while (1);

  #ifdef DEBUG
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(AP_SSID);
  #endif
  
  while (WiFi.begin(AP_SSID, AP_PASS) != WL_CONNECTED) {
    #ifdef DEBUG
      Serial.print(".");
    #endif
    delay(500);
  }
  
  #ifdef DEBUG
    Serial.println("\nConnected to network");
    printWifiStatus();
  #endif
}

void timerIsr() {
  timerIsrFlag = true;
  secCount++;
  clock_calc(&dateTime);
}

void server_Connect() {
  if (client.connect(SERVER_NAME, SERVER_PORT)) {
    #ifdef DEBUG
      Serial.println("Connected to server");
    #endif
    client.print("[" LOGID ":" PASSWD "]");
  }
}

void printWifiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}