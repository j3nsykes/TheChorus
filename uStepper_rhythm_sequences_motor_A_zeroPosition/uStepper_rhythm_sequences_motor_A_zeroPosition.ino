#include <UstepperS32.h>
#include <EEPROM.h>
#include "MovingMotor.h"

// Define constants
#define STEPSPERREV 200                  // 200 steps per revolution (1.8° motor)
#define RES (STEPSPERREV * 256) / 360.0  // Steps per degree
#define STALLSENSITIVITY 2               // Default stall sensitivity
#define BEAT_TERMINATOR 255              // Value to indicate end of a beat pattern
#define PATTERN_DISABLED 255             // Special value to indicate pattern is disabled

// EEPROM addresses
#define EEPROM_INITIALISED_ADDR 0        // Address to store initialisation flag (1 byte)
#define EEPROM_ENCODER_ADDR 4            // Address to store encoder angle (4 bytes)
#define EEPROM_MAGIC_NUMBER 0xAB         // Magic number to verify EEPROM is initialised

// Initialisation parameters
#define INIT_TIMEOUT 60000               // Timeout for initialisation mode (60 seconds)
#define POSITION_TOLERANCE 2.0           // Angle tolerance for position reset (degrees)

// Global variables
UstepperS32 stepper;
bool setupComplete = false;

// Mode selection
bool manualRun = false;  // Manual mode vs. timer mode
bool timerOn = true;     // Whether timer-based playback is active
bool initMode = false;   // Initialisation mode flag
bool needsRestart = false; // Flag to indicate a restart is required

// Create a MovingMotor object for the cymbal striker
MovingMotor motor(
  0.55,  // startPos - home position
  5,     // startDistance
  0.5,   // startSpeed
  800,   // endPos (maximum position)
  3000,  // maxVelocity
  8000,  // maxAcceleration
  1,     // maxBounces - single strike
  2.0,   // upVelocityMultiplier
  1.0,   // upAccelerationMultiplier
  2.5,   // downVelocityMultiplier
  2.0    // downAccelerationMultiplier
);

// Motor and timing parameters
int bpm = 120;                         // Base tempo in beats per minute
unsigned long beatInterval;            // Milliseconds per beat (calculated from BPM)
unsigned long prevBeatTime;            // Time of last beat
int beatCount = 0;                     // Current beat position
int activePattern = PATTERN_DISABLED;  // Which pattern is currently active

// Motor activation flags
bool motorActive = false;  // Whether the motor is currently moving
bool zeroPositionInitialised = false;  // Flag to indicate if zero position is initialised

// Beat patterns - each pattern is a list of beat numbers (0-59) where the motor should trigger
// BEAT_TERMINATOR (255) marks the end of each pattern
const byte patterns[][16] = {
  // Pattern 0: Every 15 beats. All at once. Same across both motors.
  { 0, 15, 30, 45, BEAT_TERMINATOR },

  // Pattern 1: Up Down Up Down
  { 0, 15, 32, 49, BEAT_TERMINATOR },

  // Pattern 2: Up Down Up Down #2
  { 7, 23, 41, 59, BEAT_TERMINATOR },

  // Pattern 3: W rhythm
  { 0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, BEAT_TERMINATOR },

  // Pattern 4: Syncopate
  { 2, 6, 10, 14, 18, 22, 26, 30, 34, 38, 42, 46, 50, 54, 58, BEAT_TERMINATOR },

  // Pattern 5: Random occasional
  { 4, 18, 26, 43, BEAT_TERMINATOR },

  // Pattern 6: Random occasional #2
  { 9, 25, 50, BEAT_TERMINATOR },

  // Pattern 7: Random - beats 4 & 20
  { 4, 20, BEAT_TERMINATOR },

  // Pattern 8: Random - beats 22 & 45
  { 22, 45, BEAT_TERMINATOR },

  // Pattern 9: Off-beat syncopate
  { 0, 12, 16, 24, 28, 32, 40, 44, 48, 56, BEAT_TERMINATOR },

  // Pattern 10: Off-beat syncopate #2
  { 9, 24, 36, 48, 52, BEAT_TERMINATOR },

  // Pattern 11: Single beat - just beat 0
  { 0, BEAT_TERMINATOR },

  // Pattern 12: Single beat - just beat 22
  { 22, BEAT_TERMINATOR },

  // Pattern 13: Single beat - just beat 4
  { 4, BEAT_TERMINATOR },

  // Pattern 14: Single beat - just beat 45
  { 45, BEAT_TERMINATOR }
};

// Timer data structure - Format: {start_min, start_sec, end_min, end_sec}
const int timerData[][4] = {
  { 0, 0, 0, 40 },     // Silence
  { 0, 40, 8, 55 },    // Pattern 0 all together
  { 8, 55, 11, 30 },   // Silence
  { 11, 30, 12, 30 },  // Pattern 1 up down
  { 12, 30, 13, 37 },  // Pattern 3 W rhythm
  { 13, 37, 18, 17 },  // Silence
  { 18, 17, 18, 30 },  // Pattern 5 random
  { 18, 30, 21, 30 },  // Silence
  { 21, 30, 22, 3 },   // Pattern 9 syncopate
  { 22, 3, 23, 3 },    // Silence
  { 23, 3, 24, 3 },    // Pattern 11 single beat
  { 24, 3, 25, 20 },   // Silence
  { 25, 20, 25, 34 },  // Pattern 5 random
  { 25, 34, 26, 24 },  // Silence
  { 26, 24, 26, 37 },  // Pattern 13 single beat
  { 26, 37, 30, 0 }    // Silence
};

// Pattern index for each timer slot - which pattern plays during each timer period
const byte timerPatterns[] = {
  PATTERN_DISABLED,  // Silence
  0,                 // Pattern 0
  PATTERN_DISABLED,  // Silence
  1,                 // Pattern 1
  3,                 // Pattern 3
  PATTERN_DISABLED,  // Silence
  5,                 // Pattern 5
  PATTERN_DISABLED,  // Silence
  9,                 // Pattern 9
  PATTERN_DISABLED,  // Silence
  11,                // Pattern 11
  PATTERN_DISABLED,  // Silence
  5,                 // Pattern 5
  PATTERN_DISABLED,  // Silence
  13,                // Pattern 13
  PATTERN_DISABLED   // Silence
};

// Forward declarations
void printHelp();
void triggerMotor();
void updateTiming();
void checkTimer();
unsigned long getTimeInSeconds();
void initialiseZeroPosition();
void restoreZeroPosition();
void saveEncoderPosition(float angle);
float loadEncoderPosition();
bool isEEPROMInitialised();
void setEEPROMInitialised(bool initialised);
void printInitHelp();

void setup() {
  // Initialise serial communication
  Serial.begin(9600);
  Serial.println("Beat Sequencer v2.2 with Zero Position Initialisation");
  
  // Initialise uStepper with closed loop control
  // Note: The last parameter (setHome) is set to 0 because we'll establish our own home position
  stepper.setup(CLOSEDLOOP, STEPSPERREV, 1, 1, 1, 1, 0);
  stepper.checkOrientation(30.0);

  // Initialise motor
  motor.init(&stepper);

  // Check if zero position has been initialised
  if (!isEEPROMInitialised()) {
    // Enter initialisation mode
    Serial.println("Zero position not initialised!");
    Serial.println("Entering INITIALISATION mode");
    initMode = true;
    initialiseZeroPosition();
  } else {
    // Restore position from EEPROM
    Serial.println("Restoring zero position from EEPROM...");
    restoreZeroPosition();
    
    // Continue with normal startup
    Serial.println("Zero position restored successfully.");
    Serial.println("Send 'b' to begin or wait 5 seconds for auto-start...");

    // Wait for begin signal with timeout
    unsigned long startWaitTime = millis();
    bool manualStart = false;

    while (millis() - startWaitTime < 5000) {  // 5 second timeout
      if (Serial.available() && Serial.read() == 'b') {
        manualStart = true;
        break;
      }
      delay(10);  // Small delay to prevent CPU hogging
    }

    // Set base BPM and calculate timing
    bpm = 120;  // Explicitly set to 120 BPM
    updateTiming();

    // Initialise timing variables
    prevBeatTime = millis();

    // Set mode based on auto-start or manual start
    if (manualStart) {
      // If user pressed 'b', start in manual mode
      manualRun = true;
      timerOn = false;
      Serial.println("Manual start detected - starting in MANUAL mode");

      // Start with no active pattern
      activePattern = PATTERN_DISABLED;
    } else {
      // Auto-start in timer mode
      manualRun = false;
      timerOn = true;
      Serial.println("Auto-starting in TIMER mode...");

      // Immediately check timer to set the right pattern
      checkTimer();
    }

    // Print actual timing information
    Serial.print("Starting at ");
    Serial.print(bpm);
    Serial.print(" BPM with beat interval of ");
    Serial.print(beatInterval);
    Serial.println("ms");

    Serial.print("Mode: ");
    Serial.println(manualRun ? "Manual" : "Timer");
    Serial.print("Active Pattern: ");
    Serial.println(activePattern == PATTERN_DISABLED ? "None" : String(activePattern));

    // Setup complete
    setupComplete = true;

    // Print help
    printHelp();
  }
}

void loop() {
  // Check if restart is needed (this replaces the software reset)
  if (needsRestart) {
    // Just start over without actually resetting the hardware
    needsRestart = false;
    
    // Re-run setup logic
    setup();
    return;
  }
  
  // Handle initialisation mode
  if (initMode) {
    // Check for serial commands during initialisation
    if (Serial.available()) {
      char cmd = Serial.read();
      
      if (cmd == 's') {
        // Save current position as zero
        float currentAngle = stepper.encoder.getAngle();
        saveEncoderPosition(currentAngle);
        setEEPROMInitialised(true);
        
        Serial.print("Zero position saved: ");
        Serial.print(currentAngle);
        Serial.println(" degrees");
        
        // Exit initialisation mode
        initMode = false;
        
        // Signal that we need to restart
        Serial.println("Position saved. Returning to normal operation...");
        delay(1000);
        needsRestart = true;
      } else if (cmd == '+') {
        // Move clockwise a small amount
        stepper.moveAngle(1.0);
        delay(50);
        Serial.print("Position: ");
        Serial.println(stepper.encoder.getAngle());
      } else if (cmd == '-') {
        // Move counter-clockwise a small amount
        stepper.moveAngle(-1.0);
        delay(50);
        Serial.print("Position: ");
        Serial.println(stepper.encoder.getAngle());
      } else if (cmd == '5') {
        // Move clockwise 5 degrees
        stepper.moveAngle(5.0);
        delay(50);
        Serial.print("Position: ");
        Serial.println(stepper.encoder.getAngle());
      } else if (cmd == '%') {
        // Move counter-clockwise 5 degrees
        stepper.moveAngle(-5.0);
        delay(50);
        Serial.print("Position: ");
        Serial.println(stepper.encoder.getAngle());
      } else if (cmd == 'h') {
        // Print initialisation help
        printInitHelp();
      }
    }
    return;  // Don't proceed with normal loop during initialisation
  }

  // Only proceed if setup is complete
  if (!setupComplete) return;

  // Check for serial commands
  if (Serial.available()) {
    processCommand();
  }

  // If in timer mode, check if we need to change patterns
  if (timerOn && !manualRun) {
    checkTimer();
  }

  // Handle beat timing
  unsigned long currentTime = millis();
  if (currentTime - prevBeatTime >= beatInterval) {
    // Time for a new beat
    prevBeatTime = currentTime;  // Updated to prevent drift

    // Increment beat counter (0-59 then back to 0)
    beatCount = (beatCount + 1) % 60;

    // Visual indication every 5 beats
    if (beatCount % 5 == 0) {
      Serial.print("Beat: ");
      Serial.print(beatCount);
      Serial.print(" (");
      Serial.print(currentTime / 1000.0);
      Serial.println("s)");
    }

    // Check if this beat is in the active pattern
    if (activePattern != PATTERN_DISABLED && isInPattern(beatCount, activePattern)) {
      // Trigger the motor if it's not already active
      if (!motorActive) {
        Serial.print("Beat ");
        Serial.print(beatCount);
        Serial.println(" - triggering motor!");
        triggerMotor();
      }
    }
  }

  // Update motor state
  motor.update(motorActive);

  // Check if motor movement is complete
  if (motorActive && motor.isComplete()) {
    motorActive = false;
    Serial.println("Motor movement complete");
  }
}

// Initialise the zero position - guides the user through the process
void initialiseZeroPosition() {
  Serial.println("\n--- Zero Position Initialisation ---");
  Serial.println("Please manually position the motor to the desired zero position.");
  Serial.println("You can use the following commands:");
  printInitHelp();
}

// Restore the zero position from EEPROM
void restoreZeroPosition() {
  // Get stored encoder angle
  float storedAngle = loadEncoderPosition();
  
  // Get current encoder angle
  float currentAngle = stepper.encoder.getAngle();
  
  // Calculate angle difference
  float angleDiff = currentAngle - storedAngle;
  
  // Adjust for angle wrap-around
  if (angleDiff > 180.0) angleDiff -= 360.0;
  if (angleDiff < -180.0) angleDiff += 360.0;
  
  Serial.print("Stored angle: ");
  Serial.print(storedAngle);
  Serial.print(", Current angle: ");
  Serial.print(currentAngle);
  Serial.print(", Difference: ");
  Serial.print(angleDiff);
  Serial.println(" degrees");
  
  // Move motor to correct position if difference is outside tolerance
  if (abs(angleDiff) > POSITION_TOLERANCE) {
    Serial.print("Moving motor ");
    Serial.print(-angleDiff);
    Serial.println(" degrees to restore zero position...");
    
    // Move motor to correct position
    stepper.moveAngle(-angleDiff);
    
    // Wait for movement to complete
    delay(500);
    
    // Verify position
    currentAngle = stepper.encoder.getAngle();
    angleDiff = currentAngle - storedAngle;
    
    // Adjust for angle wrap-around
    if (angleDiff > 180.0) angleDiff -= 360.0;
    if (angleDiff < -180.0) angleDiff += 360.0;
    
    Serial.print("Position after adjustment: ");
    Serial.print(currentAngle);
    Serial.print(", Remaining difference: ");
    Serial.print(angleDiff);
    Serial.println(" degrees");
    
    if (abs(angleDiff) > POSITION_TOLERANCE) {
      Serial.println("Warning: Zero position could not be restored precisely!");
    }
  } else {
    Serial.println("Motor already at correct zero position.");
  }
  
  // Reset the step counter in the driver
  stepper.driver.setPosition(0);
  Serial.println("Step counter reset to zero - this is now the home reference.");
  
  zeroPositionInitialised = true;
}

// Save encoder angle to EEPROM
void saveEncoderPosition(float angle) {
  // Convert float to bytes
  byte* angleBytes = (byte*)&angle;
  
  // Write bytes to EEPROM
  for (int i = 0; i < sizeof(float); i++) {
    EEPROM.write(EEPROM_ENCODER_ADDR + i, angleBytes[i]);
  }
}

// Load encoder angle from EEPROM
float loadEncoderPosition() {
  // Read bytes from EEPROM
  byte angleBytes[sizeof(float)];
  for (int i = 0; i < sizeof(float); i++) {
    angleBytes[i] = EEPROM.read(EEPROM_ENCODER_ADDR + i);
  }
  
  // Convert bytes to float
  float angle;
  memcpy(&angle, angleBytes, sizeof(float));
  
  return angle;
}

// Check if EEPROM has been initialised
bool isEEPROMInitialised() {
  return EEPROM.read(EEPROM_INITIALISED_ADDR) == EEPROM_MAGIC_NUMBER;
}

// Set EEPROM initialised flag
void setEEPROMInitialised(bool initialised) {
  EEPROM.write(EEPROM_INITIALISED_ADDR, initialised ? EEPROM_MAGIC_NUMBER : 0);
}

// Print initialisation help information
void printInitHelp() {
  Serial.println("\n--- Zero Position Initialisation Help ---");
  Serial.println("Available commands:");
  Serial.println("  '+' - Move clockwise 1 degree");
  Serial.println("  '-' - Move counter-clockwise 1 degree");
  Serial.println("  '5' - Move clockwise 5 degrees");
  Serial.println("  '%' - Move counter-clockwise 5 degrees");
  Serial.println("  's' - Save current position as zero and exit initialisation");
  Serial.println("  'h' - Show this help");
  Serial.println("Position the motor at the desired zero position, then press 's' to save.");
  Serial.println("-------------------------------");
}

// Check if a beat number is in the specified pattern
bool isInPattern(int beat, int patternIndex) {
  if (patternIndex == PATTERN_DISABLED) {
    return false;
  }

  const byte* pattern = patterns[patternIndex];

  // Check each beat in the pattern until we reach the terminator
  for (int i = 0; pattern[i] != BEAT_TERMINATOR; i++) {
    if (pattern[i] == beat) {
      return true;
    }
  }

  return false;
}

// Check timer to see if we need to change patterns
void checkTimer() {
  // Get current time in seconds
  unsigned long now = getTimeInSeconds();

  // Check each timer period
  for (int i = 0; i < sizeof(timerData) / sizeof(timerData[0]); i++) {
    // Convert timer start/end to seconds
    unsigned long startTime = timerData[i][0] * 60 + timerData[i][1];
    unsigned long endTime = timerData[i][2] * 60 + timerData[i][3];

    // Check if current time is within this timer period
    if (now >= startTime && now < endTime) {
      // Get the pattern for this timer period
      byte timerPattern = timerPatterns[i];

      // Only update if different
      if (activePattern != timerPattern) {
        int oldPattern = activePattern;
        activePattern = timerPattern;

        Serial.print("Timer: Changed pattern from ");
        Serial.print(oldPattern);
        Serial.print(" to ");
        Serial.print(activePattern);
        Serial.print(" at time ");
        Serial.print(now / 60);  // minutes
        Serial.print(":");
        Serial.print(now % 60);  // seconds
        Serial.println();

        if (activePattern == PATTERN_DISABLED) {
          Serial.println("Pattern disabled (silence)");
        } else {
          printPattern(activePattern);
        }
      }
      return;  // Found the right time slot, exit
    }
  }
}

// Get current time in seconds since start (for timer mode)
unsigned long getTimeInSeconds() {
  // Use millis() to simulate time
  return (millis() / 1000) % 3600;  // Cycle every hour (3600 seconds)
}

// Trigger the motor to strike once
void triggerMotor() {
  motor.reset();
  motorActive = true;
}

// Update timing based on BPM
void updateTiming() {
  // This calculates the exact interval for 120 BPM (500ms per beat)
  beatInterval = 60000 / bpm;  // Convert BPM to milliseconds

  // Reset the beat timer when changing tempo to prevent timing issues
  prevBeatTime = millis();

  Serial.print("Tempo: ");
  Serial.print(bpm);
  Serial.print(" BPM (");
  Serial.print(beatInterval);
  Serial.println("ms per beat)");

  // Double-check the calculation
  float beatsPerSecond = bpm / 60.0;
  Serial.print("This is ");
  Serial.print(beatsPerSecond, 2);
  Serial.println(" beats per second");
}

// Process serial commands
void processCommand() {
  char cmd = Serial.read();

  switch (cmd) {
    case 'h':  // Help
      printHelp();
      break;

    case '0':  // Stop pattern
      if (manualRun) {
        activePattern = PATTERN_DISABLED;
        Serial.println("Pattern disabled");
      }
      break;

    case 'm':  // Switch to manual mode
      manualRun = true;
      timerOn = false;
      Serial.println("Switched to MANUAL mode - timer disabled");
      break;

    case 't':  // Switch to timer mode
      manualRun = false;
      timerOn = true;
      Serial.println("Switched to TIMER mode - using scheduled patterns");
      break;

    case 'p':  // Print current status
      printStatus();
      break;
      
    case 'i':  // Re-enter initialisation mode
      Serial.println("Entering zero position initialisation mode...");
      initMode = true;
      setupComplete = false;
      initialiseZeroPosition();
      break;

    default:
      // Check for pattern selection (1-9, then a-f for patterns 10-15)
      if (cmd >= '1' && cmd <= '9' && manualRun) {
        // For commands 1-9, select patterns 0-8
        activePattern = cmd - '1';
        Serial.print("Pattern ");
        Serial.print(activePattern);
        Serial.println(" activated");
        printPattern(activePattern);
      } else if (cmd >= 'a' && cmd <= 'f' && manualRun) {
        // For commands a-f, select patterns 9-14
        activePattern = (cmd - 'a') + 9;
        Serial.print("Pattern ");
        Serial.print(activePattern);
        Serial.println(" activated");
        printPattern(activePattern);
      } else if (cmd == 'x') {  // Manual motor trigger
        triggerMotor();
        Serial.println("Motor triggered manually");
      } else if (cmd == 's') {  // Stop motor
        stepper.stop();
        motorActive = false;
        Serial.println("Motor stopped");
      } else if (cmd == 'r') {  // Reset beat counter
        beatCount = 0;
        prevBeatTime = millis();
        Serial.println("Beat counter reset to 0");
      } else if (cmd == '+') {  // Increase tempo
        bpm += 10;
        if (bpm > 300) bpm = 300;
        updateTiming();
      } else if (cmd == '-') {  // Decrease tempo
        bpm -= 10;
        if (bpm < 30) bpm = 30;
        updateTiming();
      } else if (cmd == 'z') {  // Return to zero position
        Serial.println("Returning to zero position...");
        stepper.driver.setPosition(0); // First reset position counter to 0 at current position
        stepper.moveToAngle(0);        // Then move to angle 0
        delay(500);
        Serial.println("At zero position");
      }
      break;
  }
}

// Print the beats in a pattern
void printPattern(int patternIndex) {
  if (patternIndex < 0 || patternIndex >= sizeof(patterns) / sizeof(patterns[0])) {
    Serial.println("Invalid pattern index");
    return;
  }

  const byte* pattern = patterns[patternIndex];
  Serial.print("Beats: ");
  for (int i = 0; pattern[i] != BEAT_TERMINATOR; i++) {
    Serial.print(pattern[i]);
    Serial.print(" ");
  }
  Serial.println();
}

// Print current status
void printStatus() {
  Serial.println("\n--- Beat Sequencer Status ---");
  Serial.print("Mode: ");
  Serial.println(manualRun ? "MANUAL" : "TIMER");

  Serial.print("Timer Mode: ");
  Serial.println(timerOn ? "ENABLED" : "DISABLED");

  Serial.print("Zero Position Initialised: ");
  Serial.println(zeroPositionInitialised ? "YES" : "NO");

  Serial.print("Current Encoder Angle: ");
  Serial.println(stepper.encoder.getAngle());

  Serial.print("Active Pattern: ");
  if (activePattern == PATTERN_DISABLED) {
    Serial.println("DISABLED (silence)");
  } else {
    Serial.print(activePattern);
    Serial.print(" - Beats: ");
    const byte* pattern = patterns[activePattern];
    for (int i = 0; pattern[i] != BEAT_TERMINATOR; i++) {
      Serial.print(pattern[i]);
      Serial.print(" ");
    }
    Serial.println();
  }

  Serial.print("Current Beat: ");
  Serial.println(beatCount);

  Serial.print("Tempo: ");
  Serial.print(bpm);
  Serial.print(" BPM (");
  Serial.print(beatInterval);
  Serial.println("ms per beat)");

  Serial.print("Motor Active: ");
  Serial.println(motorActive ? "YES" : "NO");

  if (timerOn && !manualRun) {
    // Print current timer info
    unsigned long now = getTimeInSeconds();
    Serial.print("Current Time: ");
    Serial.print(now / 60);  // minutes
    Serial.print(":");
    Serial.print(now % 60);  // seconds
    Serial.println();

    // Find current timer period
    for (int i = 0; i < sizeof(timerData) / sizeof(timerData[0]); i++) {
      unsigned long startTime = timerData[i][0] * 60 + timerData[i][1];
      unsigned long endTime = timerData[i][2] * 60 + timerData[i][3];

      if (now >= startTime && now < endTime) {
        Serial.print("Current Timer Period: ");
        Serial.print(timerData[i][0]);
        Serial.print(":");
        Serial.print(timerData[i][1]);
        Serial.print(" to ");
        Serial.print(timerData[i][2]);
        Serial.print(":");
        Serial.print(timerData[i][3]);
        Serial.print(" (Pattern: ");
        Serial.print(timerPatterns[i]);
        Serial.println(")");
        break;
      }
    }
  }

  Serial.println("-------------------------------");
}

// Print help information
void printHelp() {
  Serial.println("\n--- Beat Sequencer v2.2 ---");
  Serial.println("Available commands:");
  Serial.println("  'm' - Switch to manual mode");
  Serial.println("  't' - Switch to timer mode");
  Serial.println("  'p' - Print current status");
  Serial.println("  'i' - Enter zero position initialisation mode");
  Serial.println("  'z' - Return to zero position");

  Serial.println("\nManual Mode Commands:");
  Serial.println("  '1'-'9' - Patterns 0-8");
  Serial.println("  'a'-'f' - Patterns 9-14");
  Serial.println("  '0' - Disable pattern");

  Serial.println("\nCommon Commands:");
  Serial.println("  'x' - Trigger motor manually");
  Serial.println("  's' - Stop motor");
  Serial.println("  'r' - Reset beat counter to 0");
  Serial.println("  '+' - Increase tempo");
  Serial.println("  '-' - Decrease tempo");
  Serial.println("  'h' - Show this help");
  Serial.println("-------------------------------");
  Serial.print(">>> ");
}