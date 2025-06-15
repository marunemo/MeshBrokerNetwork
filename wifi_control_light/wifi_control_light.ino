#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <strings.h>

#include <Wire.h>
#include <BH1750.h>

#include <ArduinoMqttClient.h>

WiFiClient espClient;
MqttClient mqttClient(espClient);

// WiFi AP 리스트
const char* ssidList[] = {"MeshBroker1"};
const char* passwordList[] = {"12345678"};
const int apCount = sizeof(ssidList)/sizeof(ssidList[0]);

#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1
#define OLED_ADDR       0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

volatile bool wifi_connected = false;
TaskHandle_t wifiTaskHandles[2];
SemaphoreHandle_t connectMutex;
TaskHandle_t rssiTaskHandle = NULL;

struct ApInfo {
  String ssid;
  String password;
  int rssi;
};
ApInfo apInfos[apCount]; 

//Display Message
void showMessage(const String& line1, const String& line2 = "") {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.println(line1);
  if (line2 != "") display.println(line2);
  display.display();
}
//내림차순
int cmp(const void* a, const void* b) {
    return ((ApInfo*)b)->rssi - ((ApInfo*)a)->rssi;
}
void scanAndSortApByRssi() {
  int found = WiFi.scanNetworks();//scan network
  Serial.println("Founding network...\n");
  while (found <= 0 ) {//연결 가능한 네트워크를 계속 측정
    delay(100);
    found = WiFi.scanNetworks(); // 네트워크 스캔
  } 
  Serial.println("Found network!\n");
  // AP 리스트 초기화 및 RSSI 측정
  for (int i = 0; i < apCount; i++) {
    apInfos[i].ssid = ssidList[i]; // ssid
    apInfos[i].password = passwordList[i]; // password
    apInfos[i].rssi = -999; // Not found 
    for (int j = 0; j < found; j++) { // RSSI 측정
      if (WiFi.SSID(j) == ssidList[i]) {
        apInfos[i].rssi = WiFi.RSSI(j);
        break;
      }
    }
  }
  // quic sort
  qsort(apInfos, apCount, sizeof(ApInfo),cmp);
  // Sorted network 출력
  for (int i = 0; i < apCount; i++) {
    Serial.printf("SSID: %s, PASSWORD: %s, RSSI: %d dBm\n",
    apInfos[i].ssid.c_str(), apInfos[i].password.c_str(), apInfos[i].rssi);
  }
}
// WiFi Connection 
void connectWiFi() {
  // 순서대로 AP 연결시도
  for (int i = 0; i < apCount; i++) {
    if (apInfos[i].rssi != -999 && apInfos[i].rssi > -80) { //신호크기 제한
      WiFi.begin(apInfos[i].ssid, apInfos[i].password);
      int retry = 0;
      while (WiFi.status() != WL_CONNECTED && retry < 50) {//5초 retry
        delay(100);
        retry++;
      }
      if (WiFi.status() == WL_CONNECTED) {//연결 성공
        wifi_connected = true;
        showMessage("WiFi connected!", WiFi.SSID() + " " + WiFi.localIP().toString());
        Serial.printf("Connected to %s! IP: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

        // MQTT 연결 부분
        IPAddress localIP = WiFi.localIP();
        IPAddress brokerIP(localIP[0], localIP[1], localIP[2], 1);  // 마지막 옥텟만 1로

        mqttClient.setId("arduinoClient");
        mqttClient.connect(brokerIP, 1883);
        Serial.printf("setServer to %s\n", brokerIP.toString().c_str());

        break;
      }
    }
  }
  if (!wifi_connected) showMessage("WiFi ALL failed");//연결 실패 
  else startRssiTask();//연결 성공 RSSI 측정
}
//Show RSSI and disconnect WiFi
void rssiMonitorTask(void * parameter) {
  unsigned long lastPrint = 0;
  while (true) {//already wifi connected
    if (WiFi.status() == WL_CONNECTED) {//rssi 출력
      int rssi = WiFi.RSSI();
      if (millis() - lastPrint > 1000) {  // 1초간격 출력
        // Serial.printf("[RSSI] %d dBm\n", rssi);
        lastPrint = millis();
      }
      if(rssi<-80){ //RSSI가 너무 낮으면 disconnect
        Serial.printf("[RSSI] Too low, disconnecting..\n", rssi);
        WiFi.disconnect();
        stopRssiTask();
      }
    }else {
      Serial.println("[RSSI] WiFi Not Connected");
      stopRssiTask();
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);  // 0.1초 간격
  }
}
//FreeRTOS RSSI 체크 Task
void startRssiTask() {
  if (rssiTaskHandle == NULL) {
    xTaskCreatePinnedToCore(
      rssiMonitorTask,
      "RSSIMonitor",
      2048,
      NULL,
      1,
      &rssiTaskHandle,
      1
    );
  }
}
// RSSI Task 종료
void stopRssiTask() {
  if (rssiTaskHandle != NULL) {
    Serial.println("[RSSI Task] Stopped");
    vTaskDelete(rssiTaskHandle);
  }
}

void setup() {
  Serial.begin(115200);

  // OLED Initialization
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("SSD1306 OLED init failed");
    while (true);
  }
  connectMutex = xSemaphoreCreateMutex();
  scanAndSortApByRssi(); // scan RSSI and sorting list,connect wifi
  showMessage("Connecting to WiFi");
  connectWiFi();//wifi connect

  // 전구 조절 핀 할당 (21)
  pinMode(21, OUTPUT);

  mqttClient.subscribe("test/topic");
}

void loop() {
  static unsigned long wifiRetryTime = 0;
  static bool tryingReconnect = false;
  //재연결
  if (WiFi.status() != WL_CONNECTED) {
    if (!tryingReconnect && millis() - wifiRetryTime > 10000) {
      showMessage("WiFi reconnecting...");
      Serial.println("WiFi reconnecting...");
      tryingReconnect = true;
      wifi_connected = false;
      rssiTaskHandle = NULL;
      //wifi 재설정
      WiFi.disconnect(true, true); // 연결 종료
      WiFi.mode(WIFI_OFF);         // WiFi 전원 OFF
      delay(100);                  // Wait OFF
      WiFi.mode(WIFI_STA);         // WiFi ON
      delay(1000);                 // Wait ON

      scanAndSortApByRssi();
      showMessage("Connecting to WiFi");
      connectWiFi();//wifi connect
      
      wifiRetryTime = millis();
      tryingReconnect = false;
    }
  }

  // mqtt
  mqttClient.poll();

  if (mqttClient.parseMessage()) {
    Serial.print("메시지: ");

    String message = mqttClient.readString();
    Serial.println(message);
    if (message == "ON") {
      digitalWrite(21, HIGH);
    }
    else if (message == "OFF") {
      digitalWrite(21, LOW);
    }
  }
}