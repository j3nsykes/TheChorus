//put stepId index in this 2D array for presets

int[][] presetArrays = new int[15][];
int currentIndex = 0;
boolean startBeat=false;

void runPreset() {

  //cases can eventually change on timers.
  //change key to be currentIndex
  switch (currentIndex) {
  case 0:
    //start up nothing all off
    resetButtons();
    bpm = 50;
    inCase6 = false;
    break;
  case 1:
    resetButtons();
    //bpm = 60;
    bpm = 35;
    inCase6 = false;
    // random occassional preset 4
    for (int j = 0; j < buttons.size(); ++j)
    {
      //populate the squares
      buttons.get(0).preload(7); //preload hand1 bells
      buttons.get(1).preload(8); //preload hand2 bells
    }

    // Do something with the second array
    break;
  case 2:
    resetButtons();
    bpm = 60;
    inCase6 = false;
    //slow beat all together
    for (int j = 0; j < buttons.size(); ++j)
    {
      //populate the squares
      buttons.get(0).preload(0); //preload hand1 bells
      buttons.get(1).preload(0); //preload hand2 bells
    }
    // Do something with the third array
    break;

  case 3:
    resetButtons();
    bpm = 50;
    inCase6 = false;
    // W rhythm
    for (int j = 0; j < buttons.size(); ++j)
    {
      //populate the squares
      buttons.get(0).preload(3); //preload hand1 bells
      buttons.get(1).preload(4); //preload hand2 bells
    }
    // Do something with the fourth array
    break;

  case 4:
    resetButtons();
    bpm = 60;
    inCase6 = false;
    //high ding
    buttons.get(1).preload(11); //preload hand1 bells
    // Do something with the fourth array
    break;

  case 5:
    resetButtons();
    bpm = 62;
    inCase6 = false;
    //2 different up
    for (int j = 0; j < buttons.size(); ++j)
    {
      //populate the squares
      buttons.get(0).preload(9); //preload hand1 bells
      buttons.get(1).preload(10); //preload hand2 bells
    }
    // Do something with the fourth array
    break;

  case 6:
    bpm = 62;
    //bpm = 35;
    inCase6 = true;
    // freestyle mouse pressed testing ideas here
    break;
  case 7:
    resetButtons();
    bpm = 62;
    inCase6 = false;
    //high ding
    buttons.get(0).preload(1); //preload hand2 bells
    buttons.get(1).preload(2); //preload hand2 bells
    break;
  case 8:
    resetButtons();
    bpm = 62;
    inCase6 = false;
    //high ding
    buttons.get(0).preload(14); //preload hand1 bells
    break;
  case 9:
    resetButtons();
    bpm = 62;
    inCase6 = false;
    //high ding
    buttons.get(0).preload(13); //preload hand1 bells
    break;

  default:
    inCase6 = false;
    break;
  }
}

void resetButtons() {
  for (int i =0; i<7; i++) {
    for (int j = 0; j < buttons.size(); ++j)
    {
      //println("reset");
      //populate the squares
      buttons.get(0).reset(i); //preload hand1 bells
      buttons.get(1).reset(i); //preload hand1 bells
    }
  }

  //might need to add power down or zero setting DMX here . see what it does.
}
