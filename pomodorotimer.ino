/*
 Pomodoro Timer
 
 This sketch counts down to zero, starting at one of the Countdown options.
 Default is 25 minutes, to facilitate a "Pomodoro Session", see http://www.pomodorotechnique.com/ for details.
 The button restarts a session, or, when not having a session selects the countdown value.
 
 What you need:
 - Arduino (I used an Arduino Uno)
 - 7 Segment LED display (I used this one: http://www.dfrobot.com/wiki/index.php?title=SPI_LED_Module_(SKU:DFR0090) )
 - A button (for restarting the timer and setting the countdown value)
 - A piezo buzzer, for the "Timer done" sound
  
 Circuit:
 - 7 segment display connected to pins 3, 8 and 9
 - Button connected to pin 2
 - Speaker connected to pin 7
 
 created 12 aug 2013
 by Rogier van den Berg
 http://www.rogiervandenberg.nl
  
 */


const int buttonPin = 2;    // the number of the pushbutton pin
const int ledPin = 13;      // the number of the LED pin
const int speakerPin = 7;   // Piezo buzzer

/* Possible start point for counting down */
int countdownOptions[] = {5, 15, 25, 45}; //Possible values to count down from
int countdownSetting = 2;                //Default countdown-value is 25 minutes (Pomodoro Technique)

/* Values to prevent the button from bouncing */
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 50;    // the debounce time; increase if the output flickers

/* Values for storing a count down session */
long lastStartTime = 0;    //When did we start
boolean counting = false;  //Are we counting at this moment?
boolean active = true;     //Is the clock active or suspended (active can be "not yet counting, but the button is pressed")

/* Connecting the SPI LED module */

const int latchPin = 8; //Pin connected to latch pin (ST_CP) of 74HC595
const int clockPin = 3; //Pin connected to clock pin (SH_CP) of 74HC595
const int dataPin = 9; //Pin connected to Data in (DS) of 74HC595

/* HEX values that can be put on the display */
byte chars[]={0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90,0xff}; //0-9," "
byte charsWithDot[]={0x40,0x79,0x24,0x30,0x19,0x12,0x02,0x78,0x00,0x10,0x7f}; //0-9," " including dots

void setup() {
  //Initializign pins
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(speakerPin, OUTPUT);
  
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);  
  pinMode(clockPin, OUTPUT);
  
  //Start Serial for debug
  Serial.begin(9600);
  
  //Put 8888 on display to show user the clock is swithed on...
  displayNumber("8888"); 

  //..and play some tones
  playTone(200, 100);
}

void loop() {

 // If active for at least 2 seconds, but not yet counting, start counting
 // This enables the user to select another countdown-value, within 2 seconds
 if (!counting && active && (millis() - lastStartTime) > 2000) {
   Serial.print("GO, ");
   Serial.print(countdownOptions[countdownSetting]);
   Serial.println(" minutes");
   counting = true;
   lastStartTime = millis();
 }
 //Check for button input
 evaluateButton();

 //Make it count
 count();
}

/* This method calculates a new value to show on the display, based on time passed since starting to count */
void count() {
  
  //If we are not counting, do nothing
  if(counting) {
    unsigned long now = millis();
    if (now % 1000 == 0) {
      
      //Calculate the value to display in minutes and seconds, instead of milliseconds.
      int secondsPassed = (now-lastStartTime)/1000;
      int secondsDisplay = ((countdownOptions[countdownSetting] * 60) - secondsPassed) % 60;
      int minutesDisplay = ((countdownOptions[countdownSetting] * 60) - secondsPassed) / 60;
      
      String currentTimeValue = "";
      
      Serial.print(minutesDisplay);
      currentTimeValue += minutesDisplay;
      Serial.print(":");
      if(secondsDisplay < 10) {
        Serial.print("0");
        currentTimeValue += "0";
      }

      //Display the current value (in the Serial connection (for debug) and on the display)
      Serial.println(secondsDisplay);
      currentTimeValue += secondsDisplay;
      displayNumber(currentTimeValue);
      
      //If we are done, (0:00), stop counting, stop being active, play the end tones and suspend the LCD display
      if(secondsDisplay == 0 && minutesDisplay == 0) {
        counting = false;
        active = false;

        playTone(200, 800);

        delay(1000);

        digitalWrite(latchPin, LOW);
        shiftOut(dataPin, clockPin, MSBFIRST, chars[10]);
        shiftOut(dataPin, clockPin, MSBFIRST, chars[10]);
        shiftOut(dataPin, clockPin, MSBFIRST, chars[10]);
        shiftOut(dataPin, clockPin, MSBFIRST, chars[10]);
        shiftOut(dataPin, clockPin, MSBFIRST, chars[10]);
        shiftOut(dataPin, clockPin, MSBFIRST, chars[10]);
        shiftOut(dataPin, clockPin, MSBFIRST, chars[10]);
        shiftOut(dataPin, clockPin, MSBFIRST, chars[10]);
        digitalWrite(latchPin, HIGH);
        
      }
    } 
  }
}

/* Check if the button is pressed and restart the current session (if active and counting) or select the next count down value */
void evaluateButton() {
  int reading = digitalRead(buttonPin);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  } 
  // Check for bouncing button input
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {
       //if active (being handled) but not yet counting down, select the next countdown value
       if (active && !counting) {
          
          if(countdownSetting++ == (sizeof(countdownOptions) / sizeof(int)) - 1) {
            countdownSetting = 0;
          }
        }
        // Stop current counting action (because we were pressing the button) and make sure we are awake (to 
        counting = false;
        active = true; // Wake up!
        
        //Display currently set countdown value
        Serial.println(countdownOptions[countdownSetting]);
        displayNumber((String)countdownOptions[countdownSetting]);
        lastStartTime = millis();
      }
    }
  }
  lastButtonState = reading;
}

/* Write out the numbers to the LCD Display */
void displayNumber(String line)
{
  if(line.length() > 0) {
    //Serial.println("Verwerken:" + line);

    digitalWrite(latchPin, LOW);

    //Putting up the online value
    for(int i=line.length()-1; i >= 0; i--)
    {
      int bitToSet = (byte)line.charAt(i) - 48;
      if(i == line.length()-3) { //Dot after second number
        shiftOut(dataPin, clockPin, MSBFIRST, charsWithDot[bitToSet]);
      } else {
       shiftOut(dataPin, clockPin, MSBFIRST, chars[bitToSet]);
      }
    }
    
    //Putting up the leading zeroes
    for(int i=0; i<8-line.length(); i++)
    {
      shiftOut(dataPin, clockPin, MSBFIRST, chars[10]);
    }
    
    digitalWrite(latchPin, HIGH);
  }
 }
 
 /* Play a tone */
 void playTone(int tone, int duration) {
  for (long i = 0; i < duration * 1000L; i += tone * 2) {
    digitalWrite(speakerPin, HIGH);
    delayMicroseconds(tone);
    digitalWrite(speakerPin, LOW);
    delayMicroseconds(tone);
  }
}
