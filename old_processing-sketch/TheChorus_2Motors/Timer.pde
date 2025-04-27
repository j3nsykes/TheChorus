
//start min, start sec, end min, end sec
int[][] timerData = {
{00, 00, 00, 40}, //silence
{00, 40, 8, 55}, //P1 (preset 1)
{8, 55, 11, 30}, //silence
{11, 30, 12, 30}, //P2 (preset 2 timecode)
{12, 30, 13, 37}, //P3
{13, 37, 18, 17}, //silence
{18, 17, 18, 30}, //P4
{18, 30, 21, 30}, //silence
{21, 30, 22, 03}, //P5
{22, 03, 23, 03}, //silence
{23, 03, 24, 03}, //P7
{24, 03, 25, 20}, //silence
{25, 20, 25, 34}, //P8
{25, 34, 26, 24}, //silence
{26, 24, 26, 37}, //P9
{26, 37, 30, 00}, //silence
{30, 00, 30, 40}, //loop point silence Loops every hour
{30, 40, 38, 55},   //P1 random
{38, 55, 41, 30}, //silence
{41, 30, 42, 30}, //P2 all
{42, 30, 43, 37}, //P3 W
{43, 37, 48, 17}, //silence
{48, 17, 48, 34}, //P4 high1
{48, 34, 51, 30}, //silence
{51, 30, 52, 03}, //P5 2 up
{52, 03, 53, 03}, //silence
{53, 03, 54, 03}, //P7
{54, 03, 55, 20}, //silence
{55, 20, 55, 24}, //P8
{55, 24, 56, 24}, //silence
{56, 24, 56, 40}, //P9
{56, 40, 59, 59} //silence
};
Timer[] timers = new Timer[32];

//index number indicates which preset case should play at the corresponding timerData index ID above. 
//see presets tab for swicth case numbers and corresponding preset arrays from sequencer tab/
//0 = silence/off 
int [] index ={0, 1, 0, 2, 3, 0, 4, 0, 5, 0, 7, 0, 8, 0, 9, 0, 0, 1, 0, 2, 3, 0, 4, 0, 5, 0, 7, 0, 8, 0, 9, 0}; //order of preset patterns (0 is off)



int lastRunningTimerIndex = -1; // initialize to -1 to indicate no timer is running


void timerSetup() {

  for (int i = 0; i < timerData.length; i++) {
    int[] data = timerData[i];

    timers[i] = new Timer(data[0], data[1], data[2], data[3]);
  }
}




void timerRun() {
  pushMatrix();
  fill(0);
  textSize(50);
  //Current time
  text(hour() + ":" + nf(minute(), 2) + ":" + nf(second(), 2), 550, 450);
  textSize(20);

  popMatrix();

  for (int i = 0; i < timers.length; i++) {
    timers[i].start();
    timers[i].stop();

    if (timers[i].isRunning()) {
      lastRunningTimerIndex = i;
    }
    else{
    currentIndex =0;
    }
  }

  if (lastRunningTimerIndex == -1) {
    currentIndex = 0; // set currentIndex to 0 if no timer is running
  } else {
    currentIndex = index[lastRunningTimerIndex];
    text("Timer "+ lastRunningTimerIndex + " is running" + "  Index: "+ currentIndex, 550, 500);
    
  }


}



class Timer {
  int startMin, startSec;
  int stopMin, stopSec;
  boolean running;

  Timer(int sm, int ss, int em, int es) {
    startMin = sm;
    startSec = ss;
    stopMin = em;
    stopSec = es;
    running = false;
  }

  void start() {
    int currentMin = minute();
    int currentSec = second();
    if (currentMin >= startMin && currentSec >= startSec) {
      running = true;
    }
  }

  void stop() {
    int currentMin = minute();
    int currentSec = second();
    if (currentMin >= stopMin && currentSec >= stopSec) {
      running = false;
    }
  }

  boolean isRunning() {
    return running;
  }
}
