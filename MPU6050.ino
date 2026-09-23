#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

void setup() {
  Serial.begin(115200);
  delay(1000); // wait for Serial Monitor to open

  // Start I2C on custom ESP32 pins
  Wire.begin(21, 22);

  Serial.println("Initializing MPU6050...");
  mpu.initialize();

  // Check if MPU6050 is connected
  if (mpu.testConnection()) {
    Serial.println("MPU6050 connection successful!");
  } else {
    Serial.println("MPU6050 connection failed. Check wiring.");
    while (1); // halt
  }
}

void loop() {
  int16_t ax, ay, az;
  int16_t gx, gy, gz;

  // Read raw accelerometer and gyroscope values
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // Print to Serial Monitor
  Serial.print("aX: "); Serial.print(ax);
  Serial.print(" | aY: "); Serial.print(ay);
  Serial.print(" | aZ: "); Serial.print(az);
  Serial.print(" || gX: "); Serial.print(gx);
  Serial.print(" | gY: "); Serial.print(gy);
  Serial.print(" | gZ: "); Serial.println(gz);

  // For Serial Plotter (tab separated)
  Serial.print(ax); Serial.print("\t");
  Serial.print(ay); Serial.print("\t");
  Serial.print(az); Serial.print("\t");
  Serial.print(gx); Serial.print("\t");
  Serial.print(gy); Serial.print("\t");
  Serial.println(gz);

  delay(100); // ~10Hz update rate
}
