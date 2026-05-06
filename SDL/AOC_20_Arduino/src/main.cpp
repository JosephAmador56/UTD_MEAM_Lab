#include <Arduino.h>
#include <DRV8834.h>

// Note: All axes are defined with a right hand coordinate system in mind, so positive movement is rightward for the rack and upward for the z-axis and plunger.

// Note to self, also need to change the enable pin to a digital pin and set it to HIGH to enable the drivers, LOW to disable

// Board is 1/4 microstepping by default
#define MICROSTEPS 4

#define RACK_STEPS 200 // Fullsteps / revolution
#define RACK_DIR_PIN 8
#define RACK_STEP_PIN 9
#define RACK_RPM 30
#define RACK_MM_PER_MICROSTEP 0.04345 // mm / microstep
#define RACK_VIAL_SPACING 9.5 // mm
#define RACK_STEPS_PER_VIAL lround(RACK_VIAL_SPACING / RACK_MM_PER_MICROSTEP)
int rack_position = 0; // in microsteps, 0 is the rightmost vial position

#define Z_AXIS_STEPS 200 // Fullsteps / revolution
#define Z_AXIS_DIR_PIN 12
#define Z_AXIS_STEP_PIN 10
#define Z_AXIS_RPM 15
#define Z_AXIS_MAX_HEIGHT 125 // mm
#define Z_AXIS_MM_PER_MICROSTEP 0.0875 // mm / microstep
#define Z_AXIS_MAX_STEPS lround(Z_AXIS_MAX_HEIGHT / Z_AXIS_MM_PER_MICROSTEP)
#define Z_AXIS_MIN_HEIGHT 85 // mm
#define Z_AXIS_MIN_STEPS lround(Z_AXIS_MIN_HEIGHT / Z_AXIS_MM_PER_MICROSTEP)
int z_axis_position = 0; // in microsteps, 0 is the lowest position

#define PLUNGER_STEPS 200 // Fullsteps / revolution
#define PLUNGER_DIR_PIN 13
#define PLUNGER_STEP_PIN 11
#define PLUNGER_RPM 10
#define PLUNGER_MAX_HEIGHT 10 // microLiters
#define PLUNGER_uL_PER_MICROSTEP 0.0125 // microLiters / microstep
#define PLUNGER_MAX_STEPS lround(PLUNGER_MAX_HEIGHT / PLUNGER_uL_PER_MICROSTEP)
#define PLUNGER_MIN_STEPS 0 
int plunger_position = 0; // in microsteps, 0 is the lowest position


DRV8834 rackMotor(RACK_STEPS, RACK_DIR_PIN, RACK_STEP_PIN);
DRV8834 zAxisMotor(Z_AXIS_STEPS, Z_AXIS_DIR_PIN, Z_AXIS_STEP_PIN);
DRV8834 plungerMotor(PLUNGER_STEPS, PLUNGER_DIR_PIN, PLUNGER_STEP_PIN);

#define LED_PIN 13
#define ENABLE_PIN 7

// Utility
float waitReadFloat();
char waitReadChar();
void moveZSteps(float steps);
void movePlungerSteps(float steps);
bool isHomed;
bool isEnabled;

// Injector control functions

// Homes the axes by moving them to their known limits. Assumes user manually moves the axes to the home position and confirms by entering 1.
void homeInjector();
// Moves the rack right by one vial spacing or a custom amount in mm entered by the user.
void rackRight();
// Moves the rack left by one vial spacing or a custom amount in mm entered by the user.
void rackLeft();
// Raises the syringe by moving the z-axis upwards. Prompts user to enter 1 to fully raise or 2 to enter a custom amount in mm.
void lowerSyringe();
// Lowers the syringe by moving it downwards. Prompts user to enter 1 to fully lower or 2 to enter a custom amount in mm.
void raiseSyringe();
// Fills the plunger by moving it upwards. Prompts user to enter 1 to fully fill or 2 to enter a custom amount in uL.
void fillPlunger();
// Dispenses the plunger by moving it downwards. Prompts user to enter 1 to fully dispense or 2 to enter a custom amount in uL.
void dispensePlunger();
// Enables the stepper drivers by setting the enable pin HIGH. Note that the drivers are disabled by default on startup, they are only enabled once the injector is homed.
void enableDrivers();
// Disables the stepper drivers by setting the enable pin LOW.
void disableDrivers();
// Prints the current positions of the axes in mm for the rack and z-axis and uL for the plunger.
void printCurrentPositions();


void setup() {
  Serial.begin(9600);
  rackMotor.begin(RACK_RPM, MICROSTEPS); 
  zAxisMotor.begin(Z_AXIS_RPM, MICROSTEPS);
  plungerMotor.begin(PLUNGER_RPM, MICROSTEPS);

  isHomed = false;

  rackMotor.setSpeedProfile(BasicStepperDriver::LINEAR_SPEED, 250, 250);
  zAxisMotor.setSpeedProfile(BasicStepperDriver::LINEAR_SPEED, 250, 250); 
  plungerMotor.setSpeedProfile(BasicStepperDriver::LINEAR_SPEED, 100, 100); 

  //pinMode(LED_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  disableDrivers();
}

void loop() {
  Serial.println("\n=== Injector Manual Control ===");
  Serial.println("H: Home Axes");
  Serial.println("W: Raise Syringe (Z up)");
  Serial.println("S: Lower Syringe (Z down)");
  Serial.println("A: Move Rack Left");
  Serial.println("D: Move Rack Right");
  Serial.println("R: Fill Plunger");
  Serial.println("F: Dispense Plunger");
  printCurrentPositions();
  Serial.println("Enter command: ");

  char cntrl_choice = waitReadChar();

  if (!isHomed && cntrl_choice != 'H') {
    Serial.println("Please home the axes first (press H).");
    return;
  }

  switch (cntrl_choice) {
    case 'H': {
      homeInjector();
      break;
    }
    case 'W': {
      raiseSyringe();
      break;
    }
    case 'S': {
      lowerSyringe();
      break;
    }
    case 'A': {
      rackLeft();
      break;
    }
    case 'D': {
      rackRight();
      break;
    }
    case 'R': {
      fillPlunger();
      break;
    }
    case 'F': {
      dispensePlunger();
      break;
    }
    default:{
      Serial.println("Invalid option. Please try again.");
    }
  }
}

float waitReadFloat() {
  String input = "";

  while (true) {
    if (Serial.available()) {
      char c = Serial.read();

      // Enter key pressed
      if (c == '\n' || c == '\r') {
        break;
      }

      // ignore carriage return
      if (c != '\r') {
        input += c;
      }
    }
  }

  return input.toFloat();
}

char waitReadChar() {
  while (true) {
    if (Serial.available()) {
      char c = Serial.read();

      if (c != '\n' && c != '\r') {
        return c;
      }
    }
  }
}

void printCurrentPositions() {
  Serial.print("Current Positions - Rack: ");
  Serial.print(rack_position * RACK_MM_PER_MICROSTEP, 2);
  Serial.print(" mm, Z-Axis: ");
  Serial.print(z_axis_position * Z_AXIS_MM_PER_MICROSTEP, 2);
  Serial.print(" mm, Plunger: ");
  Serial.print(plunger_position * PLUNGER_uL_PER_MICROSTEP, 2);
  Serial.println(" uL");
}

void enableDrivers() {
  digitalWrite(ENABLE_PIN, HIGH);
  isEnabled = true;
}

void disableDrivers() {
  digitalWrite(ENABLE_PIN, LOW);
  isEnabled = false;
}


void homeInjector() {
  Serial.println("Manually move axes to home, then enter 1 to confirm:");
  disableDrivers();
  isHomed = false;
  if (waitReadFloat() == 1) {
    isHomed = true;
    rack_position = 0;
    z_axis_position = Z_AXIS_MAX_STEPS;
    plunger_position = 0;
    Serial.println("Homed.");
    enableDrivers();
    return;
  }
  Serial.println("Homing cancelled.");
}

void moveZSteps(float steps) {
  long new_position = z_axis_position + steps;

  if (new_position > Z_AXIS_MAX_STEPS) {
    Serial.println("Error: Exceeds maximum height.");
    return;
  }

  if (new_position < Z_AXIS_MIN_STEPS) {
    Serial.println("Error: Exceeds minimum height.");
    return;
  }

  zAxisMotor.move(-steps);
  z_axis_position = new_position;
}


void movePlungerSteps(float steps) {
  long new_position = plunger_position + steps;

  if (new_position > PLUNGER_MAX_STEPS) {
    Serial.println("Error: Exceeds maximum plunger height.");
    return;
  }

  if (new_position < PLUNGER_MIN_STEPS) {
    Serial.println("Error: Exceeds minimum plunger height.");
    return;
  }

  plungerMotor.move(steps);
  plunger_position = new_position;
}

void raiseSyringe() {
  Serial.println("Enter 1 to fully raise syringe, 2 for custom amount:");
  int choice = waitReadFloat();

  if (choice == 1) {
    Serial.println("Moving...");
    moveZSteps(lround(Z_AXIS_MAX_STEPS - z_axis_position));
  } 
  else if (choice == 2) {
    Serial.println("Enter raise amount in mm:");
    float mm = waitReadFloat();

    if (mm <= 0) {
      Serial.println("Error: Enter positive value.");
      return;
    }

    Serial.println("Moving...");
    moveZSteps(lround(mm / Z_AXIS_MM_PER_MICROSTEP));
  }
}

void lowerSyringe() {
  Serial.println("Enter 1 to fully lower syringe, 2 for custom amount:");
  int choice = waitReadFloat();

  if (choice == 1) {
    Serial.println("Moving...");
    moveZSteps(lround(Z_AXIS_MIN_STEPS - z_axis_position));
  } 
  else if (choice == 2) {
    Serial.println("Enter lower amount in mm:");
    float mm = waitReadFloat();

    if (mm <= 0) {
      Serial.println("Error: Enter positive value.");
      return;
    }

    Serial.println("Moving...");
    moveZSteps(-lround(mm / Z_AXIS_MM_PER_MICROSTEP));
  }
}

void rackRight() {
  Serial.println("Enter 1 to move right one vial, 2 for custom amount:");
  int choice = waitReadFloat();

  if (choice == 1) {
    Serial.println("Moving...");
    rackMotor.move(RACK_STEPS_PER_VIAL);
    rack_position += RACK_STEPS_PER_VIAL;
  } 
  else if (choice == 2) {
    Serial.println("Enter move amount in mm:");
    float mm = waitReadFloat();

    if (mm <= 0) {
      Serial.println("Error: Enter positive value.");
      return;
    }

    Serial.println("Moving...");
    long steps = lround(mm / RACK_MM_PER_MICROSTEP);
    rackMotor.move(steps);
    rack_position += steps;
  }
}

void rackLeft() {
  Serial.println("Enter 1 to move left one vial, 2 for custom amount:");
  int choice = waitReadFloat();

  if (choice == 1) {
    Serial.println("Moving...");
    rackMotor.move(-RACK_STEPS_PER_VIAL);
    rack_position -= RACK_STEPS_PER_VIAL;
  } 
  else if (choice == 2) {
    Serial.println("Enter move amount in mm:");
    float mm = waitReadFloat();

    if (mm <= 0) {
      Serial.println("Error: Enter positive value.");
      return;
    }

    Serial.println("Moving...");
    long steps = lround(mm / RACK_MM_PER_MICROSTEP);
    rackMotor.move(-steps);
    rack_position -= steps;
  }
}

void fillPlunger() {
  Serial.println("Enter 1 to fully fill plunger, 2 for custom amount:");
  int choice = waitReadFloat();

  if (choice == 1) {
    Serial.println("Filling...");
    movePlungerSteps(lround(PLUNGER_MAX_STEPS - plunger_position));
  } 
  else if (choice == 2) {
    Serial.println("Enter fill amount in uL:");
    float uL = waitReadFloat();

    if (uL <= 0) {
      Serial.println("Error: Enter positive value.");
      return;
    }

    Serial.println("Filling...");
    movePlungerSteps(lround(uL / PLUNGER_uL_PER_MICROSTEP));
  }
}

void dispensePlunger() {
  Serial.println("Enter 1 to fully dispense plunger, 2 for custom amount:");
  int choice = waitReadFloat();

  if (choice == 1) {
    Serial.println("Dispensing...");
    movePlungerSteps(lround(PLUNGER_MIN_STEPS - plunger_position));
  } 
  else if (choice == 2) {
    Serial.println("Enter dispense amount in uL:");
    float uL = waitReadFloat();

    if (uL <= 0) {
      Serial.println("Error: Enter positive value.");
      return;
    }

    Serial.println("Dispensing...");
    movePlungerSteps(-lround(uL / PLUNGER_uL_PER_MICROSTEP));
  }
}