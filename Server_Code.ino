/*
 * ESP32 - Wi-Fi Base Station Server
 *
 * This sketch configures the ESP32 as a Wi-Fi Access Point (AP)
 * and a web server to receive sensor data from a client ESP32.
 * The data is received as a JSON payload via an HTTP POST request.
 *
 * Requirements:
 * - ESPAsyncWebServer library (can be installed via Library Manager)
 * - AsyncTCP library (required by ESPAsyncWebServer)
 * - ArduinoJson library (can be installed via Library Manager)
 */

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// ====== WiFi Access Point Credentials ======
// This must match the SSID and password in the client sketch.
const char* ssid = "ESP32-Sensor-Network";
const char* password = "sensor_password";

// Create an AsyncWebServer object on port 80
AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("[ESP32] BASE STATION SERVER");
  Serial.println("===========================");

  // === Configure ESP32 as an Access Point (AP) ===
  Serial.printf("Setting up Access Point '%s'...", ssid);
  WiFi.softAP(ssid, password);
  Serial.println("✅ OK");

  // Get and print the IP address of the AP
  IPAddress IP = WiFi.softAPIP();
  Serial.printf("Access Point IP address: %s\n", IP.toString().c_str());

  // === Configure the server routes ===
  // This route will handle incoming POST requests from the client.
  server.on("/data", HTTP_POST, [](AsyncWebServerRequest *request){
    // Check if the request body is valid
    if (request->hasParam("plain", true)) {
      // Get the JSON data from the request body
      String jsonString = request->arg("plain");

      // Print the raw JSON string to verify it was received
      Serial.printf("Received JSON payload:\n%s\n", jsonString.c_str());

      // Use a StaticJsonDocument to parse the JSON
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, jsonString);

      // Check for parsing errors
      if (error) {
        Serial.printf("❌ JSON parsing failed! Error: %s\n", error.c_str());
        request->send(400, "text/plain", "Bad Request: Invalid JSON");
        return;
      }

      // Extract data from the JSON document
      float time = doc["time"];
      float temperature = doc["temp_C"];
      float pressure = doc["press_hPa"];
      float altitude = doc["alt_m"];
      float accel_x = doc["accel_x_g"];
      float accel_y = doc["accel_y_g"];
      float accel_z = doc["accel_z_g"];
      float gyro_x = doc["gyro_x_dps"];
      float gyro_y = doc["gyro_y_dps"];
      float gyro_z = doc["gyro_z_dps"];
      float heading_raw = doc["heading_raw_deg"];
      float heading_tilt = doc["heading_tilt_deg"];

      // Print the received data to the Serial Monitor
      Serial.printf("\n=======================================================\n");
      Serial.printf("✅ Data Received at %.1fs\n", time);
      Serial.printf("Temp: %.2f°C | Press: %.2fhPa | Alt: %.1fm\n", temperature, pressure, altitude);
      Serial.printf("Accel: (%.3f, %.3f, %.3f) g\n", accel_x, accel_y, accel_z);
      Serial.printf("Gyro: (%.2f, %.2f, %.2f) °/s\n", gyro_x, gyro_y, gyro_z);
      Serial.printf("Heading: %.1f°, Tilt-Compensated: %.1f°\n", heading_raw, heading_tilt);

      // Check for and print GPS data if available
      if (doc.containsKey("gps_lat")) {
        float gps_lat = doc["gps_lat"];
        float gps_lng = doc["gps_lng"];
        float gps_alt = doc["gps_alt_m"];
        float gps_speed = doc["gps_speed_kph"];
        int gps_satellites = doc["gps_satellites"];
        Serial.printf("GPS: Lat:%.4f, Lng:%.4f, Alt:%.1fm, Spd:%.2fkm/h, Sats:%d\n",
          gps_lat, gps_lng, gps_alt, gps_speed, gps_satellites);
      } else {
        Serial.println("GPS: No fix available.");
      }

      Serial.println("=======================================================\n");

      // Send a success response back to the client
      request->send(200, "text/plain", "OK");
    } else {
      // If the request body is empty or malformed
      request->send(400, "text/plain", "Bad Request: No data received.");
    }
  });

  // Start the server
  server.begin();
  Serial.println("Web server started, awaiting data...");
}

void loop() {
  // Nothing needed in the loop for the server, as the web server is asynchronous.
}
