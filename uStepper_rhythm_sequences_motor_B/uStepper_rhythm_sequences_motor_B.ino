/********************************************************************************************
*         File:  BeatSequencer_uStepper.ino                                               *
*      Version:  2.1.0                                                                     *
*         Date:  April 27th, 2025                                                          *
*  Description:  Beat-based sequencer for uStepper S32 cymbal striker                      *
*                This sketch implements a beat sequencer that activates stepper motors     *
*                based on programmable patterns at specific times or BPM settings          *
*                                                                                          *
********************************************************************************************/

#include <UstepperS32.h>
#include "MovingMotor.h"

// Define constants
#define STEPSPERREV 200                  // 200 steps per revolution (1.8° motor)
#define RES (STEPSPERREV * 256) / 360.0  // Steps per degree
#define STALLSENSITIVITY 2               // Default stall sensitivity
#define BEAT_TERMINATOR 255              // Value to indicate end of a beat pattern
#define PATTERN_DISABLED 255             // Special value to indicate pattern is disabled

// Global variables
UstepperS32 stepper;
bool setupComplete = false;

// Mode selection
bool manualRun = false;     // Manual mode vs. timer mode
bool timerOn = true;        // Whether timer-based playback is active

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
int bpm = 120;               // Base tempo in beats per minute
unsigned long beatInterval;  // Milliseconds per beat (calculated from BPM)
unsigned long prevBeatTime;  // Time of last beat
int beatCount = 0;           // Current beat position
int activePattern = PATTERN_DISABLED; // Which pattern is currently active

// Motor activation flags
bool motorActive = false;    // Whether the motor is currently moving

// Beat patterns - each pattern is a list of beat numbers (0-59) where the motor should trigger
// BEAT_TERMINATOR (255) marks the end of each pattern. Using a high number not in use 
const byte patterns[][16] = {
  // Pattern 0: Every 15 beats. All at once. Same across both motors.
  { 0, 15, 30, 45, BEAT_TERMINATOR },
  
  // Pattern 1: Up Down Up Down
  { 0, 15, 32, 49, BEAT_TERMINATOR },
  
  // Pattern 2: Up Down Up Down #B
  { 7, 23, 41, 59, BEAT_TERMINATOR },
  
  // Pattern 3: W rhythm
  { 0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, BEAT_TERMINATOR },
  
  // Pattern 4: W rhythm #B
  { 2, 6, 10, 14, 18, 22, 26, 30, 34, 38, 42, 46, 50, 54, 58, BEAT_TERMINATOR },
  
  // Pattern 5: Random occasional
  { 4, 18, 26, 43, BEAT_TERMINATOR },

  // Pattern 6: Random occasional #B
  { 9, 25, 50, BEAT_TERMINATOR },

  // Pattern 7: Random - beats 4 & 20
  { 4, 20, BEAT_TERMINATOR },

  // Pattern 8: Random - beats 22 & 45
  { 22, 45, BEAT_TERMINATOR },

  // Pattern 9: Off-beat syncopate #A
  { 0, 12, 16, 24, 28, 32, 40, 44, 48, 56, BEAT_TERMINATOR },

  // Pattern 10: off -beat #B
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
  { 11, 30, 12, 30 },  // Pattern 2 up down
  { 12, 30, 13, 37 },  // Pattern 4 W rhythm 
  { 13, 37, 18, 17 },  // Silence
  { 18, 17, 18, 30 },  // Pattern 6 random
  { 18, 30, 21, 30 },  // Silence
  { 21, 30, 22, 3 },   // Pattern 10 syncopate
  { 22, 3, 23, 3 },    // Silence
  { 23, 3, 24, 3 },    // Pattern 12 single beat
  { 24, 3, 25, 20 },   // Silence
  { 25, 20, 25, 34 },  // Pattern 6 random
  { 25, 34, 26, 24 },  // Silence
  { 26, 24, 26, 37 },  // Pattern 14
  { 26, 37, 30, 0 }    // Silence
};

// Pattern index for each timer slot - which pattern plays during each timer period
const byte timerPatterns[] = {
  PATTERN_DISABLED, // Silence
  0,                // Pattern 0 
  PATTERN_DISABLED, // Silence
  2,                // Pattern 2
  4,                // Pattern 4
  PATTERN_DISABLED, // Silence
  6,                // Pattern 6
  PATTERN_DISABLED, // Silence
  10,                // Pattern 10
  PATTERN_DISABLED, // Silence
  12,                // Pattern 12
  PATTERN_DISABLED, // Silence
  6,                // Pattern 6
  PATTERN_DISABLED, // Silence
  14,                // Pattern 14
  PATTERN_DISABLED  // Silence
};

// Forward declarations
void printHelp();
void triggerMotor();
void updateTiming();
void checkTimer();
unsigned long getTimeInSeconds();

void setup() {
  // Initialise serial communication
  Serial.begin(9600);
  Serial.println("Beat Sequencer v2.1 with Timer Mode");
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
  
  // Initialise uStepper with closed loop control
  stepper.setup(CLOSEDLOOP, STEPSPERREV, 1, 1, 1, 1, 0);
  stepper.checkOrientation(30.0);
  
  // Initialise motor
  motor.init(&stepper);
  
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

void loop() {
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
        Serial.print(now / 60); // minutes
        Serial.print(":");
        Serial.print(now % 60); // seconds
        Serial.println();
        
        if (activePattern == PATTERN_DISABLED) {
          Serial.println("Pattern disabled (silence)");
        } else {
          printPattern(activePattern);
        }
      }
      return; // Found the right time slot, exit
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
    case 'h': // Help
      printHelp();
      break;
      
    case '0': // Stop pattern
      if (manualRun) {
        activePattern = PATTERN_DISABLED;
        Serial.println("Pattern disabled");
      }
      break;
      
    case 'm': // Switch to manual mode
      manualRun = true;
      timerOn = false;
      Serial.println("Switched to MANUAL mode - timer disabled");
      break;
      
    case 't': // Switch to timer mode
      manualRun = false;
      timerOn = true;
      Serial.println("Switched to TIMER mode - using scheduled patterns");
      break;
      
    case 'p': // Print current status
      printStatus();
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
      }
      else if (cmd >= 'a' && cmd <= 'f' && manualRun) {
        // For commands a-f, select patterns 9-14
        activePattern = (cmd - 'a') + 9;
        Serial.print("Pattern ");
        Serial.print(activePattern);
        Serial.println(" activated");
        printPattern(activePattern);
      }
      else if (cmd == 'x') { // Manual motor trigger
        triggerMotor();
        Serial.println("Motor triggered manually");
      }
      else if (cmd == 's') { // Stop motor
        stepper.stop();
        motorActive = false;
        Serial.println("Motor stopped");
      }
      else if (cmd == 'r') { // Reset beat counter
        beatCount = 0;
        prevBeatTime = millis();
        Serial.println("Beat counter reset to 0");
      }
      else if (cmd == '+') { // Increase tempo
        bpm += 10;
        if (bpm > 300) bpm = 300;
        updateTiming();
      }
      else if (cmd == '-') { // Decrease tempo
        bpm -= 10;
        if (bpm < 30) bpm = 30;
        updateTiming();
      }
      break;
  }
}

// Print the beats in a pattern
void printPattern(int patternIndex) {
  if (patternIndex < 0 || patternIndex >= sizeof(patterns)/sizeof(patterns[0])) {
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
    Serial.print(now / 60); // minutes
    Serial.print(":");
    Serial.print(now % 60); // seconds
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
  Serial.println("\n--- Beat Sequencer v2.1 ---");
  Serial.println("Available commands:");
  Serial.println("  'm' - Switch to manual mode");
  Serial.println("  't' - Switch to timer mode");
  Serial.println("  'p' - Print current status");
  
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