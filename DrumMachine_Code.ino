//Memory
#include <EEPROM.h>

//LCD
#include <LiquidCrystal_I2C.h>  /*include LCD I2C Library*/
LiquidCrystal_I2C lcd(0x27,16,2);  /*I2C scanned address defined + I2C screen size*/
boolean isDisplaying = false;
int pointerCoordinate = 0;
bool pointerOnLCD = false;
unsigned long pointerTime;
int drum_select;

//Keyboard
#include <Keypad.h>
const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
  {'#','*','0'},
  {'9','7','8'},
  {'6','4','5'},
  {'3','1','2'}
};
byte rowPins[ROWS] = {5, 4, 3, 2}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {8, 7, 6}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

unsigned long specialButton1Timer;
unsigned long specialButton2Timer;
unsigned long saveSlotsButtonTimer;
int saveSlotKey = -1;


///////////////
//MIDI and Sync
bool isPlaying = false;
int bpm = 140;
int swing = 0;

int drumVolume[7];
bool muted[7];
//int drumsMidi[7] = {36, 52, 54, 57, 58, 62, 54};
int drumsMidi[7] = {39, 52, 54, 57, 58, 62, 54};

int syncOutPin = 9;
bool isEveryOther = false;
unsigned long timeSyncOut, timeDrumMachine;
int whichQuarterNote = 0;

//Pattern
char drumPattern[7][16] = {    
                        {'#', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '}, //kick
                        {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '}, //snare
                        {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '}, //clap
                        {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '}, //closed hi-hat
                        {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '}, //open hi-hat
                        {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '}, //riddle 
                        {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '}  //maracas
                         };


void setup() {
  lcd.init();  /*LCD display initialized*/
  lcd.clear();     /*Clear LCD Display*/
  lcd.backlight();      /*Turn ON LCD Backlight*/
  pointerTime = millis();

  pinMode(syncOutPin, OUTPUT);
  //Serial for midi communication
  Serial.begin(31250);
  timeSyncOut = timeDrumMachine = millis();

  //append mute table with false - none of the drums are muted when device is powered up 
  for(int i = 0; i < 7; i++) {
    muted[i] = false;
    drumVolume[i] = 100;
  }
}
/*
/This function will print on LCD pattern for choosed drum.
/Example 'techno' kick 4x4:
//////////////////
/#   #   #   #   /
/|kck|= m|bpm|140/
//////////////////
/where # is note on, blank characters between are no note.
*/

void printDrumPattern(int drumChoice) { 
  lcd.clear();
  char *DrumsNames[] = {"kck", "snr", "clp", "cld", "ope", "rid", "mar"};

  for(int i = 0; i < 16; i++) {
    lcd.setCursor(i,0);
    lcd.print(drumPattern[drumChoice][i]);
  } 
 
  for(int i = 0; i < 16; i += 4) {
    lcd.setCursor(i,1);
    lcd.print("|"); 
  }

  lcd.setCursor(1,1);
  lcd.print(DrumsNames[drumChoice]);

  drawBPM();

  lcd.setCursor(5,1);
  if(isPlaying) lcd.print(">"); 
  else lcd.print("="); 
  
  lcd.setCursor(7,1);
  if(muted[drumChoice]) lcd.print("m"); 
  else lcd.print(" "); 

  
}

void drawBPM(){
  lcd.setCursor(9,1);
  lcd.print("BPM"); 
  lcd.setCursor(13,1);
  lcd.print(bpm);
  if(bpm < 10){
  lcd.setCursor(14,1);
  lcd.print("  ");
  } 
  else if(bpm < 100){
  lcd.setCursor(15,1);
  lcd.print(" ");
  }
}

void updateDrumPatternList(char x, int p, int ds){
    drumPattern[ds][p] = x;
}


void loop() {
  //display kick pattern as defualt after turning the device on
  if(!isDisplaying) {
    printDrumPattern(0);
    lcd.setCursor(5,1);
    lcd.print("="); 
    isDisplaying = true;
  }

 
  char key = keypad.getKey();
  unsigned long currentTime = millis();

  
  //start stop 0 button
  if(key == '0'){
    lcd.setCursor(5,1);
    if(!isPlaying){ 
      isPlaying = true;
      lcd.print(">"); 
      }

    else{
      isPlaying = false;
      lcd.print("="); 
      //set to 0, so after stop, and play, drum machine will play till first note in pattern
      whichQuarterNote = 0;
    }
  }

  //change displayed drum on LCD with keys 2 and 8
  if (key == '2'){
    if(drum_select < 6) drum_select++;
    else drum_select = 0;
    printDrumPattern(drum_select);
    }

  if (key == '8'){
    if(drum_select > 0) drum_select--;
    else drum_select = 6;
    printDrumPattern(drum_select);
    }

  //Assign note on or off 
  if (key == '5'){
    char noteSign = drumPattern[drum_select][pointerCoordinate];
    lcd.setCursor(pointerCoordinate, 0);
    char sign;

    if (noteSign == '#'){
      drumPattern[drum_select][pointerCoordinate] = ' ';
      sign = ' ';
    }

    else{
      drumPattern[drum_select][pointerCoordinate] = '#';
      sign = '#';
    }
    updateDrumPatternList(sign, pointerCoordinate, drum_select);
    lcd.print(drumPattern[drum_select][pointerCoordinate]); //update screen
  }  

  //change pointer position with keys 4 and 6:
  if (key == '4' || key == '6'){
    //to ensure that the pointer will not stay printed on screen
    //printing new screen create LCD blink
    lcd.setCursor(pointerCoordinate, 0);
    lcd.print(drumPattern[drum_select][pointerCoordinate]);
    if(key == '4'){
      if(pointerCoordinate > 0) pointerCoordinate--;
      else pointerCoordinate = 15;
    }

    if (key == '6'){
      if(pointerCoordinate < 15) pointerCoordinate++;
      else pointerCoordinate = 0;
    }
  }

    //volume down selected drum
  if(key == '1' && isPlaying){
    if(drumVolume[drum_select] - 10 >= 10) drumVolume[drum_select] -= 10; 
  }

  //volume up selected drum
  if(key == '3' && isPlaying){
    if(drumVolume[drum_select] + 10 <= 120) drumVolume[drum_select] += 10; 
  }

  //mute/unmute drum
  if(key == '*' && isPlaying){    
    lcd.setCursor(7,1);
    if(muted[drum_select]) {
      muted[drum_select] = false;
      lcd.print(" ");
    }
    else {
      muted[drum_select] = true; 
      lcd.print("m");
    }
  }

  currentTime = millis();

  //Blinking pointer part:
  //pointer will blink at 650 miliseconds interval
  if(currentTime - pointerTime >= 650 ){
    lcd.setCursor(pointerCoordinate, 0);
    pointerTime = millis();
    if(!pointerOnLCD){
      lcd.print('_');
      pointerOnLCD = true; 
    }
    else{
      lcd.print(drumPattern[drum_select][pointerCoordinate]);
      pointerOnLCD = false; 
    }
  }  

  //Special buttons # and * options:
  if(key == '#' && !isPlaying){
    specialButton1Timer = millis();
    saveSlotKey = -1;
  }

  if(key == '*' && !isPlaying) specialButton2Timer = millis();

  int keyInt = -1;
  if(key){
    if(key != '#' || key != '*') keyInt = key - '0';
    }

  if(!isPlaying){
    if(keyInt < 7 && keyInt >= 0){ 
      saveSlotKey = keyInt;
      saveSlotsButtonTimer = millis();
    }
  }

  //saving pattern - press [#] button and then from 1 to 6 on numpad, to pick slot
  if(specialButton1Timer && currentTime - specialButton1Timer <= 5000 && !isPlaying){
    if(saveSlotKey != -1 && currentTime - saveSlotsButtonTimer <= 3000 ){
      saveSlotsButtonTimer = specialButton1Timer = 0;
      savePattern(saveSlotKey);
      saveSlotKey = -1;
    }
  }

  //loading Pattern - press [*] button and then from 1 to 6 on numpad, to pick slot
  if(specialButton2Timer && currentTime - specialButton2Timer <= 5000 && !isPlaying){
    if(saveSlotKey != -1 && currentTime - saveSlotsButtonTimer <= 3000 ){
      saveSlotsButtonTimer = specialButton2Timer = 0;
      loadPattern(saveSlotKey);
      saveSlotKey = -1;
    }
  }

  //read bpm from potetiometer 1
  int readPotentiometer1 = scalePotentiometerReadBPM(analogRead(A7)); // BPM from potentiometer

  

  if (readPotentiometer1 != bpm){
    if (readPotentiometer1 - bpm > 3 ||  bpm - readPotentiometer1 > 3){
      bpm = readPotentiometer1;
      //Update BPM value on the screen
      drawBPM();
    }
  }

 
  //read swing from potetiometer 2
  int readPotentiometer2 = analogRead(A3);
  readPotentiometer2 = scalePotentiometerReadSwing(readPotentiometer2); // Swing from potentiometer
  if (readPotentiometer2 != swing){
    if(readPotentiometer2 - swing > 3 ||  swing - readPotentiometer2 > 3) swing = readPotentiometer2;

  }

  //////////////////////////
  //////////////////////////
  //Playing and Syncin' Part

  int x, y;
  x = y = countBPM(bpm);

  //send sync out via jack connected to pin D9
  unsigned long current_time = millis();
  if(current_time >= timeSyncOut + y){
    //send2ppq();
    timeSyncOut = current_time;
  }
  
  x /= 2; //BPM is not working good here, to be figured.
  //swing - every other note is playig slighty faster
  if(!isEveryOther) x += swing; 
  else x -= swing;  


  //Sending signal via MIDI cable connected to serial pin tx.
  //Turning note On
  current_time = millis();

  if(current_time >= timeDrumMachine + x && isPlaying){
    send2ppq();
    play_drums();
    timeDrumMachine = current_time;
    if(whichQuarterNote < 15) whichQuarterNote++;
    else whichQuarterNote = 0;
    if(!isEveryOther) isEveryOther = true;
    else isEveryOther = false;

  }
}



void play_drums(){
  for(int i = 0; i < 7; i++){
    bool ax = true;
    if(!muted[i] && drumPattern[i][whichQuarterNote] == '#') {
      noteOn(0x90, drumsMidi[i], drumVolume[i]);  
    }
  }
}



int countBPM(int bpm) {
  float y = (60.0 * 1000) / bpm;
  y /= 2;
  int x = (int)y;
  return x;
}


int scalePotentiometerReadBPM(int potRead){
  // Linear transform
  potRead = (float(potRead - 1) * (300.0 - 71.0) / (920.0 - 1.0)) + 71.0;
  potRead = int(potRead);
  return potRead;
}


int scalePotentiometerReadSwing(int potRead){
  // Linear transform
  potRead = float(potRead) / 10.0;
  potRead = int(potRead);
  potRead /= 2; //adjust swing to correct measured subiectivly level 
  return potRead;
}


void noteOn(int cmd, int pitch, int velocity) {
  Serial.write(cmd);
  Serial.write(pitch);
  Serial.write(velocity);
}


void send2ppq(){
  //Sending sync audio out of jack connected to D9 
  //2ppq
  for(int i = 0; i < 2; i++){
    digitalWrite(syncOutPin, HIGH);
    delayMicroseconds(1);
    digitalWrite(syncOutPin, LOW);
    delayMicroseconds(1);
  } 
}


//Save and Load pattern to EEPROM:
//6 save slots, so 6 pattern could be stored

void savePattern(int savingSlot){
  /*
  int address;
  address = 0 + (savingSlot * 112);
  for(int i = 0; i < 7 ;i++){
    for(int j = 0; j < 16; j++){
      EEPROM.put(address, drumPattern[i][j]);
      address++;
    }
  }
  */
  //display info about save:
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Pattern has been");
  lcd.setCursor(0,1);
  lcd.print("saved on slot");
  lcd.setCursor(14,1);
  lcd.print(savingSlot);
  delay(4000);
  isDisplaying = false;

}


void loadPattern(int savingSlot){
  int address;
  address = 0 + (savingSlot * 112);
  if(EEPROM.read(address) != 255){
    for(int i = 0; i < 7 ;i++){
      for(int j = 0; j < 16; j++){
        char valueRead;
        EEPROM.get(address, valueRead);
        drumPattern[i][j] = valueRead;
        address++;
      }
    }
  //display info about load:
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Pattern loaded");
  lcd.setCursor(2,1);
  lcd.print("from slot");
  lcd.setCursor(12,1);
  lcd.print(savingSlot);
  delay(4000);
  isDisplaying = false;
  }
  else {
  lcd.clear();
  lcd.setCursor(5,0);
  lcd.print("Slot");
  lcd.setCursor(10,0);
  lcd.print(savingSlot);
  lcd.setCursor(3,1);
  lcd.print("is empty!");
  delay(4000);
  isDisplaying = false;
  }
}
