// xdb401 pressure sensor, esp32 s3
// don't need pull-up resistor, because they built-in in sensor

#include <Wire.h>

const uint8_t SENSOR_ADDR = 0x7F;  // Адрес датчика
const uint8_t SDA_PIN = 8;
const uint8_t SCL_PIN = 9;

float Fullscale_P = 1000.0;  // Полный масштаб давления

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  delay(1000);
}

void loop() {
  // 1. Отправляем команду 0x0A в регистр 0x30 для запуска измерения
  if (!writeRegister(SENSOR_ADDR, 0x30, 0x0A)) {
    Serial.println("Ошибка записи команды запуска измерения");
    delay(1000);
    return;
  }

  // 2. Ждём завершения измерения, проверяя бит Sco (бит 3) регистра 0x30
  bool acquisitionComplete = false;
  for (int i = 0; i < 10; i++) {  // максимум 10 попыток с задержкой
    uint8_t reg30 = 0;
    if (!readRegister(SENSOR_ADDR, 0x30, &reg30)) {
      Serial.println("Ошибка чтения регистра 0x30");
      delay(100);
      continue;
    }
    if ((reg30 & 0x08) == 0) {  // бит 3 == 0
      acquisitionComplete = true;
      break;
    }
    delay(50);  // подождать 50 мс и проверить снова
  }

  if (!acquisitionComplete) {
    Serial.println("Измерение не завершено, данные не готовы");
    delay(500);
    return;
  }

  // 3. Читаем данные из регистров 0x06-0x0A (5 байт)
  uint8_t data[5];
  if (!readBytesFromRegister(SENSOR_ADDR, 0x06, data, 5)) {
    Serial.println("Ошибка чтения данных давления и температуры");
    delay(500);
    return;
  }

  // Обработка давления (3 байта: 0x06, 0x07, 0x08)
  float Cal_PData1 = data[0] * 65536.0 + data[1] * 256.0 + data[2];
  float Cal_PData2;
  if (Cal_PData1 > 8388608) {
    Cal_PData2 = (Cal_PData1 - 16777216) / 8388608.0;
  } else {
    Cal_PData2 = Cal_PData1 / 8388608.0;
  }
  float Pressure_data = Cal_PData2 * Fullscale_P;

  // Обработка температуры (2 байта: 0x09, 0x0A)
  float Cal_TData1 = data[3] * 256.0 + data[4];
  float Cal_TData2;
  if (Cal_TData1 > 32768) {
    Cal_TData2 = (Cal_TData1 - 65536) / 256.0;
  } else {
    Cal_TData2 = Cal_TData1 / 256.0;
  }
  float Temp_data = Cal_TData2;

  // Вывод результатов
  Serial.print("Pressure: ");
  Serial.print(Pressure_data);
  Serial.print(" kPa, Temperature: ");
  Serial.print(Temp_data);
  Serial.println(" °C");

  delay(1000);  // пауза 1 секунда
}

// Функция записи одного байта в регистр
bool writeRegister(uint8_t deviceAddress, uint8_t regAddress, uint8_t value) {
  Wire.beginTransmission(deviceAddress);
  Wire.write(regAddress);
  Wire.write(value);
  return (Wire.endTransmission() == 0);
}

// Функция чтения одного байта из регистра
bool readRegister(uint8_t deviceAddress, uint8_t regAddress, uint8_t* value) {
  Wire.beginTransmission(deviceAddress);
  Wire.write(regAddress);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  if (Wire.requestFrom(deviceAddress, (uint8_t)1) != 1) {
    return false;
  }
  *value = Wire.read();
  return true;
}

// Функция чтения нескольких байт из регистра
bool readBytesFromRegister(uint8_t deviceAddress, uint8_t regAddress, uint8_t* buffer, uint8_t length) {
  Wire.beginTransmission(deviceAddress);
  Wire.write(regAddress);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  if (Wire.requestFrom(deviceAddress, length) != length) {
    return false;
  }
  for (uint8_t i = 0; i < length; i++) {
    buffer[i] = Wire.read();
  }
  return true;
}
