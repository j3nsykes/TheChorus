/********************************************************************************************
*         File:  MovingMotor_uStepper.ino                                                 *
*      Version:  3.1.0                                                                     *
*         Date:  April 27th, 2025                                                          *
*  Description:  Moving Motor Control for uStepper S32 with absolute positioning           *
*                This sketch uses the MovingMotor class to control a stepper motor         *
*                with absolute position references for both start and end positions        *
*                Added automatic homing on power-up                                        *
*                                                                                          *
* For more information about uStepper S32:                                                 *
*    http://ustepper.com/docs/usteppers32/html/index.html                                  *
*                                                                                          *
********************************************************************************************/

#include <UstepperS32.h>
#include "MovingMotor.h"

// Define constants
#define STEPSPERREV 200    // 200 steps per revolution (1.8° motor)
#define RES (STEPSPERREV * 256)/360.0  // Steps per degree
#define STALLSENSITIVITY 2  // Default stall sensitivity
#define HOME_POSITION 0.55  // Home position angle
#define FORCE_HOMING true   // Enable forced homing sequence

// Global variables
UstepperS32 stepper;
bool setupComplete = false;
bool homingComplete = false;

// Motor control parameters
uint32_t maxVelocity = 500;      // Default max velocity
uint32_t maxAcceleration = 2000; // Default acceleration
uint32_t maxDeceleration = 2000; // Default deceleration
int16_t homingSpeed = 50;        // Speed for homing operations (RPM)
int8_t stallSensitivity = STALLSENSITIVITY; // Sensitivity for stall detection

// Create a MovingMotor object
// Parameters: startPos, startDistance, startSpeed, endPos, maxVelocity, maxAcceleration, maxBounces, 
//             upVelMult, upAccMult, downVelMult, downAccMult
MovingMotor motor(
  HOME_POSITION,  // startPos - using defined HOME_POSITION constant
  5,     // startDistance
  0.5,   // startSpeed
  800,   // endPos (do not go beyond 2080 this is the end of the beam)
  3000,  // maxVelocity - base velocity for cymbal striking
  8000,  // maxAcceleration - high acceleration for responsive cymbal hitting
  5,     // maxBounces - number of times to strike the cymbal
  2.0,   // upVelocityMultiplier - fast upward strike
  1.0,   // upAccelerationMultiplier - quick response when striking. Lower numbers faster
  2.5,   // downVelocityMultiplier - moderately fast return
  2.0    // downAccelerationMultiplier - normal acceleration for return
);

// Forward declaration
void printStatus();

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  delay(2000);  // Give time for serial to initialize
  
  Serial.println("MOTOR MOVEMENT TEST - uStepper S32");
  Serial.println("Will attempt to move motor to absolute position 0.55");
  
  // Initialize uStepper S32 with closed loop control
  stepper.setup(CLOSEDLOOP, STEPSPERREV, 1, 1, 1, 1, 0);
  delay(2000);  // Allow time for stepper to initialize
  
  // Check motor orientation - this is necessary for closed-loop operation
  stepper.checkOrientation(30.0);
  delay(1000);
  
  // Set very conservative parameters for testing
  stepper.setMaxVelocity(100);        // Very slow velocity for testing
  stepper.setMaxAcceleration(300);    // Low acceleration for testing
  
  // Initialize the motor with reference to the stepper
  motor.init(&stepper);
  
  // Print current position before any movement
  Serial.print("STARTING POSITION: ");
  Serial.println(stepper.encoder.getAngleMoved());
  delay(1000);
  
  // TEST 1: Try to move using closedLoop angleMove
  Serial.println("\nTEST 1: Attempting to move using moveToAngle(0.55)...");
  stepper.enableClosedLoop();
  stepper.moveToAngle(HOME_POSITION);
  
  // Wait for movement to complete
  unsigned long startTime = millis();
  while(stepper.getMotorState() && (millis() - startTime < 5000)) {
    delay(500);
    Serial.print("Current position: ");
    Serial.println(stepper.encoder.getAngleMoved());
  }
  
  // Print result of first test
  Serial.print("POSITION AFTER TEST 1: ");
  Serial.println(stepper.encoder.getAngleMoved());
  delay(2000);
  
  // TEST 2: Try raw step movement
  Serial.println("\nTEST 2: Attempting raw step movement (-200 steps)...");
  stepper.disableClosedLoop();
  stepper.moveSteps(-200);  // Move 200 steps in negative direction
  delay(3000);  // Give time for movement
  
  // Print result of second test
  Serial.print("POSITION AFTER TEST 2: ");
  Serial.println(stepper.encoder.getAngleMoved());
  
  // Set this position as our start position for the rest of the program
  stepper.enableClosedLoop();
  motor.setStartPosition();
  
  // Now proceed with normal setup
  setupComplete = true;
  
  Serial.println("\nTesting complete. Motor should have attempted to move left twice.");
  Serial.println("Available commands:");
  Serial.println("  'a' - Activate motor movement (cymbal striking sequence)");
  Serial.println("  's' - Stop motor movement");
  Serial.println("  'r' - Reset motor to start position");
  Serial.println("  'h' - Set current position as start position");
  Serial.println("  'e' - Set current position as end position");
  Serial.println("  '1' - Go to start position");
  Serial.println("  '2' - Go to end position");
  Serial.println("  'm' - Move manually (enter angle after m, e.g., 'm90')");
  Serial.print(">>> ");
}

void loop() {
  // Only proceed if setup is complete
  if (!setupComplete) return;
  
  // Check for commands
  if (Serial.available()) {
    // Read the command
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command.length() > 0) {
      processCommand(command);
      Serial.print(">>> ");
    }
  }
  
  // Update motor position at every iteration
  motor.update(motor.isComplete() ? false : true);
  
  // Add a small delay to prevent overwhelming the serial port
  delay(50);
}

void processCommand(String command) {
  char cmd = command.charAt(0);
  String value = "";
  
  if (command.length() > 1) {
    value = command.substring(1);
  }
  
  float numValue = value.toFloat();
  
  switch (cmd) {
    case 'a':  // Activate motor movement
      Serial.println("Activating motor movement between start and end positions");
      motor.reset();
      motor.update(true);
      break;
      
    case 's':  // Stop motor
      Serial.println("Stopping motor movement");
      stepper.stop();
      motor.update(false);
      break;
      
    case 'r':  // Reset motor
      Serial.println("Resetting motor to start position");
      motor.reset();
      break;
      
    case 'h':  // Set start position
      Serial.println("Setting current position as start position");
      motor.setStartPosition();
      break;
      
    case 'e':  // Set end position
      Serial.println("Setting current position as end position");
      motor.setEndPosition();
      break;
      
    case '1':  // Go to start position
      Serial.println("Going to start position");
      motor.goToStart();
      break;
      
    case '2':  // Go to end position
      Serial.println("Going to end position");
      motor.goToEnd();
      break;
      
    case 'm':  // Move to specified angle
      if (value.length() > 0) {
        Serial.print("Moving to angle: ");
        Serial.println(numValue);
        stepper.enableClosedLoop();
        stepper.moveToAngle(numValue);
        
        // Wait for move to complete
        while(stepper.getMotorState()) {
          delay(10);
        }
      } else {
        Serial.println("Please provide an angle after 'm'");
      }
      break;
      
    case 'v':  // Set max velocity
      if (value.length() > 0) {
        uint32_t velocity = (uint32_t)numValue;
        Serial.print("Setting max velocity to: ");
        Serial.println(velocity);
        motor.setMaxVelocity(velocity);
      } else {
        Serial.println("Please provide a velocity value after 'v'");
      }
      break;
      
    case 'x':  // Set max acceleration
      if (value.length() > 0) {
        uint32_t accel = (uint32_t)numValue;
        Serial.print("Setting max acceleration to: ");
        Serial.println(accel);
        motor.setMaxAcceleration(accel);
      } else {
        Serial.println("Please provide an acceleration value after 'x'");
      }
      break;
      
    case 'b':  // Set max bounce cycles
      if (value.length() > 0) {
        int cycles = (int)numValue;
        Serial.print("Setting max bounce cycles to: ");
        Serial.println(cycles);
        motor.setMaxCycles(cycles);
      } else {
        Serial.println("Please provide a number of cycles after 'b'");
      }
      break;
      
    case 'u':  // Set up velocity multiplier
      if (value.length() > 0) {
        float mult = numValue;
        Serial.print("Setting up velocity multiplier to: ");
        Serial.println(mult);
        motor.setUpVelocityMultiplier(mult);
      } else {
        Serial.println("Please provide a multiplier value after 'u'");
      }
      break;
      
    case 'j':  // Set up acceleration multiplier
      if (value.length() > 0) {
        float mult = numValue;
        Serial.print("Setting up acceleration multiplier to: ");
        Serial.println(mult);
        motor.setUpAccelerationMultiplier(mult);
      } else {
        Serial.println("Please provide a multiplier value after 'j'");
      }
      break;
      
    case 'd':  // Set down velocity multiplier
      if (value.length() > 0) {
        float mult = numValue;
        Serial.print("Setting down velocity multiplier to: ");
        Serial.println(mult);
        motor.setDownVelocityMultiplier(mult);
      } else {
        Serial.println("Please provide a multiplier value after 'd'");
      }
      break;
      
    case 'k':  // Set down acceleration multiplier
      if (value.length() > 0) {
        float mult = numValue;
        Serial.print("Setting down acceleration multiplier to: ");
        Serial.println(mult);
        motor.setDownAccelerationMultiplier(mult);
      } else {
        Serial.println("Please provide a multiplier value after 'k'");
      }
      break;
      
    case 'i':  // Info about motor state
      printStatus();
      break;
      
    case 'c':  // Toggle continuous status reporting
      // For this example, we're just acknowledging the command
      Serial.println("Continuous status reporting function would be toggled here");
      break;
      
    default:
      Serial.print("Unknown command: ");
      Serial.println(command);
      break;
  }
}

void printStatus() {
  Serial.println("--- Motor Status ---");
  Serial.print("Current Absolute Angle: ");
  Serial.println(motor.getCurrentAngle());
  
  Serial.print("Position: ");
  Serial.println(motor.getPosition());
  
  Serial.print("Start Position Angle: ");
  Serial.println(motor.getStartAngle());
  
  Serial.print("End Position Angle: ");
  Serial.println(motor.getEndAngle());
  
  Serial.print("Max Velocity: ");
  Serial.println(motor.getMaxVelocity());
  
  Serial.print("Max Acceleration: ");
  Serial.println(motor.getMaxAcceleration());
  
  Serial.print("Current Strike: ");
  Serial.print(motor.getCycles());
  Serial.print(" of ");
  Serial.println(motor.getMaxCycles());
  
  Serial.print("Movement complete: ");
  Serial.println(motor.isComplete() ? "Yes" : "No");
  
  Serial.print("Velocity Multipliers - Up: ");
  Serial.print(motor.getUpVelocityMultiplier());
  Serial.print(", Down: ");
  Serial.println(motor.getDownVelocityMultiplier());
  
  Serial.print("Acceleration Multipliers - Up: ");
  Serial.print(motor.getUpAccelerationMultiplier());
  Serial.print(", Down: ");
  Serial.println(motor.getDownAccelerationMultiplier());
  
  Serial.println("-------------------");
}