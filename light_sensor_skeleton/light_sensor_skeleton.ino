#include <Wire.h>
#include <BH1750.h>

BH1750 lightMeter;

void setup() {
  Serial.begin(115200);

  // ESP32의 I2C 핀 할당 (SDA: 21, SCL: 22)
  Wire.begin(21, 22);
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
  // BH1750 센서 초기화 (고해상도 연속 측정 모드)
  // if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
  //   Serial.println("BH1750 Sensor initialized.");
  // } else {
  //   Serial.println("Could not find BH1750 sensor!");
  //   while (1);
  // }
}

void loop() {
  float lux = lightMeter.readLightLevel();
  Serial.print("Light: ");
  Serial.print(lux);
  Serial.println(" lx");
  delay(1000);
}
