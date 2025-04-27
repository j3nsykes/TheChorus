class MovingMotor {
  int x; // current x position of the ellipse
  float y; // current y position of the ellipse
  float direction; // direction of movement, 1 for moving right, -1 for moving left
  float speed; // speed of the ellipse
  float distance; // distance that the ellipse should move
  int cycles = 0; // number of completed cycles
  boolean flag = false;
  boolean on =false;
  int start = 0;
  int end; //maybe add this into the class if each motor needs different end point
  int lastX=0;

  MovingMotor(int startX, float startY, float startDistance, float startSpeed, int endX) {
    x = startX;
    y = startY;
    end = endX;
    direction = 1;
    speed = startSpeed;
    distance = startDistance;
  }

  void update(boolean _flip) {
    on = _flip;

    if (on) {
      if (!flag) {

        // move the ellipse in the x direction by the distance and speed
        x += direction * distance * speed;


        // check if the ellipse has reached the end of the cycle
        if (x >= end) {
          direction = -1; // reverse the direction of movement
        } else if (x <= start) {
          direction = 1; // reverse the direction of movement
          cycles++; // increment the number of completed cycles
        }
      }

      // stop the sketch if one cycle has been completed
      if (cycles == 1) {
        flag = true;
      }
    }
    //println("flag: "+flag);
  }

  void output(int _rC, int _sC, int _pC) {
    int r = _rC;
    int s =_sC;
    int p = _pC;
    
//    if(lastX!=x){
    // draw the visual ellipse at its current position
    pushMatrix();
    noStroke();
    fill(100);
    translate(0, 300);
    ellipse(10+x, y, 50, 50);
    popMatrix();
    //}
    //lastX=x;

    //dmx out
    if (isConnected) {
      //println("connected");
      //println("dmx");
      //dmxOutput.set(rampChan[r], 120);
   dmxOutput.set(SpeedChan[s], 80); //motor manual speed set here. was 255 maybe make slower to begin with 
   dmxOutput.set(PositionChan[p], x);//set all on
    }
    else{
       //println("not connected");
    }
  }

  void reset() {
    // reset the ellipse for another cycle
    x = 0; // initial x position of the ellipse
    direction = 1; // initial direction of movement, 1 for moving right, -1 for moving left
    cycles = 0; // number of completed cycles
    flag = false; // reset the flag
  }
}
