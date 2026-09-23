#include <Wire.h>
#include <Adafruit_BMP280.h>

// Create BMP280 object
Adafruit_BMP280 bmp;

void setup() {
  Serial.begin(115200);
  delay(1000); // Give time for Serial Monitor to start

  // Initialize I2C with custom ESP32 pins
  Wire.begin(21, 22); // SDA = GPIO21, SCL = GPIO22

  // Try to initialize BMP280 with correct address
  if (!bmp.begin(0x76)) {
    Serial.println("BMP280 not found at 0x76. Check wiring or try 0x77.");
    while (true); // Stop if sensor not found
  }

  Serial.println("BMP280 detected successfully!");
}

void loop() {
  // Read and print temperature
  Serial.print("Temperature = ");
  Serial.print(bmp.readTemperature());
  Serial.println(" °C");

  // Read and print pressure
  Serial.print("Pressure = ");
  Serial.print(bmp.readPressure());
  Serial.println(" Pa");

  // Optional: Read altitude (requires known sea level pressure)
  Serial.print("Approx. Altitude = ");
  Serial.print(bmp.readAltitude(1013.25)); // Update with your local sea level pressure
  Serial.println(" m");

  delay(2000); // Wait 2 seconds before next reading
}
