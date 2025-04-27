import processing.opengl.*;
import ddf.minim.*;
import ddf.minim.ugens.*;

Minim       minim;
AudioOutput out;

Sampler     hand1;
Sampler     hand2;


boolean[] hand1Row = new boolean[60];
boolean[] hand2Row = new boolean[60];


ArrayList<Rect> buttons = new ArrayList<Rect>();

int bpm = 60;
int totalSteps = 60;

int beat; // which beat we're on
int c=0;

MovingMotor motor1, motor2;
boolean flip1, flip2 = false;

boolean manualRun = true;
boolean timerOn = false;

void setup()
{
  size(1600, 600);
  timerSetup();

  minim = new Minim(this);
  out   = minim.getLineOut();
  hand1 = new Sampler( "tambourine-02.wav", 4, minim );
  hand2 = new Sampler( "tambourine-02.wav", 4, minim );

  // patch samplers to the output
  hand1.patch( out );
  hand2.patch( out );


  for (int i = 0; i < totalSteps; i++)
  {
    buttons.add( new Rect(10+i*26, 80, hand1Row, i ) );
    buttons.add( new Rect(10+i*26, 130, hand2Row, i ) );

  }

  beat = 0;

  // start the sequencer
  out.setTempo( bpm );
  out.playNote( 0, 0.25f, new Tick() );

  dmxSetup();
  //on start?

  dmxOutput.set(3, 70);
  dmxOutput.set(1, 0);

  //add power up and down function
  //powerUpDown();

  //startX, startY, distance, speed

  motor1 = new MovingMotor(0, 50, 1, 18, 37);   //was 90 tramway, make 37 Turner
  motor2 = new MovingMotor(0, 150, 1, 18, 77); //77m turner

}

void draw()
{
  background(221);
  if (timerOn) {
    timerRun();
  }
  runPreset();
  debugText();


  motor1.update(flip1);
  //rC, sC, pC
  motor1.output(0, 0, 0); //zero in the array index


  motor2.update(flip2);
  //rC, sC, pC
  motor2.output(1, 1, 1);



  cycleChecks();

  //visual end point marker
  stroke(255, 0, 0);
  line(170, height/2, 170, height-5);

  //draw the squares
  for (int i = 0; i < buttons.size(); ++i)
  {
    noStroke();
    buttons.get(i).draw();
  }

  stroke(128);
  if ( beat % 4 == 0 )
  {
    fill(0, 198, 109);
  } else {
    fill(200, 200, 200);
  }

  // beat marker
  rect(10+beat*26, 55, 14, 9);
}

void debugText() {
  fill(0);
  textSize(22);
  text("BPM: "+bpm, 290, 30);
  text("PRESET ID NUM: "+currentIndex, 30, 30);


  for (int i=1; i<3; i++) {
    fill(30);
    textSize(22);
    text("H: " + i, 1460, 50+(i*50));
  }

  pushMatrix();
  textSize(42);
  noStroke();
  fill(100);
  translate(0, 350);
  text(motor1.x, 400, 0);
  text(motor2.x, 400, 50);
  popMatrix();


}

void cycleChecks() {
  // reset the ellipses if one cycle has completed
  if (cyclesCompleted1()) {
    //println("reset1");
    flip1=false;
    motor1.reset();
  }

  // reset the ellipses if one cycle has completed
  if (cyclesCompleted2()) {
    // println("reset2");
    flip2=false;
    motor2.reset();
  }

}
boolean cyclesCompleted1() {
  // check if all ellipses have completed one cycle
  return motor1.cycles == 1 ;
}
boolean cyclesCompleted2() {
  // check if all ellipses have completed one cycle
  return motor2.cycles == 1;
}


void resetAllMotors() {
  // reset all ellipses
  motor1.reset();
  motor2.reset();

}

void flipMotor1() {
  flip1 = true;
}
void flipMotor2() {
  flip2 = true;
}



void powerUpDown() {
  for (int i=0; i<2; i++) {
    //dmxOutput.set(rampChan[i], 70);
    dmxOutput.set(SpeedChan[i], 70);
    dmxOutput.set(PositionChan[i], 0);//set all on
  }
}
void keyPressed() {
  if (key =='r') { //run motors manually
    flip1 = true;
    //flip2 = true;

  }
  //clear
  if (key =='c') { //clear sequener
    //println("reset");
    currentIndex=0;
  }
  if (key =='m') { //clear sequener
    //println("reset");
    manualRun=true;
    timerOn= false;
  }
  if (key =='s') { //clear sequener
    //println("reset");
    manualRun=false;
    timerOn= true;
  }
  if(manualRun){
  //move index ID
  if (key == CODED) {
    if (keyCode == UP) {
      currentIndex++;
    } else if (keyCode == DOWN) {
      currentIndex--;
    }
  }


  //don't break
  if (currentIndex>10) {
    currentIndex=0;
  } else if (currentIndex<0) {
    currentIndex=0;
  }

  switch(key) {
  case '0':
    currentIndex=0;
    break;
  case '1':
    currentIndex=1;
    break;
  case '2':
    currentIndex=2;
    break;
  case '3':
    currentIndex=3;
    break;
  case '4':
    currentIndex=4;
    break;
  case '5':
    currentIndex=5;
    break;
  case '6':
    currentIndex=6;
    break;
      case '7':
    currentIndex=7;
    break;
      case '8':
    currentIndex=8;
    break;
      case '9':
    currentIndex=9;
    break;
  }
}
}



void mousePressed()
{
  for (int i = 0; i < buttons.size(); ++i)
  {
    buttons.get(i).mousePressed();
  }
}
