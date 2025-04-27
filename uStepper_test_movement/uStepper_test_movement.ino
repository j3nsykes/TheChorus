/********************************************************************************************
*         File:  uStepper_Control_Modified.ino                                             *
*      Version:  1.2.0                                                                      *
*         Date:  April 25th, 2025                                                           *
*  Description:  Comprehensive control example for uStepper S32                             *
*                This sketch demonstrates control of:                                       *
*                - Speed (RPM)                                                              *
*                - Position (angle)                                                         *
*                - Velocity (max speed during movements)                                    *
*                - Acceleration                                                             *
*                - Home position setting                                                    *
*                - End position setting                                                     *
*                - Direction control                                                        *
*                                                                                           *
*                The sketch responds to serial commands:                                    *
*                - 'P' followed by angle: Move to absolute position (degrees)              *
*                - 'R' followed by angle: Move relative angle (degrees)                    *
*                - 'S' followed by speed: Set continuous speed (RPM)                       *
*                - 'V' followed by value: Set maximum velocity                             *
*                - 'A' followed by value: Set maximum acceleration                         *
*                - 'T' followed by value: Set stall sensitivity (-64 to 63)                *
*                - 'Z': Set current position as home (zero)                                *
*                - 'E': Set current position as end                                        *
*                - 'G': Go home (return to zero position)                                  *
*                - 'F': Go to end position                                                 *
*                - 'I': Invert direction                                                   *
*                - 'C': Toggle closed loop control                                         *
*                - 'X': Stop the motor                                                     *
*                - 'H': Help menu                                                          *
*                - 'Q': Print status                                                       *
*                                                                                           *
* For more information, check out the documentation:                                        *
*    http://ustepper.com/docs/usteppers32/html/index.html                                  *
*                                                                                           *
********************************************************************************************/

#include <UstepperS32.h>

// Constants for steps calculation
#define STEPSPERREV 200  // Number of steps per revolution (1.8° motor)
#define RES (STEPSPERREV * 256)/360.0  // Steps per degree
#define STALLSENSITIVITY 2  // Default stall sensitivity

UstepperS32 stepper;

// Default parameters
float currentPosition = 0.0;
float targetPosition = 0.0;
float currentSpeed = 0.0;
uint32_t maxVelocity = 500;      // Default max velocity
uint32_t maxAcceleration = 2000; // Default acceleration
uint32_t maxDeceleration = 2000; // Default deceleration
bool isClosedLoop = true;        // Use closed loop control by default

// Direction and position parameters
bool directionInverted = false;  // Direction control flag
float homeOffset = 0.0;          // Offset for home position
float endOffset = 0.0;           // Offset for end position
int16_t homingSpeed = 50;        // Speed for homing operations (RPM)
int8_t stallSensitivity = STALLSENSITIVITY;  // Sensitivity for stall detection

// For serial communication
String inputString = "";

void setup() {
  // Initialise serial communication
  Serial.begin(9600);
  while (!Serial) {
    ; // Wait for serial port to connect
  }
  
  Serial.println();
  Serial.println("uStepper S32 Control System");
  Serial.println("Version 1.2.0");
  Serial.println("Send 'b' to begin...");
  
  // Wait for begin signal
  while (Serial.read() != 'b') {
    ; // Wait for 'b' character
  }
  
  // Initialise uStepper S32 with closed loop control
  stepper.setup(CLOSEDLOOP, STEPSPERREV, 1, 1, 1, 1, 0);  // Setup with closed loop, STEPSPERREV steps per revolution
  
  // Check motor orientation
  stepper.checkOrientation(30.0);  // Check orientation with +/- 30 microsteps movement
  
  // Set initial parameters
  stepper.setMaxVelocity(maxVelocity);
  stepper.setMaxAcceleration(maxAcceleration);
  
  // Initialise home position to current position
  homeOffset = stepper.angleMoved();
  
  // Initialise end position to same as home (will be set later)
  endOffset = homeOffset;
  
  // Print welcome message and help
  Serial.println("Ready to accept commands");
  Serial.println("Type 'H' for available commands");
  Serial.println();
  printHelp();
  Serial.print(">>> ");  // Command prompt
}

void loop() {
  // Process any pending serial commands
  if (Serial.available()) {
    // Read command (until tab, newline, or carriage return)
    String command = Serial.readStringUntil(0x09); // First try to read until tab
    
    if (command.length() == 0) {
      // If nothing was read (no tab), try newline or carriage return
      command = Serial.readStringUntil('\n');
      if (command.length() == 0) {
        command = Serial.readStringUntil('\r');
      }
    }
    
    if (command.length() > 0) {
      processCommand(command);
      Serial.println();
      Serial.print(">>> ");  // Command prompt
    }
  }
}

void processCommand(String command) {
  command.trim();  // Remove any whitespace
  
  if (command.length() < 1) {
    return;  // Empty command, do nothing
  }
  
  // Check if it's a numeric command for steps
  int32_t steps = command.toInt();
  if (steps != 0) {
    Serial.print("Moving ");
    Serial.print(steps);
    Serial.println(" steps...");
    
    // Apply direction inversion if needed
    if (directionInverted) {
      steps = -steps;
    }
    
    // Enable closed loop for better positioning
    if (isClosedLoop) {
      stepper.enableClosedLoop();
    }
    
    stepper.moveSteps(steps);
    
    // Wait for move to complete
    while(stepper.getMotorState()) {
      delay(10);  // Small delay to prevent CPU hogging
    }
    
    Serial.println("Move completed.");
    return;
  }
  
  // If not a numeric command, process as a letter command
  char cmd = command.charAt(0);
  String value = "";
  
  if (command.length() > 1) {
    value = command.substring(1);
  }
  
  float numValue = value.toFloat();
  
  // Check for homing commands
  if (command == "hr") {
    Serial.println("Homing clockwise...");
    stepper.moveToEnd(CW, homingSpeed, stallSensitivity);
    Serial.println("Homing completed.");
    return;
  } 
  else if (command == "hl") {
    Serial.println("Homing counterclockwise...");
    stepper.moveToEnd(CCW, homingSpeed, stallSensitivity);
    Serial.println("Homing completed.");
    return;
  }
  
  // Process other single-letter commands
  switch (cmd) {
    case 'P':  // Move to absolute position (angle)
    case 'p':
      Serial.print("Moving to absolute position: ");
      Serial.println(numValue);
      
      // Enable closed loop for better positioning
      if (isClosedLoop) {
        stepper.enableClosedLoop();
      }
      
      // Apply home offset for absolute positioning
      stepper.moveToAngle(numValue + homeOffset);
      targetPosition = numValue;
      
      // Wait for move to complete
      while(stepper.getMotorState()) {
        delay(10);  // Small delay to prevent CPU hogging
      }
      break;
      
    case 'R':  // Move relative angle
    case 'r':
      Serial.print("Moving relative angle: ");
      Serial.println(numValue);
      
      // Enable closed loop for better positioning
      if (isClosedLoop) {
        stepper.enableClosedLoop();
      }
      
      // For relative moves, adjust direction if inverted
      if (directionInverted) {
        numValue = -numValue;
        Serial.println("Direction inverted, moving: " + String(numValue));
      }
      
      stepper.moveAngle(numValue);
      targetPosition = currentPosition + numValue;
      
      // Wait for move to complete
      while(stepper.getMotorState()) {
        delay(10);  // Small delay to prevent CPU hogging
      }
      break;
      
    case 'S':  // Set continuous speed (RPM)
    case 's':
      Serial.print("Setting speed to: ");
      // Apply direction inversion if enabled
      if (directionInverted) {
        numValue = -numValue;
        Serial.print("Direction inverted, setting speed to: ");
      }
      Serial.println(numValue);
      stepper.setRPM(numValue);
      currentSpeed = numValue;
      break;
      
    case 'V':  // Set maximum velocity
    case 'v':
      if (numValue > 0) {
        maxVelocity = (uint32_t)numValue;
        stepper.setMaxVelocity(maxVelocity);
        Serial.print("Maximum velocity set to: ");
        Serial.println(maxVelocity);
      }
      break;
      
    case 'A':  // Set maximum acceleration
    case 'a':
      if (numValue > 0) {
        maxAcceleration = (uint32_t)numValue;
        stepper.setMaxAcceleration(maxAcceleration);
        Serial.print("Maximum acceleration set to: ");
        Serial.println(maxAcceleration);
      }
      break;
      
    case 'T':  // Set stall sensitivity
    case 't':
      if (numValue >= -64 && numValue <= 63) {
        stallSensitivity = (int8_t)numValue;
        Serial.print("Stall sensitivity set to: ");
        Serial.println(stallSensitivity);
      } else {
        Serial.println("Stall sensitivity must be between -64 and 63");
      }
      break;
      
    case 'Z':  // Set current position as home (zero)
    case 'z':
      // Enable closed loop for better positioning
      if (isClosedLoop) {
        stepper.enableClosedLoop();
      }
      
      homeOffset = stepper.angleMoved();
      Serial.println("Current position set as home (zero)");
      currentPosition = 0.0;
      targetPosition = 0.0;
      break;
      
    case 'E':  // Set current position as end position
    case 'e':
      // Enable closed loop for better positioning
      if (isClosedLoop) {
        stepper.enableClosedLoop();
      }
      
      endOffset = stepper.angleMoved();
      Serial.print("Current position set as end position (angle: ");
      Serial.print(endOffset);
      Serial.println(")");
      break;
      
    case 'G':  // Go home (return to zero position)
    case 'g':
      Serial.println("Moving to home position");
      
      // Enable closed loop for better positioning
      if (isClosedLoop) {
        stepper.enableClosedLoop();
      }
      
      stepper.moveToAngle(homeOffset);
      targetPosition = 0;
      
      // Wait for move to complete
      while(stepper.getMotorState()) {
        delay(10);  // Small delay to prevent CPU hogging
      }
      break;
      
    case 'F':  // Go to end position
    case 'f':
      Serial.print("Moving to end position (angle: ");
      Serial.print(endOffset);
      Serial.println(")");
      
      // Enable closed loop for better positioning
      if (isClosedLoop) {
        stepper.enableClosedLoop();
      }
      
      stepper.moveToAngle(endOffset);
      
      // Wait for move to complete
      while(stepper.getMotorState()) {
        delay(10);  // Small delay to prevent CPU hogging
      }
      break;
      
    case 'I':  // Invert direction
    case 'i':
      directionInverted = !directionInverted;
      Serial.print("Direction inverted: ");
      Serial.println(directionInverted ? "Yes" : "No");
      break;
      
    case 'C':  // Toggle closed loop control
    case 'c':
      isClosedLoop = !isClosedLoop;
      if (isClosedLoop) {
        stepper.enableClosedLoop();
        Serial.println("Closed loop control enabled");
      } else {
        stepper.disableClosedLoop();
        Serial.println("Closed loop control disabled");
      }
      break;
      
    case 'X':  // Stop the motor
    case 'x':
      stepper.stop();
      Serial.println("Motor stopped");
      break;
      
    case 'Q':  // Print status
    case 'q':
      reportStatus();
      break;
      
    case 'H':  // Help menu
    case 'h':
      printHelp();
      break;
      
    default:
      Serial.println("Unknown command. Type 'H' for help.");
      break;
  }
}

void reportStatus() {
  // Update current position (subtract home offset for user display)
  currentPosition = stepper.angleMoved() - homeOffset;
  
  // Get current speed
  currentSpeed = stepper.getDriverRPM();
  
  // Print status
  Serial.println("--- Status ---");
  Serial.print("Position: ");
  Serial.print(currentPosition);
  Serial.println(" degrees");
  
  Serial.print("Current Speed: ");
  Serial.print(currentSpeed);
  Serial.println(" RPM");
  
  Serial.print("Max Velocity: ");
  Serial.println(maxVelocity);
  
  Serial.print("Max Acceleration: ");
  Serial.println(maxAcceleration);
  
  Serial.print("Closed Loop: ");
  Serial.println(isClosedLoop ? "Enabled" : "Disabled");
  
  Serial.print("Direction Inverted: ");
  Serial.println(directionInverted ? "Yes" : "No");
  
  Serial.print("Home Position: ");
  Serial.print(homeOffset);
  Serial.println(" degrees (absolute)");
  
  Serial.print("End Position: ");
  Serial.print(endOffset);
  Serial.println(" degrees (absolute)");
  
  Serial.print("Stall Sensitivity: ");
  Serial.println(stallSensitivity);
  
  Serial.println("-------------");
}

void printHelp() {
  Serial.println("==== uStepper S32 Control ====");
  Serial.println("Available commands:");
  Serial.println("  <steps>    - Move specific number of steps (can be negative)");
  Serial.println("  hr         - Home clockwise (uses stall detection)");
  Serial.println("  hl         - Home counterclockwise (uses stall detection)");
  Serial.println("  P<angle>   - Move to absolute position in degrees");
  Serial.println("  R<angle>   - Move relative angle in degrees");
  Serial.println("  S<speed>   - Set continuous speed in RPM");
  Serial.println("  V<value>   - Set maximum velocity");
  Serial.println("  A<value>   - Set maximum acceleration");
  Serial.println("  T<value>   - Set stall sensitivity (-64 to 63)");
  Serial.println("  Z          - Set current position as home (zero)");
  Serial.println("  E          - Set current position as end position");
  Serial.println("  G          - Go home (return to zero position)");
  Serial.println("  F          - Go to end position");
  Serial.println("  I          - Invert direction");
  Serial.println("  C          - Toggle closed loop control");
  Serial.println("  X          - Stop the motor");
  Serial.println("  Q          - Print status");
  Serial.println("  H          - Show this help menu");
  Serial.println("Examples:");
  Serial.println("  500    (moves 500 steps)");
  Serial.println("  hr     (homes clockwise)");
  Serial.println("  P90    (moves to 90 degrees)");
  Serial.println("  S50    (sets speed to 50 RPM)");
  Serial.println("============================");
}