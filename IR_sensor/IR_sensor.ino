// IR 센서의 OUT 핀을 연결한 핀 번호 (필요에 따라 변경)
const int irSensorPin = 21;

void setup() {
  Serial.begin(115200);       // 시리얼 통신 시작
  pinMode(irSensorPin, INPUT); // IR 센서 핀을 입력으로 설정
  Serial.println("IR 센서 입력 대기 중...");
}

void loop() {
  int sensorValue = digitalRead(irSensorPin); // 센서 상태 읽기

  unsigned long now = millis() / 1000;
  int hours = now / 3600;
  int minutes = (now % 3600) / 60;
  int seconds = now % 60;

  // 시간 출력 + 센서 값
  char timestamp[16];
  sprintf(timestamp, "%02d:%02d:%02d", hours, minutes, seconds);
  Serial.print("[");
  Serial.print(timestamp);
  Serial.print("] ");

  Serial.print("IR Sensor Value: ");
  Serial.println(1 - sensorValue);               // 시리얼로 출력 (0 또는 1)

  delay(200); // 짧은 지연
}
