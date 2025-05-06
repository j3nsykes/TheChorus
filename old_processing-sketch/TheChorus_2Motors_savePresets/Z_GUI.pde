
import controlP5.*;

ControlP5 cp5;

int sliderTicks = 100;
public float sliderValue = 0;

void setupGUI(){

 cp5 = new ControlP5(this);
 
   cp5.addSlider("bpm")
     .setPosition(10,10)
     .setWidth(400)
     .setRange(10,280) // values can range from big to small as well
     .setValue(60)
 
     .setNumberOfTickMarks(10)
     .setSliderMode(Slider.FIX)
     .setSize(400,20)
     .setColorBackground(color(141))
     .setColorForeground(color(0, 198, 109))
     .setColorValueLabel(color(255))
     .setColorLabel(color(0))
     .setFont(createFont("Arial",18))
     .setColorTickMark(color(0))
     .setColorActive(color(1,225,145))
     ;
}

void drawGUI(){

  fill(sliderTicks);
  rect(0,350,width,80);
}

public void bpm(int input) {
  bpm = input;
  println("a slider event. setting value to "+bpm);
}
