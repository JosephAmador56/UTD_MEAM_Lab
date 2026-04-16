#include <Arduino.h>
#include <DRV8834.h>

#define RACK_STEPS 200
#define RACK_DIR_PIN 8
#define RACK_STEP_PIN 9
#define RACK_RPM 60

// Board is 1/4 microstepping by default
#define MICROSTEPS 4

DRV8834 rackMotor(RACK_STEPS, RACK_DIR_PIN, RACK_STEP_PIN);

#define LED_PIN 13

void setup() {
  Serial.begin(9600);
  rackMotor.begin(RACK_RPM, MICROSTEPS); 
  //rackMotor.setSpeedProfile(BasicStepperDriver::LINEAR_SPEED, 1000, 1000); 

  //pinMode(LED_PIN, OUTPUT);
}

void loop() {
  Serial.println("Please selected an option:");
  Serial.println("1. Move left 800 microsteps");
  Serial.println("2. Move right 800 microsteps");
  Serial.println("3. Move X microsteps");

  int choice = waitnReadInt();

  switch (choice) {
    case 1:
      Serial.println("Moving left...");
      rackMotor.move(-800); // Move left by 800 microsteps
      break;
    case 2:
      Serial.println("Moving right...");
      rackMotor.move(800); // Move right by 800 microsteps
      break;
    case 3:
      Serial.println("Enter number of microsteps to move (positive for right, negative for left):");
      int steps = waitnReadInt();
      Serial.print("Moving ");
      Serial.print(steps);
      Serial.println(" microsteps...");
      rackMotor.move(steps);
      break;
    default:
      Serial.println("Invalid option. Please try again.");
  }
}

int waitnReadInt() {
  while (Serial.available() == 0) {
    // wait for input
  }
  return Serial.parseInt();
}
