#include <TinyGPS++.h>

// Your GPS wiring
#define RXD2 17 // GPS TX connected to ESP32 RX
#define TXD2 16 // GPS RX connected to ESP32 TX
#define GPS_BAUD 9600

TinyGPSPlus gps;
HardwareSerial gpsSerial(2); // Use UART2

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2); // Start GPS Serial
  Serial.println("CanSat GPS Module Initialized");
}

void loop() {
  // Read incoming GPS data
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // If new data is available, print it
  if (gps.location.isUpdated()) {
    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 6);
    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 6);
  }

  if (gps.altitude.isUpdated()) {
    Serial.print("Altitude: ");
    Serial.print(gps.altitude.meters());
    Serial.println(" m");
  }

  if (gps.speed.isUpdated()) {
    Serial.print("Speed: ");
    Serial.print(gps.speed.kmph());
    Serial.println(" km/h");
  }

  if (gps.time.isUpdated()) {
    Serial.print("Time (UTC): ");
    Serial.print(gps.time.hour());
    Serial.print(":");
    Serial.print(gps.time.minute());
    Serial.print(":");
    Serial.println(gps.time.second());
  }

  Serial.println("--------------------------");
  delay(1000); // Update every second
}
