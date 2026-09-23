#include "DHT.h"

#define DHTPIN 14 // DHT11 data pin connected to GPIO14
#define DHTTYPE DHT11 // Define sensor type

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  delay(1000); // Wait for Serial Monitor
  dht.begin();
  Serial.println("DHT11 Sensor Reading Started...");
}

void loop() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature(); // in Celsius

  // Check if read failed
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT11 sensor!");
    return;
  }

  // Output to Serial Monitor
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print("°C | Humidity: ");
  Serial.print(humidity);
  Serial.println("%");

  delay(2000); // Delay between readings
}
