#include <LiquidCrystal.h>
//LiquidCrystal lcd(7, 6, 5, 4, 3, 2);
#define DEBOUNCE 20  // button debouncer, how many ms to debounce, 5+ ms is usually plenty
#define MIDI_CC_GENERAL1 0x0E
#define MIDI_CC MIDI_CC_GENERAL1

#define BUTTON_START_NOTE 65
#define POT_NOTE 64

// here is where we define the buttons that we'll use. button "1" is the first, button "6" is the 6th, etc
//byte buttons[] = {4, 9, 12, A0}; // the analog 0-5 pins are also known as 14-19
byte buttons[] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 12, A2}; // the analog 0-5 pins are also known as 14-19
// This handy macro lets us determine how big the array up above is, by checking the size
#define NUMBUTTONS sizeof(buttons)
// we will track if a button is just pressed, just released, or 'currently pressed' 
volatile byte pressed[NUMBUTTONS], justpressed[NUMBUTTONS], justreleased[NUMBUTTONS];

const int analogInPin0 = A0;  // Analog input pin that the potentiometer is attached to
const int analogInPin1 = A1;
const int potSwitchPin = 2;

int sensorValue1 = 0;        // value read from the pot
int sensorValue2 = 0;
int newSensorValue1, newSensorValue2;
int potValue1, potValue2;

int switchState;             // the current reading from the input pin
int lastSwitchState = LOW;   // the previous reading from the input pin

long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 50;    // the debounce time; increase if the output flickers

unsigned long time;



int noteToPrint;

void controlChange(byte channel, byte control, byte value)
{
  // 0xB0 is the first of 16 control change channels. Subtract one to go from MIDI's 1-16 channels to 0-15
  channel += 0xB0 - 1;
  
  // Ensure we're between channels 1 and 16 for a CC message
  if (channel >= 0xB0 && channel <= 0xBF)
  {
      Serial.write(channel);
      Serial.write(control);
      Serial.write(value);
  }
}

void setup() {
   //  lcd.begin(16, 2);
  byte i;
  
  // set up serial port
  Serial.begin(57600);
   pinMode(potSwitchPin, INPUT);
  
   sensorValue1 = analogRead(analogInPin0);
   sensorValue2 = analogRead(analogInPin1);
 // Serial.print("Button checker with ");
  //Serial.print(NUMBUTTONS, DEC);
  //Serial.println(" buttons");
 
  // Make input & enable pull-up resistors on switch pins
  for (i=0; i < NUMBUTTONS; i++)
  { pinMode(buttons[i], INPUT);
    digitalWrite(buttons[i], HIGH);
  }

  // Run timer2 interrupt every 15 ms 
  TCCR2A = 0;
  TCCR2B = 1<<CS22 | 1<<CS21 | 1<<CS20;

  //Timer2 Overflow Interrupt Enable
  TIMSK2 |= 1<<TOIE2;

}

SIGNAL(TIMER2_OVF_vect) {
  check_switches();
}

void check_switches()
{
  static byte previousstate[NUMBUTTONS];
  static byte currentstate[NUMBUTTONS];
  static long lasttime;
  byte index;

  if (millis() < lasttime){ // we wrapped around, lets just try again
     lasttime = millis();
  }
  
  if ((lasttime + DEBOUNCE) > millis()) {
    // not enough time has passed to debounce
    return; 
  }
  // ok we have waited DEBOUNCE milliseconds, lets reset the timer
  lasttime = millis();
  
  for (index = 0; index < NUMBUTTONS; index++){
    currentstate[index] = digitalRead(buttons[index]);   // read the button
    
    /*     
    Serial.print(index, DEC);
    Serial.print(": cstate=");
    Serial.print(currentstate[index], DEC);
    Serial.print(", pstate=");
    Serial.print(previousstate[index], DEC);
    Serial.print(", press=");
    */
    
    if (currentstate[index] == previousstate[index]) {
      if ((pressed[index] == LOW) && (currentstate[index] == LOW)) {
          // just pressed
          justpressed[index] = 1;
      }
      else if ((pressed[index] == HIGH) && (currentstate[index] == HIGH)) {
          // just released
          justreleased[index] = 1;
      }
      pressed[index] = !currentstate[index];  // remember, digital HIGH means NOT pressed
    }
    //Serial.println(pressed[index], DEC);
    previousstate[index] = currentstate[index];   // keep a running tally of the buttons
  }
}


void loop() {
  
 
  for (byte i = 0; i < NUMBUTTONS; i++){
  
    if (justpressed[i]) {
      justpressed[i] = 0;
     // Serizal.print(i, DEC);
     // Serial.println(" Just pressed"); 
      // remember, check_switches() will CLEAR the 'just pressed' flag
       
    
        //Serial.println(i);
        
      
        Serial.write(0x90);
        Serial.write(BUTTON_START_NOTE + i);
      //  noteToPrint =  52 + i;      
        //Serial.write(127);
        Serial.write(0x64);

      
    }
    if (justreleased[i]) {
      justreleased[i] = 0;
        Serial.write(0x80);
        Serial.write(BUTTON_START_NOTE + i);
      //  noteToPrint =  52 + i;      
        //Serial.write(127);
        Serial.write(0x00);
      //Serial.print(i, DEC);
      //Serial.println(" Just released");
      // remember, check_switches() will CLEAR the 'just pressed' flag
    }
    if (pressed[i]) {
     // Serial.print(i, DEC);
      //Serial.println(" pressed");   
      // is the button pressed down at this moment
    }
  }

   /**************************** POT 1 ***************************************/
  newSensorValue1 = analogRead(analogInPin0);
  if(newSensorValue1 != sensorValue1){
     if(newSensorValue1 > sensorValue1 + 3 || newSensorValue1 < sensorValue1 - 3){
      sensorValue1 = newSensorValue1;
         potValue1 = map(newSensorValue1, 0, 1023, 0, 128);
     /*  Serial.write(144);
       Serial.write(199);
       Serial.write(potValue1);}
    */
    controlChange(1, MIDI_CC + 1, potValue1);
    }
  }
  /**************************************************************************/

  int reading = digitalRead(potSwitchPin);

   if (reading != lastSwitchState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  /**************************** POT 2 ***************************************/
  
  newSensorValue2 = analogRead(analogInPin1);
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != switchState) {
      switchState = reading;
          if (switchState == LOW) {
               Serial.write(0x90); //144
               Serial.write(POT_NOTE); //E4
               Serial.write(0x64);
             
          }else{
               Serial.write(0x80); //144
               Serial.write(POT_NOTE); //E4
               Serial.write(0x00);
            }
    }
  }
  lastSwitchState = reading;
   if(newSensorValue2 != sensorValue2){
                 if(newSensorValue2 > sensorValue2 + 3 || newSensorValue2 < sensorValue2 - 3){
                  sensorValue2 = newSensorValue2;
                     potValue2 = map(newSensorValue2, 0, 1023, 0, 128);
                 /*  Serial.write(144);
                   Serial.write(199);
                   Serial.write(potValue1);}
                */
                controlChange(2, MIDI_CC + 2, potValue2);
                }
              }
   /**************************************************************************/
  delay(2);

}


