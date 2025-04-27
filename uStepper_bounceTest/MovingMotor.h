/********************************************************************************************
*         File:  MovingMotor.h                                                            *
*      Version:  2.0.0                                                                     *
*         Date:  April 25th, 2025                                                          *
*  Description:  Header file for MovingMotor class with absolute position control          *
*                                                                                          *
********************************************************************************************/

#ifndef MOVING_MOTOR_H
#define MOVING_MOTOR_H

#include <Arduino.h>
#include <UstepperS32.h>

// Define constants
#define STEPSPERREV 200    // 200 steps per revolution (1.8° motor)
#define RES (STEPSPERREV * 256)/360.0  // Steps per degree
#define STALLSENSITIVITY 2  // Default stall sensitivity

class MovingMotor {
  private:
    UstepperS32* stepperPtr;  // Pointer to the stepper motor object
    int position;             // Current logical position
    float startAngle;         // Absolute angle for start position
    float endAngle;           // Absolute angle for end position
    float direction;          // Direction of movement (1 for forward, -1 for backward)
    float speed;              // Speed factor (will be converted to RPM)
    float distance;           // Distance increment for each update
    int cycles;               // Number of completed cycles
    int maxCycles;            // Maximum number of bounce cycles to perform
    bool cycleComplete;       // Flag for completed cycle
    bool isActive;            // Whether motor is currently active
    uint32_t maxVelocity;     // Maximum velocity
    uint32_t maxAcceleration; // Maximum acceleration
    float upVelocityMultiplier;  // Multiplier for upward velocity
    float upAccelMultiplier;     // Multiplier for upward acceleration
    float downVelocityMultiplier;// Multiplier for downward velocity
    float downAccelMultiplier;   // Multiplier for downward acceleration
    
  public:
    // Constructor
    MovingMotor(int startPos, float startDistance, float startSpeed, int endPos, 
                uint32_t maxVel = 500, uint32_t maxAccel = 2000, int maxBounces = 10,
                float upVelMult = 3.0, float upAccMult = 2.0, 
                float downVelMult = 1.5, float downAccMult = 1.0);
    
    // Initialize with stepper reference
    void init(UstepperS32* stepper);
    
    // Update motor position
    void update(boolean activate);
    
    // Send commands to the uStepper motor
    void outputToMotor();
    
    // Set start position to current absolute position
    void setStartPosition();
    
    // Set end position to current absolute position
    void setEndPosition();
    
    // Go to start position
    void goToStart();
    
    // Go to end position
    void goToEnd();
    
    // Reset the motor for another cycle
    void reset();
    
    // Set maximum velocity
    void setMaxVelocity(uint32_t velocity);
    
    // Set maximum acceleration
    void setMaxAcceleration(uint32_t acceleration);
    
    // Set maximum number of bounce cycles
    void setMaxCycles(int numCycles);
    
    // Set velocity and acceleration multipliers
    void setUpVelocityMultiplier(float multiplier);
    void setUpAccelerationMultiplier(float multiplier);
    void setDownVelocityMultiplier(float multiplier);
    void setDownAccelerationMultiplier(float multiplier);
    
    // Getters for the motor state
    int getPosition();
    bool isComplete();
    int getCycles();
    int getMaxCycles();
    float getStartAngle();
    float getEndAngle();
    float getCurrentAngle();
    uint32_t getMaxVelocity();
    uint32_t getMaxAcceleration();
    float getUpVelocityMultiplier();
    float getUpAccelerationMultiplier();
    float getDownVelocityMultiplier();
    float getDownAccelerationMultiplier();
    
    // Get absolute encoder angle converted to position value
    int angleToPosition(float angle);
};

#endif // MOVING_MOTOR_H