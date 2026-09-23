/*
 * ESP32 - Fully Calibrated Sensor System with SD Card Logging & WiFi Client
 *
 * This is a consolidated and corrected version of the code provided.
 * It combines all sensor and GPS logic into a single, valid sketch.
 * This version also includes WiFi capabilities to send sensor data to a server.
 *
 * Sensors:
 * - MPU6050: SDA=21, SCL=22 -> Wire
 * - BMP280 + QMC5883L: SDA=17, SCL=16 -> WireShared
 * - Ublox NEO-6M GPS: RX=15, TX=4 -> SerialGPS
 * - microSd card adapter: MISO=19, MOSI=23, SCK=18, CS=5
 *
 * Features:
 * - Auto-calibration for MPU6050 and compass
 * - Tilt-compensated heading
 * - Offset correction
 * - Writes all sensor data to a CSV file on a microSD card
 * - Prints data to Serial Monitor with clear SD card status
 * - Sends sensor data to a specified Wi-Fi server via HTTP POST request.
 */

#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <MPU6050.h>
#include <TinyGPS++.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ====== WiFi Network Credentials ======
// Make sure this matches the SSID and password of the server's access point.
const char* ssid = "ESP32-Sensor-Network";
const char* password = "sensor_password";

// The IP address of the server (the ESP32 AP).
// This is the default IP address for the ESP32's Access Point.
const char* serverIp = "192.168.4.1";

// ====== I2C Addresses ======
#define BMP280_ADDRESS 0x76
#define MPU6050_ADDRESS 0x68
#define QMC5883_ADDRESS 0x0D

// ====== MicroSD Card Pins ======
#define SD_CS_PIN 5
#define SD_MOSI_PIN 23
#define SD_MISO_PIN 19
#define SD_SCK_PIN 18

// ====== GPS Pins ======
#define GPS_RX_PIN 15
#define GPS_TX_PIN 4

// ====== I2C Bus Instances ======
TwoWire WireShared = TwoWire(1);

// ====== Sensor Objects ======
Adafruit_BMP280 bmp(&WireShared);
MPU6050 mpu6050;

// ====== GPS Objects ======
HardwareSerial SerialGPS(2);
TinyGPSPlus gps;

// This variable will store the pressure reading at startup.
// It will serve as the "sea-level pressure" for our relative altitude calculations.
float seaLevelPressurehPa;

// File object for the SD card logging
File dataFile;
String fileName;

// ====== Enhanced Calibration Data Structure ======
struct {
  float accelBias[3] = {0, 0, 0};
  float gyroBias[3] = {0, 0, 0};
  float magOffset[2] = {0, 0};
  float magScale[2] = {1.0, 1.0};
  float magMin[2] = {32767, 32767};
  float magMax[2] = {-32768, -32768};
  bool compassCalibrated = false;
} calib;

// ====== Sensor Data ======
struct SensorData {
  float time = 0.0;
  float temperature = 0.0;
  float pressure = 0.0;
  float altitude = 0.0;
  float accel_x = 0.0, accel_y = 0.0, accel_z = 0.0;
  float gyro_x = 0.0, gyro_y = 0.0, gyro_z = 0.0;
  float heading_raw = 0.0;
  float heading_tilt = 0.0;
  // GPS Data
  float gps_lat = 0.0;
  float gps_lng = 0.0;
  float gps_alt = 0.0;
  int gps_satellites = 0;
  float gps_speed = 0.0;
  float gps_course = 0.0;
  bool gps_is_valid = false;
  bool gps_is_connected = false;
} sensorData;

// ====== Flags ======
bool sensorsReady = false;
bool sdCardReady = false;
String sdCardStatusMessage = "SD Card not initialized.";

// Function declarations to avoid 'not declared in this scope' errors
void scanI2C(TwoWire &wire, const char* name);
bool initQMC5883L();
void calibrateMPU6050();
void calibrateCompass();
void readGPS();
void readMPU6050();
void readBMP280();
void readCompass();
void readMagnetometer(int16_t &x, int16_t &y, int16_t &z);
void printSensorData(float t);
void logSensorDataToFile();
void setupWiFi();
void sendDataToServer();

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("[ESP32] CALIBRATED SENSOR SYSTEM");
  Serial.println("================================");

  // === Initialize I2C buses ===
  Wire.begin(21, 22, 400000);
  delay(10);
  WireShared.begin(17, 16, 400000);
  delay(10);
  SerialGPS.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  // === Scan and Initialize Sensors ===
  scanI2C(Wire, "MPU6050 Bus (21,22)");
  scanI2C(WireShared, "Shared Bus (17,16)");

  if (!bmp.begin(BMP280_ADDRESS)) {
    Serial.println("❌ BMP280 FAILED!");
  } else {
    Serial.println("✅ BMP280 OK");
    seaLevelPressurehPa = bmp.readPressure() / 100.0F;
    Serial.printf(" BMP280 Calibrated: 0m reference pressure set to %.2f hPa\n", seaLevelPressurehPa);
  }

  mpu6050.initialize();
  if (!mpu6050.testConnection()) {
    Serial.println("❌ MPU6050 FAILED!");
  } else {
    Serial.println("✅ MPU6050 OK");
  }

  if (initQMC5883L()) {
    Serial.println("✅ QMC5883L OK");
  } else {
    Serial.println("❌ QMC5883L FAILED!");
  }

  // === Start Calibration ===
  Serial.println("\n🔍 Starting Calibration...");
  delay(2000);
  calibrateMPU6050();
  calibrateCompass();
  Serial.println("✅ Calibration Complete!");
  Serial.println("All sensors ready.\n");

  // === Initialize SD Card ===
  Serial.println("💾 Initializing SD Card...");
  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD Card Status: FAILED! Card not found, invalid pins, or failed to initialize.");
    sdCardReady = false;
    sdCardStatusMessage = "SD Card FAILED: Not detected.";
  } else {
    Serial.println("SD Card Status: OK. Card initialized successfully.");
    sdCardReady = true;

    // Create a new file with a unique name
    int fileNumber = 0;
    do {
      fileName = "/data" + String(fileNumber++) + ".csv";
    } while (SD.exists(fileName));

    dataFile = SD.open(fileName, FILE_WRITE);
    if (dataFile) {
      Serial.printf("SD Card: Created and opened file %s\n", fileName.c_str());
      // Write header to the file
      dataFile.println("Time(s),Temperature(C),Pressure(hPa),Altitude(m),AccelX(g),AccelY(g),AccelZ(g),GyroX(dps),GyroY(dps),GyroZ(dps),HeadingRaw(deg),HeadingTilt(deg),GPS_Lat,GPS_Lon,GPS_Alt(m),GPS_Speed(kph),GPS_Satellites");
      dataFile.close();
      sdCardStatusMessage = "SD Card OK";
    } else {
      Serial.println("SD Card: FAILED to create a new file for logging.");
      sdCardReady = false;
      sdCardStatusMessage = "SD Card FAILED: File creation error.";
    }
  }

  // === Setup Wi-Fi ===
  setupWiFi();

  sensorsReady = true;
}

void loop() {
  static unsigned long lastReading = 0;
  static unsigned long startTime = millis();

  if (sensorsReady) {
    readGPS();
    if (millis() - lastReading >= 1000) {
      lastReading = millis();
      sensorData.time = (millis() - startTime) / 1000.0;
      readMPU6050();
      readBMP280();
      readCompass();
      logSensorDataToFile();
      printSensorData(sensorData.time);
      sendDataToServer();
    }
  }
  delay(10);
}

// === Function to setup Wi-Fi client connection ===
void setupWiFi() {
  // Use the correct constant for station mode
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("\nConnecting to WiFi...");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected!");
    Serial.printf("Connected to AP, IP address: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n❌ WiFi Connection Failed!");
    Serial.println("Check SSID and password, or server AP is not active.");
  }
}

// === Function to send sensor data to the server ===
void sendDataToServer() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    // The server is at the IP address of the AP, listening on port 80, route /data
    String serverUrl = "http://" + String(serverIp) + "/data";
    http.begin(serverUrl.c_str());
    http.addHeader("Content-Type", "application/json");

    // Use a StaticJsonDocument to create a JSON object from sensor data
    StaticJsonDocument<512> doc;
    doc["time"] = sensorData.time;
    doc["temp_C"] = sensorData.temperature;
    doc["press_hPa"] = sensorData.pressure;
    doc["alt_m"] = sensorData.altitude;
    doc["accel_x_g"] = sensorData.accel_x;
    doc["accel_y_g"] = sensorData.accel_y;
    doc["accel_z_g"] = sensorData.accel_z;
    doc["gyro_x_dps"] = sensorData.gyro_x;
    doc["gyro_y_dps"] = sensorData.gyro_y;
    doc["gyro_z_dps"] = sensorData.gyro_z;
    doc["heading_raw_deg"] = sensorData.heading_raw;
    doc["heading_tilt_deg"] = sensorData.heading_tilt;

    // Conditionally add GPS data if it is valid
    if (sensorData.gps_is_valid) {
      doc["gps_lat"] = sensorData.gps_lat;
      doc["gps_lng"] = sensorData.gps_lng;
      doc["gps_alt_m"] = sensorData.gps_alt;
      doc["gps_speed_kph"] = sensorData.gps_speed;
      doc["gps_satellites"] = sensorData.gps_satellites;
    } else {
      doc["gps_status"] = "No fix";
    }

    String output;
    serializeJson(doc, output);

    // Send the JSON payload
    int httpResponseCode = http.POST(output);
    if (httpResponseCode > 0) {
      Serial.printf("✅ HTTP POST success! Response code: %d\n", httpResponseCode);
    } else {
      Serial.printf("❌ HTTP POST failed! Error code: %d\n", httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("❌ Not connected to WiFi. Can't send data.");
  }
}

// === Print Sensor Data with SD Card Status ===
void printSensorData(float t) {
  Serial.printf("Time: %.1fs | Temp: %.2f°C | Press: %.2fhPa | Alt: %.1fm | Acc(X,Y,Z): %.3f, %.3f, %.3f g | Gyro(X,Y,Z): %.2f, %.2f, %.2f °/s | Head: %.1f°, Tilt Head: %.1f° | ",
    t,
    sensorData.temperature,
    sensorData.pressure,
    sensorData.altitude,
    sensorData.accel_x, sensorData.accel_y, sensorData.accel_z,
    sensorData.gyro_x, sensorData.gyro_y, sensorData.gyro_z,
    sensorData.heading_raw,
    sensorData.heading_tilt);

  if (sensorData.gps_is_connected) {
    if (sensorData.gps_is_valid) {
      Serial.printf("GPS: Lat:%.4f, Lng:%.4f, Alt:%.1fm, Spd:%.2fkm/h, Sats:%d | ",
        sensorData.gps_lat, sensorData.gps_lng, sensorData.gps_alt,
        sensorData.gps_speed, sensorData.gps_satellites);
    } else {
      Serial.printf("GPS: Waiting for fix... (%d sats) | ", sensorData.gps_satellites);
    }
  } else {
    Serial.print("GPS: Disconnected. | ");
  }

  // SD card status is now part of the print statement
  if (sdCardReady) {
    Serial.print("SD Card: ✅");
  } else {
    Serial.printf("SD Card: ❌ %s", sdCardStatusMessage.c_str());
  }

  if (!calib.compassCalibrated) {
    Serial.print(" [UNCAL]");
  }
  Serial.println();
}

// === Log Sensor Data to File ===
void logSensorDataToFile() {
  if (sdCardReady) {
    dataFile = SD.open(fileName, FILE_APPEND);
    if (dataFile) {
      dataFile.printf("%.1f,%.2f,%.2f,%.2f,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f,%.1f,%.1f,%.6f,%.6f,%.2f,%.2f,%d\n",
        sensorData.time,
        sensorData.temperature,
        sensorData.pressure,
        sensorData.altitude,
        sensorData.accel_x,
        sensorData.accel_y,
        sensorData.accel_z,
        sensorData.gyro_x,
        sensorData.gyro_y,
        sensorData.gyro_z,
        sensorData.heading_raw,
        sensorData.heading_tilt,
        sensorData.gps_lat,
        sensorData.gps_lng,
        sensorData.gps_alt,
        sensorData.gps_speed,
        sensorData.gps_satellites
      );
      dataFile.close();
      sdCardStatusMessage = "SD Card OK"; // Reset status message if successful
    } else {
      sdCardReady = false;
      sdCardStatusMessage = "File failed to open.";
    }
  } else {
    // If we're here, it means it already failed in setup.
    // We don't need to update the status message again.
  }
}

void readBMP280() {
  sensorData.temperature = bmp.readTemperature();
  sensorData.pressure = bmp.readPressure() / 100.0F;
  sensorData.altitude = bmp.readAltitude(seaLevelPressurehPa);
}

void readMPU6050() {
  int16_t ax, ay, az, gx, gy, gz;
  mpu6050.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  sensorData.accel_x = (ax / 16384.0) - calib.accelBias[0];
  sensorData.accel_y = (ay / 16384.0) - calib.accelBias[1];
  sensorData.accel_z = (az / 16384.0) - calib.accelBias[2];
  sensorData.gyro_x = (gx / 131.0) - calib.gyroBias[0];
  sensorData.gyro_y = (gy / 131.0) - calib.gyroBias[1];
  sensorData.gyro_z = (gz / 131.0) - calib.gyroBias[2];
}

bool initQMC5883L() {
  WireShared.beginTransmission(QMC5883_ADDRESS);
  WireShared.write(0x0B);
  WireShared.write(0x01);
  if (WireShared.endTransmission() != 0) return false;
  delay(10);
  WireShared.beginTransmission(QMC5883_ADDRESS);
  WireShared.write(0x09);
  WireShared.write(0x1D);
  if (WireShared.endTransmission() != 0) return false;
  delay(100);
  return true;
}

void readMagnetometer(int16_t &x, int16_t &y, int16_t &z) {
  WireShared.beginTransmission(QMC5883_ADDRESS);
  WireShared.write(0x00);
  WireShared.endTransmission(false);
  WireShared.requestFrom(QMC5883_ADDRESS, 6);
  if (WireShared.available() >= 6) {
    uint8_t data[6];
    for (int i = 0; i < 6; i++) data[i] = WireShared.read();
    x = (data[1] << 8) | data[0];
    y = (data[3] << 8) | data[2];
    z = (data[5] << 8) | data[4];
  } else {
    x = y = z = 0;
  }
}

void readCompass() {
  int16_t mx_raw, my_raw, mz_raw;
  WireShared.beginTransmission(QMC5883_ADDRESS);
  WireShared.write(0x00);
  if (WireShared.endTransmission(false) != 0) return;
  if (WireShared.requestFrom(QMC5883_ADDRESS, 6) != 6) return;
  uint8_t data[6];
  for (int i = 0; i < 6; i++) data[i] = WireShared.read();
  mx_raw = (data[1] << 8) | data[0];
  my_raw = (data[3] << 8) | data[2];
  mz_raw = (data[5] << 8) | data[4];
  if (mx_raw == 0 && my_raw == 0) return;

  float mx_cal, my_cal, mz_cal;
  if (calib.compassCalibrated) {
    mx_cal = (mx_raw - calib.magOffset[0]) * calib.magScale[0];
    my_cal = (my_raw - calib.magOffset[1]) * calib.magScale[1];
    mz_cal = mz_raw;
  } else {
    mx_cal = mx_raw;
    my_cal = my_raw;
    mz_cal = mz_raw;
  }

  sensorData.heading_raw = atan2(my_cal, mx_cal) * 180.0 / PI;
  if (sensorData.heading_raw < 0) sensorData.heading_raw += 360;

  float ax = sensorData.accel_x + calib.accelBias[0];
  float ay = sensorData.accel_y + calib.accelBias[1];
  float az = sensorData.accel_z + calib.accelBias[2] + 1.0;

  float roll = atan2(ay, az);
  float pitch = atan2(-ax, sqrt(ay * ay + az * az));

  float magX_comp = mx_cal * cos(pitch) + mz_cal * sin(pitch);
  float magY_comp = mx_cal * sin(roll) * sin(pitch) + my_cal * cos(roll) - mz_cal * sin(roll) * cos(pitch);

  sensorData.heading_tilt = atan2(magY_comp, magX_comp) * 180.0 / PI;
  if (sensorData.heading_tilt < 0) sensorData.heading_tilt += 360;
}

void calibrateMPU6050() {
  Serial.println("🔧 Calibrating MPU6050 (keep flat!)...");
  delay(2000);
  long ax = 0, ay = 0, az = 0;
  long gx = 0, gy = 0, gz = 0;
  const int samples = 100;
  for (int i = 0; i < samples; i++) {
    int16_t raw_ax, raw_ay, raw_az, raw_gx, raw_gy, raw_gz;
    mpu6050.getMotion6(&raw_ax, &raw_ay, &raw_az, &raw_gx, &raw_gy, &raw_gz);
    ax += raw_ax; ay += raw_ay; az += raw_az;
    gx += raw_gx; gy += raw_gy; gz += raw_gz;
    delay(10);
  }
  calib.accelBias[0] = (ax / (float)samples) / 16384.0;
  calib.accelBias[1] = (ay / (float)samples) / 16384.0;
  calib.accelBias[2] = ((az / (float)samples) / 16384.0) - 1.0;
  calib.gyroBias[0] = (gx / (float)samples) / 131.0;
  calib.gyroBias[1] = (gy / (float)samples) / 131.0;
  calib.gyroBias[2] = (gz / (float)samples) / 131.0;

  Serial.printf("✅ MPU6050 Calibrated:\n");
  Serial.printf(" Acc Bias: %.3f, %.3f, %.3f\n", calib.accelBias[0], calib.accelBias[1], calib.accelBias[2]);
  Serial.printf(" Gyro Bias: %.2f, %.2f, %.2f\n", calib.gyroBias[0], calib.gyroBias[1], calib.gyroBias[2]);
}

void calibrateCompass() {
  Serial.println("🧭 Rotate device in figure-8 for 20s...");
  Serial.println(" Move slowly in all orientations!");
  delay(3000);
  unsigned long calTime = millis();
  const int calDuration = 20000;
  int sampleCount = 0;
  calib.magMin[0] = calib.magMin[1] = 32767;
  calib.magMax[0] = calib.magMax[1] = -32768;
  while (millis() - calTime < calDuration) {
    int16_t x, y, z;
    readMagnetometer(x, y, z);
    if (x != 0 && y != 0) {
      if (x < calib.magMin[0]) calib.magMin[0] = x;
      if (y < calib.magMin[1]) calib.magMin[1] = y;
      if (x > calib.magMax[0]) calib.magMax[0] = x;
      if (y > calib.magMax[1]) calib.magMax[1] = y;
      sampleCount++;
    }
    if (sampleCount % 20 == 0) Serial.print(".");
    delay(100);
  }
  calib.magOffset[0] = (calib.magMax[0] + calib.magMin[0]) / 2.0;
  calib.magOffset[1] = (calib.magMax[1] + calib.magMin[1]) / 2.0;
  float rangeX = calib.magMax[0] - calib.magMin[0];
  float rangeY = calib.magMax[1] - calib.magMin[1];
  float avgRange = (rangeX + rangeY) / 2.0;
  calib.magScale[0] = avgRange / rangeX;
  calib.magScale[1] = avgRange / rangeY;
  calib.compassCalibrated = true;

  Serial.printf("\n✅ Compass Calibrated (%d samples):\n", sampleCount);
  Serial.printf(" X: %d to %d -> offset: %.0f, scale: %.3f\n",
    (int)calib.magMin[0], (int)calib.magMax[0],
    calib.magOffset[0], calib.magScale[0]);
  Serial.printf(" Y: %d to %d -> offset: %.0f, scale: %.3f\n",
    (int)calib.magMin[1], (int)calib.magMax[1],
    calib.magOffset[1], calib.magScale[1]);
}

void scanI2C(TwoWire &wire, const char* name) {
  Serial.printf("Scanning %s: ", name);
  byte count = 0;
  for (byte addr = 1; addr < 127; addr++) {
    wire.beginTransmission(addr);
    if (wire.endTransmission() == 0) {
      Serial.printf("[0x%02X] ", addr);
      count++;
    }
  }
  if (count == 0) Serial.println("No devices");
  else Serial.println();
}

void readGPS() {
  // A simple method to check if the GPS module is sending data.
  static unsigned long lastCharsProcessed = 0;
  unsigned long currentCharsProcessed = gps.charsProcessed();
  static unsigned long lastConnectionCheck = 0;

  while (SerialGPS.available() > 0) {
    if (gps.encode(SerialGPS.read())) {
      // Data is available, update the connection status
      sensorData.gps_is_connected = true;
      if (gps.location.isValid()) {
        sensorData.gps_is_valid = true;
        sensorData.gps_lat = gps.location.lat();
        sensorData.gps_lng = gps.location.lng();
        sensorData.gps_alt = gps.altitude.meters();
        sensorData.gps_satellites = gps.satellites.value();
        sensorData.gps_speed = gps.speed.kmph();
        sensorData.gps_course = gps.course.deg();
      } else {
        sensorData.gps_is_valid = false;
        sensorData.gps_satellites = gps.satellites.value();
      }
    }
  }

  // Update the connection status every few seconds
  if (millis() - lastConnectionCheck >= 5000) {
    if (currentCharsProcessed == lastCharsProcessed) {
      sensorData.gps_is_connected = false;
      sensorData.gps_is_valid = false;
    } else {
      sensorData.gps_is_connected = true;
    }
    lastCharsProcessed = currentCharsProcessed;
    lastConnectionCheck = millis();
  }
}
