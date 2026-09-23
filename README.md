# CanSat

ESP32 CanSat sensor telemetry firmware with standalone & integrated modules for GPS, BMP280, MPU6050, QMC5883L, and DHT11. Features onboard calibration, MicroSD CSV logging, and real-time Wi-Fi ground station streaming.

---

## 📂 Project Structure

| File | Description | Hardware / Protocol |
| :--- | :--- | :--- |
| **`GPS.ino`** | Standalone GPS module reading | Ublox NEO-6M (UART2: RX=17, TX=16) |
| **`QMC5883L.ino`** | Digital compass heading & orientation | QMC5883L (I2C: SDA=18, SCL=19) |
| **`BMP280.ino`** | Barometric pressure, altitude & temperature | BMP280 (I2C: SDA=21, SCL=22) |
| **`MPU6050.ino`** | 6-axis accelerometer & gyroscope | MPU6050 (I2C: SDA=21, SCL=22) |
| **`DHT11.ino`** | Temperature & humidity measurement | DHT11 (Digital: GPIO 14) |
| **`All_Combined.ino`** | Full sensor suite with calibration and MicroSD CSV logging | Multi-I2C + SPI SD Card |
| **`Server_Code.ino`** | Ground station Wi-Fi AP & AsyncWebServer | ESP32 Access Point (IP: `192.168.4.1`) |
| **`Client_Code.ino`** | CanSat flight firmware with sensors, SD logging & Wi-Fi telemetry | ESP32 Wi-Fi Station + HTTP Client |

---

## 🔌 Pin Configuration

### Sensors & Peripherals
* **MPU6050 (I2C - Default Wire)**:
  * SDA: `GPIO 21`
  * SCL: `GPIO 22`
* **BMP280 + QMC5883L (Shared Wire1)**:
  * SDA: `GPIO 17`
  * SCL: `GPIO 16`
* **GPS (NEO-6M UART2)**:
  * RXD2: `GPIO 15` (or `17` in standalone)
  * TXD2: `GPIO 4` (or `16` in standalone)
  * Baud: `9600`
* **DHT11**:
  * Data: `GPIO 14`
* **MicroSD Card (SPI)**:
  * CS: `GPIO 5`
  * MOSI: `GPIO 23`
  * MISO: `GPIO 19`
  * SCK: `GPIO 18`

---

## 📚 Required Libraries

Install the following libraries via the Arduino IDE Library Manager:
* [TinyGPSPlus](https://github.com/mikalhart/TinyGPSPlus)
* [Adafruit BMP280 Library](https://github.com/adafruit/Adafruit_BMP280_Library)
* [MPU6050 by Electronic Cats](https://github.com/ElectronicCats/mpu6050)
* [QMC5883LCompass](https://github.com/mprograms/QMC5883LCompass)
* [DHT sensor library](https://github.com/adafruit/DHT-sensor-library)
* [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
* [AsyncTCP](https://github.com/me-no-dev/AsyncTCP)
* [ArduinoJson](https://arduinojson.org/)
