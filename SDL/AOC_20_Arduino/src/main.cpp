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
#define RACK_STEPS_PER_VIAL (RACK_VIAL_SPACING / RACK_MM_PER_MICROSTEP)
int rack_position = 0; // in microsteps, 0 is the rightmost vial position

#define Z_AXIS_STEPS 200 // Fullsteps / revolution
#define Z_AXIS_DIR_PIN 12
#define Z_AXIS_STEP_PIN 10
#define Z_AXIS_RPM 30
#define Z_AXIS_MAX_HEIGHT 100 // mm
#define Z_AXIS_MM_PER_MICROSTEP 0.0875 // mm / microstep
#define Z_AXIS_MAX_STEPS (Z_AXIS_MAX_HEIGHT / Z_AXIS_MM_PER_MICROSTEP)
#define Z_AXIS_MIN_HEIGHT 30 // mm
#define Z_AXIS_MIN_STEPS (Z_AXIS_MIN_HEIGHT / Z_AXIS_MM_PER_MICROSTEP)
int z_axis_position = 0; // in microsteps, 0 is the lowest position

#define PLUNGER_STEPS 200 // Fullsteps / revolution
#define PLUNGER_DIR_PIN 13
#define PLUNGER_STEP_PIN 11
#define PLUNGER_RPM 30
#define PLUNGER_MAX_HEIGHT 10 // microLiters
#define PLUNGER_uL_PER_MICROSTEP 0.0125 // microLiters / microstep
#define PLUNGER_MAX_STEPS (PLUNGER_MAX_HEIGHT / PLUNGER_uL_PER_MICROSTEP)
#define PLUNGER_MIN_STEPS 0 
int plunger_position = 0; // in microsteps, 0 is the lowest position


DRV8834 rackMotor(RACK_STEPS, RACK_DIR_PIN, RACK_STEP_PIN);
DRV8834 zAxisMotor(Z_AXIS_STEPS, Z_AXIS_DIR_PIN, Z_AXIS_STEP_PIN);
DRV8834 plungerMotor(PLUNGER_STEPS, PLUNGER_DIR_PIN, PLUNGER_STEP_PIN);

#define LED_PIN 13
#define ENABLE_PIN 7

// Utility
int waitReadInt();
char waitReadChar();
void moveZSteps(float steps);
void movePlungerSteps(float steps);
bool isHomed;
bool isEnabled;

// Injector control functions
void homeInjector();
void rackRight();
void rackLeft();
void lowerSyringe();
void raiseSyringe();
void fillPlunger();
void dispensePlunger();
void enableDrivers();
void disableDrivers();
void printCurrentPositions();


void setup() {
  Serial.begin(9600);
  rackMotor.begin(RACK_RPM, MICROSTEPS); 
  zAxisMotor.begin(Z_AXIS_RPM, MICROSTEPS);
  plungerMotor.begin(PLUNGER_RPM, MICROSTEPS);

  isHomed = false;

  //rackMotor.setSpeedProfile(BasicStepperDriver::LINEAR_SPEED, 1000, 1000); 

  //pinMode(LED_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  disableDrivers();
}

void loop() {
  Serial.println("=== Injector Manual Control ===");
  Serial.println("H: Home Axes");
  Serial.println("W: Raise Syringe (Z up)");
  Serial.println("S: Lower Syringe (Z down)");
  Serial.println("A: Move Rack Left");
  Serial.println("D: Move Rack Right");
  Serial.println("R: Fill Plunger");
  Serial.println("F: Dispense Plunger");
  printCurrentPositions();
  Serial.print("Enter command: ");

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
  if (waitReadInt() == 1) {
    isHomed = true;
    rack_position = 0;
    z_axis_position = Z_AXIS_MAX_STEPS;
    plunger_position = 0;
    Serial.println("Homed.");
    enableDrivers();
  }
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

  zAxisMotor.move(steps);
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
  int choice = waitReadInt();

  if (choice == 1) {
    moveZSteps((Z_AXIS_MAX_STEPS - z_axis_position));
  } 
  else if (choice == 2) {
    Serial.println("Enter raise amount in mm:");
    float mm = waitReadInt();

    if (mm <= 0) {
      Serial.println("Error: Enter positive value.");
      return;
    }

    moveZSteps(mm / Z_AXIS_MM_PER_MICROSTEP);
  }
}

void lowerSyringe() {
  Serial.println("Enter 1 to fully lower syringe, 2 for custom amount:");
  int choice = waitReadInt();

  if (choice == 1) {
    moveZSteps(-z_axis_position);
  } 
  else if (choice == 2) {
    Serial.println("Enter lower amount in mm:");
    float mm = waitReadInt();

    if (mm <= 0) {
      Serial.println("Error: Enter positive value.");
      return;
    }

    moveZSteps(-mm / Z_AXIS_MM_PER_MICROSTEP);
  }
}

void rackRight() {
  Serial.println("Enter 1 to move right one vial, 2 for custom amount:");
  int choice = waitReadInt();

  if (choice == 1) {
    rackMotor.move(RACK_STEPS_PER_VIAL);
    rack_position += RACK_STEPS_PER_VIAL;
  } 
  else if (choice == 2) {
    Serial.println("Enter move amount in mm:");
    float mm = waitReadInt();

    if (mm <= 0) {
      Serial.println("Error: Enter positive value.");
      return;
    }

    long steps = mm / RACK_MM_PER_MICROSTEP;
    rackMotor.move(steps);
    rack_position += steps;
  }
}

void rackLeft() {
  Serial.println("Enter 1 to move left one vial, 2 for custom amount:");
  int choice = waitReadInt();

  if (choice == 1) {
    rackMotor.move(-RACK_STEPS_PER_VIAL);
    rack_position -= RACK_STEPS_PER_VIAL;
  } 
  else if (choice == 2) {
    Serial.println("Enter move amount in mm:");
    float mm = waitReadInt();

    if (mm <= 0) {
      Serial.println("Error: Enter positive value.");
      return;
    }

    long steps = mm / RACK_MM_PER_MICROSTEP;
    rackMotor.move(-steps);
    rack_position -= steps;
  }
}

void fillPlunger() {
  Serial.println("Enter 1 to fully fill plunger, 2 for custom amount:");
  int choice = waitReadInt();

  if (choice == 1) {
    movePlungerSteps((PLUNGER_MAX_STEPS - plunger_position));
  } 
  else if (choice == 2) {
    Serial.println("Enter fill amount in uL:");
    float uL = waitReadInt();

    if (uL <= 0) {
      Serial.println("Error: Enter positive value.");
      return;
    }

    movePlungerSteps(uL / PLUNGER_uL_PER_MICROSTEP);
  }
}

void dispensePlunger() {
  Serial.println("Enter 1 to fully dispense plunger, 2 for custom amount:");
  int choice = waitReadInt();

  if (choice == 1) {
    movePlungerSteps(-plunger_position);
  } 
  else if (choice == 2) {
    Serial.println("Enter dispense amount in uL:");
    float uL = waitReadInt();

    if (uL <= 0) {
      Serial.println("Error: Enter positive value.");
      return;
    }

    movePlungerSteps(-uL / PLUNGER_uL_PER_MICROSTEP);
  }
}