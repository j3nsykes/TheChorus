import dmxP512.*;
import processing.serial.*;

DmxP512 dmxOutput;
int universeSize=128;
boolean isConnected = false;

boolean DMXPRO=true;
String DMXPRO_PORT="dev/cu.usbserial-EN442868";//case matters ! on windows port must be upper cased.
int DMXPRO_BAUDRATE=115000;

int [] PositionChan = {1, 5, 9, 13};
int [] SpeedChan = {3, 7, 11, 15};
int [] Position2Chan = {2, 6, 10, 14};
int [] rampChan = {4, 8, 12, 16};



boolean completedOneCycle = false;
boolean flip = true;

void dmxSetup() {
  String[] portNames = Serial.list();
  printArray(portNames);

  try {
    dmxOutput = new DmxP512(this, universeSize, false);
    dmxOutput.setupDmxPro(DMXPRO_PORT, DMXPRO_BAUDRATE);
    isConnected = true;
    println("Serial connection established.");
  }
  catch (RuntimeException e) {
    isConnected = false;
    println("Serial connection failed: " + e.getMessage());
  }
}
