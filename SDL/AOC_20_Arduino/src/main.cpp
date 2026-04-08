#include <Arduino.h>
#include <DRV8834.h>

#define RACK_STEPS 200
#define RACK_DIR_PIN 8
#define RACK_STEP_PIN 9
#define RACK_RPM 60

#define RACK_LEFT_BTN 3
#define RACK_RIGHT_BTN 2

// Board is 1/4 microstepping by default
#define MICROSTEPS 4

DRV8834 rackMotor(RACK_STEPS, RACK_DIR_PIN, RACK_STEP_PIN);

void setup() {
  rackMotor.begin(RACK_RPM, MICROSTEPS); 
  //rackMotor.setSpeedProfile(BasicStepperDriver::LINEAR_SPEED, 1000, 1000); 

  pinMode(RACK_LEFT_BTN, INPUT);
  pinMode(RACK_RIGHT_BTN, INPUT);
}

void loop() {
  if (digitalRead(RACK_LEFT_BTN) == HIGH) {
    Serial.println("Left button pressed");
    rackMotor.move(-800); // Move left by 800 microsteps
  }
  else if (digitalRead(RACK_RIGHT_BTN) == HIGH) {
    Serial.println("Right button pressed");
    rackMotor.move(800); // Move right by 800 microsteps
  }
}

