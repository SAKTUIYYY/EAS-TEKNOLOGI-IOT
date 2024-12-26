#include <DHT.h>
#include <Wire.h>
#include <INA226_WE.h>

#define DHTPIN 4      // Pin sensor DHT11
#define DHTTYPE DHT11 // Tipe sensor DHT
#define RELAY_PIN 13  // Pin kontrol relay
#define LED_PIN_RED 19 // Pin LED merah
#define LED_PIN_ORANGE 18 // Pin LED oranye
#define LED_PIN_GREEN 23 // Pin LED hijau
#define FLAME_SENSOR_PIN 36 // Pin analog sensor flame (gunakan pin ADC ESP32)
#define WATER_SENSOR_PIN 34 // Pin analog sensor level air
#define I2C_ADDRESS 0x40 // Alamat I2C INA226

DHT dht(DHTPIN, DHTTYPE);
INA226_WE ina226 = INA226_WE(I2C_ADDRESS);

// Fungsi keanggotaan fuzzy untuk suhu rendah
float fuzzyLowTemp(float temp) {
  if (temp <= 20) return 1;
  if (temp >= 29) return 0;
  return (29 - temp) / 9.0;
}

// Fungsi keanggotaan fuzzy untuk suhu tinggi
float fuzzyHighTemp(float temp) {
  if (temp <= 29) return 0;
  if (temp >= 35) return 1;
  return (temp - 29) / 6.0;
}

void setup() {
  Serial.begin(9600);
  dht.begin();
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN_RED, OUTPUT);
  pinMode(LED_PIN_ORANGE, OUTPUT);
  pinMode(LED_PIN_GREEN, OUTPUT);
  pinMode(FLAME_SENSOR_PIN, INPUT);
  pinMode(WATER_SENSOR_PIN, INPUT);
  digitalWrite(RELAY_PIN, LOW); // Pastikan relay dalam keadaan mati saat mulai
  digitalWrite(LED_PIN_RED, LOW);   // Pastikan LED merah dalam keadaan mati saat mulai
  digitalWrite(LED_PIN_ORANGE, LOW); // Pastikan LED oranye dalam keadaan mati saat mulai
  digitalWrite(LED_PIN_GREEN, LOW); // Pastikan LED hijau dalam keadaan mati saat mulai

  Wire.begin();
  ina226.init();

  // Konfigurasi INA226
  ina226.setResistorRange(0.1, 1.3); // Resistor 0.1 Ohm, arus hingga 1.3A
  ina226.setCorrectionFactor(0.93); // Faktor koreksi jika diperlukan

  Serial.println("Sistem Monitoring Suhu, Arus, Api, dan Level Air - INA226, DHT11, Flame Sensor, Water Level Sensor");
  ina226.waitUntilConversionCompleted(); // Tunggu konversi pertama selesai
}

void loop() {
  // Membaca suhu dari DHT11
  float temperature = dht.readTemperature();

  if (isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    // Logika fuzzy untuk suhu
    float lowTemp = fuzzyLowTemp(temperature);
    float highTemp = fuzzyHighTemp(temperature);

    // Inferensi fuzzy: Menentukan status relay
    bool relayStatus = false;
    relayStatus |= (highTemp > 0.5); // Hidupkan relay jika suhu tinggi (fuzzy > 0.5)

    // Kontrol relay
    digitalWrite(RELAY_PIN, relayStatus ? HIGH : LOW);
    digitalWrite(LED_PIN_RED, relayStatus ? HIGH : LOW); // Kontrol LED merah

    // Debugging suhu dan status relay
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" *C ");
    Serial.print("Relay Status: ");
    Serial.println(relayStatus ? "ON" : "OFF");
  }

  // Membaca data dari INA226
  float shuntVoltage_mV = ina226.getShuntVoltage_mV();
  float busVoltage_V = ina226.getBusVoltage_V();
  float current_mA = ina226.getCurrent_mA();
  float power_mW = ina226.getBusPower();
  float loadVoltage_V = busVoltage_V + (shuntVoltage_mV / 1000);

  // Menampilkan data INA226
  Serial.print("Shunt Voltage [mV]: "); Serial.println(shuntVoltage_mV);
  Serial.print("Bus Voltage [V]: "); Serial.println(busVoltage_V);
  Serial.print("Load Voltage [V]: "); Serial.println(loadVoltage_V);
  Serial.print("Current [mA]: "); Serial.println(current_mA);
  Serial.print("Bus Power [mW]: "); Serial.println(power_mW);

  // Kontrol LED oranye berdasarkan arus & tegangan
  if (current_mA > 1.50 && shuntVoltage_mV > 1.50 ) {
    digitalWrite(LED_PIN_ORANGE, LOW); // Nyalakan LED oranye jika arus lebih dari 1.50 mA
  } else {
    digitalWrite(LED_PIN_ORANGE, HIGH); // Matikan LED oranye jika arus kurang dari atau sama dengan 1.50 mA
  }

  if (!ina226.overflow) {
    Serial.println("Values OK - no overflow");
  } else {
    Serial.println("Overflow! Choose higher current range");
  }

  // Membaca data dari sensor flame
  int flameValue = analogRead(FLAME_SENSOR_PIN); // Membaca nilai analog dari sensor flame

  Serial.print("Flame Sensor Value: ");
  Serial.println(flameValue);

  // Kondisi deteksi api (nilai threshold disesuaikan dengan sensor, default: < 1000)
  if (flameValue < 1000) {
    Serial.println("Api terdeteksi!");
  } else {
    Serial.println("Tidak ada api.");
  }

  // Membaca data dari sensor level air
  int waterLevel = analogRead(WATER_SENSOR_PIN); // Membaca nilai analog dari sensor level air

  Serial.print("Water Level Sensor Value: ");
  Serial.println(waterLevel);

  // Kondisi level air (jika terdeteksi air maka LED hijau menyala)
  if (waterLevel > 0) {
    Serial.println("Air terdeteksi.");
    digitalWrite(LED_PIN_GREEN, HIGH); // Nyalakan LED hijau jika air terdeteksi
  } else {
    Serial.println("Tidak ada air.");
    digitalWrite(LED_PIN_GREEN, LOW); // Matikan LED hijau jika tidak ada air
  }

  Serial.println();
  delay(3000); // Delay untuk pembacaan data berikutnya
}
