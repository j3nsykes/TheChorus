//sequence tick class
class Tick implements Instrument
{
  void noteOn( float dur )
  {
    if ( hand1Row[beat] ) hand1.trigger();
    if ( hand1Row[beat] ) flipMotor1();
    if ( hand2Row[beat] ) hand2.trigger();
    if ( hand2Row[beat] ) flipMotor2();

  }

  void noteOff()
  {

    // next beat
    beat = (beat+1)%60;
    // set the new tempo
    out.setTempo( bpm );
    // play this again right now, with a sixteenth note duration
    out.playNote( 0, 0.25f, this );
  }
}



// simple class for drawing the gui
class Rect
{
  int x, y, w, h;
  boolean[] steps;
  int stepId;


  public Rect(int _x, int _y, boolean[] _steps, int _id)
  {
    x = _x;
    y = _y;
    w = 20;
    h = 30;
    steps = _steps;
    stepId = _id;
  }

  public void preload(int _index) {

    int index =_index;
    int[] currentArray = presetArrays[index];

     //all same beat slow
    presetArrays[0] = new int[] {0, 15, 30, 45};

    // up down up down
    presetArrays[1] = new int[] {0, 15, 32, 49};
    presetArrays[2] = new int[] {7,23,41,59};

    //W and syncopate
    presetArrays[3] = new int[] {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56};
    presetArrays[4] = new int[] {2, 6, 10, 14, 18, 22, 26, 30, 34, 38, 42, 46, 50, 54, 58};
    
    //random occasional
    presetArrays[5] = new int[] {4, 18, 26, 43};
    presetArrays[6] = new int[] {9, 25, 50};
   
    //random
    presetArrays[7] = new int[] {4,20};
    presetArrays[8] = new int[] {22,45};
    

    //off beat syncopate
    presetArrays[9] = new int[] {0, 12, 16, 24, 28, 32, 40, 44, 48, 56};
    presetArrays[10] = new int[] {9, 24, 36, 48, 52};

    //single random
    presetArrays[11] = new int[] {0};
    presetArrays[12] = new int[] {22};
    presetArrays[13] = new int[]{4};
    presetArrays[14] = new int[]{45};
    
    
    for (int i = 0; i < currentArray.length; i++) {
      //println(currentArray[i]);
      steps[currentArray[i]]=true;
    }
  }

  public void reset(int _index) {
    int index =_index;
     //all same beat slow
    presetArrays[0] = new int[] {0, 15, 30, 45};

    // up down up down
    presetArrays[1] = new int[] {0, 15, 32, 49};
    presetArrays[2] = new int[] {7,23,41,59};

    //W and syncopate
    presetArrays[3] = new int[] {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56};
    presetArrays[4] = new int[] {2, 6, 10, 14, 18, 22, 26, 30, 34, 38, 42, 46, 50, 54, 58};
    
    //random occasional
    presetArrays[5] = new int[] {4, 18, 26, 43};
    presetArrays[6] = new int[] {9, 25, 50};
   
    //random
    presetArrays[7] = new int[] {4,20};
    presetArrays[8] = new int[] {22,45};
    

    //off beat syncopate
    presetArrays[9] = new int[] {0, 12, 16, 24, 28, 32, 40, 44, 48, 56};
    presetArrays[10] = new int[] {9, 24, 36, 48, 52};
    

    //single random
    presetArrays[11] = new int[] {0};
    presetArrays[12] = new int[] {22};
    presetArrays[13] = new int[]{4};
    presetArrays[14] = new int[]{45};

    int[] currentArray = presetArrays[index];



    for (int i = 0; i < currentArray.length; i++) {
      //println(currentArray[i]);
      steps[currentArray[i]]=false;
    }
  }


  public void draw()
  {
    if ( steps[stepId] )
    {
      if (stepId%4==0) {
        fill(0, 198, 109);
      } else {
        fill(1, 255, 145);
      }
    } else
    {
      fill(141);
    }

    rect(x, y, w, h, 3);
  }

 public void mousePressed()
  {
    if ( mouseX >= x && mouseX <= x+w && mouseY >= y && mouseY <= y+h )
    {
      steps[stepId] = !steps[stepId];
      
      // If we're in case 6, track the selected squares
      if (inCase6) {
        int squareId = stepId;
        
        if (steps == hand1Row) {
          // This is a square in the first row (hand1)
          if (steps[stepId]) {
            // If the square is now selected, add it to our hand1 list if not already there
            if (!selectedSquaresHand1.contains(squareId)) {
              selectedSquaresHand1.add(squareId);
            }
          } else {
            // If the square is now deselected, remove it from our hand1 list
            selectedSquaresHand1.remove(Integer.valueOf(squareId));
          }
        } else if (steps == hand2Row) {
          // This is a square in the second row (hand2)
          if (steps[stepId]) {
            // If the square is now selected, add it to our hand2 list if not already there
            if (!selectedSquaresHand2.contains(squareId)) {
              selectedSquaresHand2.add(squareId);
            }
          } else {
            // If the square is now deselected, remove it from our hand2 list
            selectedSquaresHand2.remove(Integer.valueOf(squareId));
          }
        }
      }
    }
  }
  }
