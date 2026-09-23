#include <Wire.h>
#include <QMC5883LCompass.h>

QMC5883LCompass compass;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Use working I2C pins: SDA = 18, SCL = 19
  Wire.begin(18, 19);

  compass.init();
  Serial.println("QMC5883L Initialized (using Wire on 18/19)");
  delay(500);
}

void loop() {
  compass.read();

  int x = compass.getX();
  int y = compass.getY();
  int z = compass.getZ();
  int azimuth = compass.getAzimuth();

  char direction[4];
  compass.getDirection(direction, azimuth);

  // --- For Serial Monitor ---
  Serial.print("X: "); Serial.print(x);
  Serial.print(" | Y: "); Serial.print(y);
  Serial.print(" | Z: "); Serial.print(z);
  Serial.print(" | Heading: "); Serial.print(azimuth);
  Serial.print("° "); Serial.println(direction);

  // --- For Serial Plotter ---
  Serial.print("Plot_X: "); Serial.print(x);
  Serial.print(" Plot_Y: "); Serial.print(y);
  Serial.print(" Plot_Z: "); Serial.print(z);
  Serial.print(" Heading: "); Serial.println(azimuth);

  delay(300); // smooth updates
}
