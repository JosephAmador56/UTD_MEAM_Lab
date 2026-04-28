#include <Arduino.h>
#include <DRV8834.h>

#define RACK_STEPS 200
#define RACK_DIR_PIN 8
#define RACK_STEP_PIN 9
#define RACK_RPM 30

#define Z_AXIS_STEPS 200
#define Z_AXIS_DIR_PIN 12
#define Z_AXIS_STEP_PIN 10
#define Z_AXIS_RPM 30

#define PLUNGER_STEPS 200
#define PLUNGER_DIR_PIN 13
#define PLUNGER_STEP_PIN 11
#define PLUNGER_RPM 30

// Board is 1/4 microstepping by default
#define MICROSTEPS 4

DRV8834 rackMotor(RACK_STEPS, RACK_DIR_PIN, RACK_STEP_PIN);
DRV8834 zAxisMotor(Z_AXIS_STEPS, Z_AXIS_DIR_PIN, Z_AXIS_STEP_PIN);
DRV8834 plungerMotor(PLUNGER_STEPS, PLUNGER_DIR_PIN, PLUNGER_STEP_PIN);

#define LED_PIN 13

// Utility
int waitReadInt();
void readOption();

// Control Functions
int homeInjector();
int nextVial();
int prevVial();
int lowerSyringe();
int raiseSyringe();
int fillPlunger();
int dispensePlunger();
int moveRackManual();


void setup() {
  Serial.begin(9600);
  rackMotor.begin(RACK_RPM, MICROSTEPS); 
  zAxisMotor.begin(Z_AXIS_RPM, MICROSTEPS);
  plungerMotor.begin(PLUNGER_RPM, MICROSTEPS);

  //rackMotor.setSpeedProfile(BasicStepperDriver::LINEAR_SPEED, 1000, 1000); 

  //pinMode(LED_PIN, OUTPUT);
}

void loop() {
  Serial.println("=== Injector Control Menu ===");
  Serial.println("Select category:");
  Serial.println("1. Rack Control");
  Serial.println("2. Syringe Control");
  Serial.println("3. Plunger Control");
  Serial.println("4. Manual Control");

  int cntrl_choice = waitReadInt();
  int option = -1;

  switch (cntrl_choice) {
    case 1:{
      Serial.println("--- Rack Control ---");
      Serial.println("1. Home Injector");
      Serial.println("2. Next Vial");
      Serial.println("3. Previous Vial");
      Serial.println("4. Move Manual Steps");
      option = waitReadInt();
      break;
    }
    case 2:{
      Serial.println("--- Syringe Control ---");
      Serial.println("1. Lower");
      Serial.println("2. Raise");
      option = waitReadInt();
      break;
    }
    case 3:{
      Serial.println("--- Plunger Control ---");
      Serial.println("1. Fill");
      Serial.println("2. Dispense");
      option = waitReadInt();
      break;
    }
    case 4:{
      Serial.println("--- Manual Control ---");
      Serial.println("1. Fill");
      Serial.println("2. Dispense");
      option = waitReadInt();
      break;
    }
    default:{
      Serial.println("Invalid option. Please try again.");
    }
  }


  

}

int waitReadInt() {
  String input = "";

  while (true) {
    if (Serial.available()) {
      char c = Serial.read();

      // Enter key pressed
      if (c == '\n') {
        break;
      }

      // ignore carriage return
      if (c != '\r') {
        input += c;
      }
    }
  }

  return input.toInt();
}

void readOption() {

}
