/********************************************************************************************
*         File:  MovingMotor.cpp                                                          *
*      Version:  4.0.0                                                                     *
*         Date:  April 25th, 2025                                                          *
*  Description:  Implementation file for MovingMotor class with absolute position control  *
*                                                                                          *
********************************************************************************************/

#include "MovingMotor.h"

// Constructor
MovingMotor::MovingMotor(int startPos, float startDistance, float startSpeed, int endPos, 
                          uint32_t maxVel, uint32_t maxAccel, int maxBounces,
                          float upVelMult, float upAccMult, float downVelMult, float downAccMult) {
  position = startPos;
  startAngle = startPos;   // Set initial startAngle from constructor parameter
  endAngle = endPos;       // Set endAngle directly from constructor parameter
  direction = 1;
  speed = startSpeed;
  distance = startDistance;
  cycles = 0;
  maxCycles = maxBounces;  // Initialize maxCycles from constructor
  cycleComplete = false;
  isActive = false;
  maxVelocity = maxVel;
  maxAcceleration = maxAccel;
  upVelocityMultiplier = upVelMult;
  upAccelMultiplier = upAccMult;
  downVelocityMultiplier = downVelMult;
  downAccelMultiplier = downAccMult;
  stepperPtr = NULL;      // Initialize stepper pointer to NULL
}

// Initialize with stepper reference
void MovingMotor::init(UstepperS32* stepper) {
  stepperPtr = stepper;
  
  // Set maximum velocity and acceleration
  stepperPtr->setMaxVelocity(maxVelocity);
  stepperPtr->setMaxAcceleration(maxAcceleration);
  
  // Make sure we're in closed loop mode for accurate positioning
  stepperPtr->enableClosedLoop();
  
  // Move to the start position - use the startAngle value from constructor
  Serial.print("Moving to start position (angle: ");
  Serial.print(startAngle);
  Serial.println(")");
  
  stepperPtr->moveToAngle(startAngle);
  
  // Wait for the move to complete
  while(stepperPtr->getMotorState()) {
    delay(10);
  }
  
  Serial.println("MovingMotor initialized with absolute position control");
  Serial.print("Start position angle: ");
  Serial.println(startAngle);
  Serial.print("End position angle: ");
  Serial.println(endAngle);
  Serial.print("Max bounce cycles: ");
  Serial.println(maxCycles);
  Serial.print("Max velocity: ");
  Serial.println(maxVelocity);
  Serial.print("Max acceleration: ");
  Serial.println(maxAcceleration);
  Serial.print("Up velocity multiplier: ");
  Serial.println(upVelocityMultiplier);
  Serial.print("Up acceleration multiplier: ");
  Serial.println(upAccelMultiplier);
  Serial.print("Down velocity multiplier: ");
  Serial.println(downVelocityMultiplier);
  Serial.print("Down acceleration multiplier: ");
  Serial.println(downAccelMultiplier);
}

// Get absolute encoder angle converted to position value
int MovingMotor::angleToPosition(float angle) {
  // This function converts an absolute angle to a position value
  // based on the conversion factor from steps per degree
  return (int)(angle * RES);
}

// Update motor position
void MovingMotor::update(boolean activate) {
  // Check if stepper is initialized
  if (stepperPtr == NULL) {
    Serial.println("Error: Stepper not initialized!");
    return;
  }
  
  isActive = activate;
  
  if (isActive && !cycleComplete) {
    // Get current absolute position from encoder
    float currentAngle = stepperPtr->angleMoved();
    
    // Check where we are in the movement cycle
    if (direction > 0) {
      // Moving toward end position (upward to hit cymbal)
      if (abs(currentAngle - endAngle) < 0.5) { // Within 0.5 degrees of end
        // We've reached the end position, reverse direction immediately
        direction = -1;
        Serial.println("Reached end position (hit cymbal), falling back to start");
        
        // Set down velocity for return using the multiplier
        stepperPtr->setMaxVelocity(maxVelocity * downVelocityMultiplier);
        stepperPtr->setMaxAcceleration(maxAcceleration * downAccelMultiplier);
        
        // Move to start position
        stepperPtr->enableClosedLoop();
        stepperPtr->moveToAngle(startAngle);
      }
    } else {
      // Moving toward start position (downward)
      if (abs(currentAngle - startAngle) < 0.5) { // Within 0.5 degrees of start
        // We've reached the start position
        if (cycles < maxCycles) { // Use configurable maxCycles
          // Reverse direction and continue bouncing
          direction = 1;
          cycles++;
          Serial.print("Strike ");
          Serial.print(cycles);
          Serial.print(" of ");
          Serial.print(maxCycles);
          Serial.println(": Reached start position, moving up to hit cymbal");
          
          // Set up velocity for cymbal strike using the multiplier
          stepperPtr->setMaxVelocity(maxVelocity * upVelocityMultiplier);
          stepperPtr->setMaxAcceleration(maxAcceleration * upAccelMultiplier);
          
          // Move to end position
          stepperPtr->enableClosedLoop();
          stepperPtr->moveToAngle(endAngle);
        } else {
          // Completed desired bounce cycles
          cycleComplete = true;
          Serial.println("Cymbal striking complete - returned to start position");
          stepperPtr->stop();
        }
      }
    }
    
    // If we're not at a target position yet and no movement is happening, start movement
    if (!cycleComplete && !stepperPtr->getMotorState()) {
      // No active motion, need to start movement
      float targetAngle = (direction > 0) ? endAngle : startAngle;
      
      // Set appropriate velocity based on direction using multipliers
      if (direction > 0) {
        // Moving up to hit cymbal
        stepperPtr->setMaxVelocity(maxVelocity * upVelocityMultiplier);
        stepperPtr->setMaxAcceleration(maxAcceleration * upAccelMultiplier);
        Serial.print("Moving UP to: ");
      } else {
        // Moving down
        stepperPtr->setMaxVelocity(maxVelocity * downVelocityMultiplier);
        stepperPtr->setMaxAcceleration(maxAcceleration * downAccelMultiplier);
        Serial.print("Moving DOWN to: ");
      }
      
      // Move toward target
      stepperPtr->enableClosedLoop();
      stepperPtr->moveToAngle(targetAngle);
      
      // Log current status
      Serial.print(targetAngle);
      Serial.print(" degrees, velocity: ");
      Serial.print(direction > 0 ? maxVelocity * upVelocityMultiplier : maxVelocity * downVelocityMultiplier);
      Serial.print(", acceleration: ");
      Serial.println(direction > 0 ? maxAcceleration * upAccelMultiplier : maxAcceleration * downAccelMultiplier);
    }
  } else if (!isActive && stepperPtr != NULL) {
    // Stop the motor when inactive
    stepperPtr->stop();
  }
}

// Send commands to the uStepper motor
void MovingMotor::outputToMotor() {
  if (stepperPtr == NULL) return;
  
  // This method is now mainly for reporting status
  // The actual movement is handled in update()
  
  // Get current position from the encoder
  float currentAngle = stepperPtr->angleMoved();
  
  // Log current status
  Serial.print("Current Angle: ");
  Serial.print(currentAngle);
  Serial.print(", Target: ");
  Serial.print((direction > 0) ? endAngle : startAngle);
  Serial.print(", Direction: ");
  Serial.println(direction > 0 ? "To End" : "To Start");
}

// Set start position to current absolute position
void MovingMotor::setStartPosition() {
  if (stepperPtr == NULL) return;
  
  // Enable closed loop for accurate position reading
  stepperPtr->enableClosedLoop();
  
  // Get the current absolute angle
  startAngle = stepperPtr->angleMoved();
  
  // Reset position to 0 since this is our new start point
  position = 0;
  
  Serial.print("Start position set to absolute angle: ");
  Serial.println(startAngle);
}

// Set end position to current absolute position
void MovingMotor::setEndPosition() {
  if (stepperPtr == NULL) return;
  
  // Enable closed loop for accurate position reading
  stepperPtr->enableClosedLoop();
  
  // Get the current absolute angle
  endAngle = stepperPtr->angleMoved();
  
  Serial.print("End position set to absolute angle: ");
  Serial.println(endAngle);
}

// Go to start position
void MovingMotor::goToStart() {
  if (stepperPtr == NULL) return;
  
  // Enable closed loop for accurate positioning
  stepperPtr->enableClosedLoop();
  
  Serial.print("Moving to start position (angle: ");
  Serial.print(startAngle);
  Serial.println(")");
  
  // Move to the start angle
  stepperPtr->moveToAngle(startAngle);
  
  // Reset our position to 0
  position = 0;
  
  // Wait for move to complete
  while(stepperPtr->getMotorState()) {
    delay(10);  // Small delay to prevent CPU hogging
  }
}

// Go to end position
void MovingMotor::goToEnd() {
  if (stepperPtr == NULL) return;
  
  // Enable closed loop for accurate positioning
  stepperPtr->enableClosedLoop();
  
  Serial.print("Moving to end position (angle: ");
  Serial.print(endAngle);
  Serial.println(")");
  
  // Move to the end angle
  stepperPtr->moveToAngle(endAngle);
  
  // Update our position
  position = angleToPosition(endAngle - startAngle);
  
  // Wait for move to complete
  while(stepperPtr->getMotorState()) {
    delay(10);  // Small delay to prevent CPU hogging
  }
}

// Reset the motor for another cycle
void MovingMotor::reset() {
  if (stepperPtr == NULL) return;
  
  // Reset cycle tracking
  direction = 1;  // Start by moving toward end position
  cycles = 0;
  cycleComplete = false;
  
  // Restore original velocities
  stepperPtr->setMaxVelocity(maxVelocity);
  stepperPtr->setMaxAcceleration(maxAcceleration);
  
  // Move motor back to start position
  goToStart();
  
  Serial.println("Motor reset to start position, ready for new bounce cycle");
}

// Set maximum velocity
void MovingMotor::setMaxVelocity(uint32_t velocity) {
  if (stepperPtr == NULL) return;
  
  maxVelocity = velocity;
  stepperPtr->setMaxVelocity(maxVelocity);
  
  Serial.print("Maximum velocity set to: ");
  Serial.println(maxVelocity);
}

// Set maximum acceleration
void MovingMotor::setMaxAcceleration(uint32_t acceleration) {
  if (stepperPtr == NULL) return;
  
  maxAcceleration = acceleration;
  stepperPtr->setMaxAcceleration(maxAcceleration);
  
  Serial.print("Maximum acceleration set to: ");
  Serial.println(maxAcceleration);
}

// Set maximum number of bounce cycles
void MovingMotor::setMaxCycles(int numCycles) {
  if (numCycles > 0) {
    maxCycles = numCycles;
    Serial.print("Maximum bounce cycles set to: ");
    Serial.println(maxCycles);
  }
}

// Set velocity and acceleration multipliers
void MovingMotor::setUpVelocityMultiplier(float multiplier) {
  if (multiplier > 0) {
    upVelocityMultiplier = multiplier;
    Serial.print("Up velocity multiplier set to: ");
    Serial.println(upVelocityMultiplier);
  }
}

void MovingMotor::setUpAccelerationMultiplier(float multiplier) {
  if (multiplier > 0) {
    upAccelMultiplier = multiplier;
    Serial.print("Up acceleration multiplier set to: ");
    Serial.println(upAccelMultiplier);
  }
}

void MovingMotor::setDownVelocityMultiplier(float multiplier) {
  if (multiplier > 0) {
    downVelocityMultiplier = multiplier;
    Serial.print("Down velocity multiplier set to: ");
    Serial.println(downVelocityMultiplier);
  }
}

void MovingMotor::setDownAccelerationMultiplier(float multiplier) {
  if (multiplier > 0) {
    downAccelMultiplier = multiplier;
    Serial.print("Down acceleration multiplier set to: ");
    Serial.println(downAccelMultiplier);
  }
}

// Getters for the motor state
int MovingMotor::getPosition() { 
  return position; 
}

bool MovingMotor::isComplete() { 
  return cycleComplete; 
}

int MovingMotor::getCycles() { 
  return cycles; 
}

int MovingMotor::getMaxCycles() {
  return maxCycles;
}

float MovingMotor::getStartAngle() {
  return startAngle;
}

float MovingMotor::getEndAngle() {
  return endAngle;
}

float MovingMotor::getCurrentAngle() {
  if (stepperPtr == NULL) return 0.0;
  return stepperPtr->angleMoved();
}

// Getters for velocity and acceleration
uint32_t MovingMotor::getMaxVelocity() {
  return maxVelocity;
}

uint32_t MovingMotor::getMaxAcceleration() {
  return maxAcceleration;
}

// Getters for multipliers
float MovingMotor::getUpVelocityMultiplier() {
  return upVelocityMultiplier;
}

float MovingMotor::getUpAccelerationMultiplier() {
  return upAccelMultiplier;
}

float MovingMotor::getDownVelocityMultiplier() {
  return downVelocityMultiplier;
}

float MovingMotor::getDownAccelerationMultiplier() {
  return downAccelMultiplier;
}