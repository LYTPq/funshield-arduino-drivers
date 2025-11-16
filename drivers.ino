#include "funshield.h"

constexpr int diodePins[] = { led1_pin, led2_pin, led3_pin, led4_pin };
constexpr int buttonPins[] = { button1_pin, button2_pin, button3_pin };
constexpr int diodesCount = sizeof(diodePins) / sizeof(diodePins[0]);
constexpr int buttonsCount = sizeof(buttonPins) / sizeof(buttonPins[0]);
constexpr int digitsCount = sizeof(digits) / sizeof(digits[0]);

constexpr int numPositions = 4;         

constexpr int emptyGlyph = 0xff;
constexpr int dotGlyph = 0b01111111;

constexpr int dotPosition = 1;

int power(int value, int exponent) {
  int result = 1;
  for (int i = 0; i < exponent; i++) {
    result *= value;
  }
  return result;            
}


constexpr int timeFormatExponent = 3 - dotPosition;

const int counterLimit = power(digitsCount, numPositions);
const unsigned long timeFormat = power(digitsCount, timeFormatExponent);    

constexpr byte LETTER_GLYPH[] {     

  0b10001000,   // A                    
  0b10000011,   // b             
  0b11000110,   // C               
  0b10100001,   // d                  
  0b10000110,   // E
  0b10001110,   // F
  0b10000010,   // G
  0b10001001,   // H
  0b11111001,   // I
  0b11100001,   // J
  0b10000101,   // K
  0b11000111,   // L
  0b11001000,   // M
  0b10101011,   // n
  0b10100011,   // o
  0b10001100,   // P
  0b10011000,   // q
  0b10101111,   // r
  0b10010010,   // S
  0b10000111,   // t
  0b11000001,   // U
  0b11100011,   // v
  0b10000001,   // W
  0b10110110,   // ksi
};

constexpr byte EMPTY_GLYPH = 0b11111111;

class Diode {
private:
  int diodeNumber_;
  bool currentState_;

public:
  void initialize(int diodeNumber) {
    diodeNumber_ = diodeNumber;
    pinMode(diodePins[diodeNumber], OUTPUT);
    change(false);
  }

  void change(bool newState) {
    digitalWrite(diodePins[diodeNumber_], newState ? LOW : HIGH);
    currentState_ = newState;
  }
  void change() {
    change(!currentState_);
  }

  bool returnState() {
    return currentState_;
  }
};


Diode diodes[diodesCount];


class Timer {
private:
  unsigned long lastTime_;
  unsigned long interval_;
public:

  void initialize(unsigned long interval, unsigned long currentTime) {
    interval_ = interval;
    lastTime_ = currentTime;
  }


  void changeInterval(unsigned long interval) {
    interval_ = interval;
  }


  bool start(unsigned long currentTime) {
    if (currentTime - lastTime_ >= interval_) {
      lastTime_ += interval_;
      return true;
    }
    return false;
  }
};

Timer timer;


class Button {
private:
  int buttonNumber_;
  bool currentState_;
  Timer timer_;


public:

  void timerInitialize(unsigned long interval, unsigned long currentTime) {  //in order to make timer object inside button private
    timer_.initialize(interval, currentTime);
  }

  void initialize(int buttonNumber) {
    buttonNumber_ = buttonNumber;
    pinMode(buttonPins[buttonNumber_], INPUT);
  }

  bool pressed() {
    return digitalRead(buttonPins[buttonNumber_]) == LOW;
  }

  bool triggered() {
    bool newState = pressed();
    if (currentState_ != newState) {
      currentState_ = newState;
      return newState;
    }
    return false;
  }

  bool events(unsigned long waitingInterval, unsigned long incrementInterval, unsigned long currentTime) {
    if (triggered()) {
      timer_.initialize(waitingInterval, currentTime);  //pressed one time
      return true;
    }

    else if (currentState_ && timer_.start(currentTime)) {  //being held
      timer_.changeInterval(incrementInterval);
      return true;
    }

    return false;
  }
};


Button buttons[buttonsCount];




class Display {
public:
  void initialize(){
    pinMode(latch_pin, OUTPUT);
    pinMode(data_pin, OUTPUT);
    pinMode(clock_pin, OUTPUT);
    clear();
  } 

  void showGlyph(byte glyphMask, byte positionMask){
    digitalWrite(latch_pin, LOW);
    shiftOut(data_pin, clock_pin, MSBFIRST, glyphMask);
    shiftOut(data_pin, clock_pin, MSBFIRST,positionMask);
    digitalWrite(latch_pin, HIGH);
  }    

  void clear() {
    byte binaryPosition = ((1 << numPositions) - 1);
    showGlyph(emptyGlyph, binaryPosition);   
  } 


  void showDigit(int digit, int position){
    // byte binaryPosition = (1 << position);
    byte positionMask = binaryPosition(position);
    showGlyph(digits[digit], positionMask);
  }


  byte binaryPosition(int position){
    return (1 << (numPositions - 1 - position));
  }
   
};

Display display;



class NumericDisplay : public Display {
private:
  int number_;
  int dotPosition_;
  int currentPosition_;
  int highestOrdinal_;
  bool triggered_;
  bool isInteger_;


  void highestOrder() {
    highestOrdinal_ = numPositions - 1; 

    while (highestOrdinal_ > 0){
      int currentDigit = (number_ / power(digitsCount, highestOrdinal_)) % digitsCount;

      if (currentDigit == 0 && (isInteger_ || highestOrdinal_ != dotPosition_)){
      --highestOrdinal_;
      }

      else {
        break;
      }
    }  
  }

public:
  void initialize() {
    Display::initialize();
    triggered_ = false;
    clear();
  }


  void setNumber(int newNumber) {
    number_ = newNumber;
    triggered_ = true;
    isInteger_ = true;
    currentPosition_ = 0;
    highestOrder();
  }

  void setNumber(int newNumber, int newDotPosition) {
    number_ = newNumber;
    triggered_ = true;
    isInteger_ = false;
    dotPosition_ = newDotPosition;
    currentPosition_ = 0;
    highestOrder();
  }

  void showDigit(int digit, int position, bool isDecimal){
    byte positionMask = binaryPosition(position);
    byte glyphDigit = digits[digit];
    if (isDecimal){
      glyphDigit &= dotGlyph;
    }
    showGlyph(glyphDigit, positionMask);
  }
  
  void update() {

      int currentDigit = (number_ / power(digitsCount, currentPosition_)) % digitsCount;

      if (currentPosition_ > highestOrdinal_) {
        showGlyph(emptyGlyph, binaryPosition(currentPosition_));
        
      } else {
        bool dot = (!isInteger_ && currentPosition_ == dotPosition_);
        showDigit(currentDigit, currentPosition_, dot);
        }
      currentPosition_ = (currentPosition_ + 1) % numPositions;
  }

  void deactivate() {
    triggered_ = false;
    clear();
  }
};

NumericDisplay numericDisplay;



class TextDisplay : public Display {
private:
  char currentText_[numPositions];     
  int currentPosition_ = 0;
public:
  void initialize() {
    Display::initialize();
  }
  void setText(const char* inputString){       
    const char* sourcePointer = inputString;
    char* targetPointer = currentText_;
    for (int i = 0; i < numPositions; ++i) {
      if (*sourcePointer != '\0'){
        *targetPointer++ = *sourcePointer++;
      }else{
        *targetPointer++ = ' ';
      }
    }
  } 

  void update(){
    showChar(currentText_[currentPosition_], spaceOffset - currentPosition_);
    currentPosition_ = (currentPosition_ + 1) % numPositions;
  }
};


TextDisplay textDisplay;


SerialInputHandler serialInput;




class Stopwatch{
private:
  enum class stopwatchStates {STOPPED, RUNNING, LAPPED};
  stopwatchStates currentState_ = stopwatchStates::STOPPED;
  unsigned long startTime_;
  unsigned long elapsedTime_;
  unsigned long lappedTime_;
  unsigned long previousDisplayTime_;
  unsigned long displayTime_;

public:
  void initialize(){
    previousDisplayTime_ = 0;
    numericDisplay.setNumber(0, dotPosition);
    currentState_ = stopwatchStates::STOPPED;
  }
  

  void start(unsigned long currentTime){
    startTime_ = currentTime; 
    currentState_ = stopwatchStates::RUNNING;
  }

  void stop(unsigned long currentTime){
    elapsedTime_ += currentTime - startTime_;
    currentState_ = stopwatchStates::STOPPED;
  }

  void reset() {
    elapsedTime_ = 0;
    currentState_ = stopwatchStates::STOPPED;
  }

  void lapFreeze(int currentTime){
    lappedTime_ = displayTime_;                   // save current time 
    currentState_ = stopwatchStates::LAPPED;
  }

  void lapUnfreeze(){
    currentState_ = stopwatchStates::RUNNING;
  }

  void update(unsigned long currentTime) {
    if (currentState_ != stopwatchStates::STOPPED) {
      displayTime_ = (elapsedTime_ + (currentTime - startTime_)) / timeFormat;        

      if (currentState_ == stopwatchStates::LAPPED){
      setTime(lappedTime_);                                      
      }else{
        setTime(displayTime_);
      }

    } else if (currentState_ == stopwatchStates::STOPPED){
      displayTime_ = elapsedTime_  / timeFormat;
      setTime(displayTime_);
    }

  }

  void buttonHandler(unsigned long currentTime) {
    if (buttons[startStopButton].triggered() && currentState_ != stopwatchStates::LAPPED) {
      if (currentState_ == stopwatchStates::STOPPED) {
        start(currentTime);

      }else{                            
        stop(currentTime); 
      }
    }
    else if (buttons[resetButton].triggered() && currentState_ == stopwatchStates::STOPPED){
      reset();
    }
    else if (buttons[lapButton].triggered() && currentState_ != stopwatchStates::STOPPED){
      if (currentState_ == stopwatchStates::RUNNING){
        lapFreeze(currentTime);
      }else {
        lapUnfreeze();
      }
    }
  }

  void setTime(unsigned long displayTime){
    if (displayTime != previousDisplayTime_) {
      numericDisplay.setNumber(displayTime, dotPosition);  
      previousDisplayTime_ = displayTime;
    }

  }
};

Stopwatch stopwatch;



class Counter {
private:
  int value_;
  bool binary_[diodesCount];
  int currentLedPosition_ = 0;
public:

  void initialize(int initialValue = 0) {
    value_ = initialValue;
  }

  void changePosition() {
    currentLedPosition_ = (currentLedPosition_+ 1) % numPositions;
  }

  int currentPosition(){
    return currentLedPosition_;
  }

  int digitOnPosition(){
    return (value_ / power(digitsCount, currentLedPosition_)) % digitsCount;
  }

  void increment() {
    value_ = (value_ + power(digitsCount, currentLedPosition_)) % counterLimit;
  }

  void decrement() {
    value_ = (value_ + (counterLimit - power(digitsCount, currentLedPosition_))) % counterLimit;
  }

  // void getBinary() {
  //   for (int i = 0; i < diodesCount; i++) {
  //     binary_[diodesCount - 1 - i] = ((value_ & (1 << i)) != 0);  // convert an integer into binary, uses bool array
  //   }
  // }

  // void displayChanges() {

  //   getBinary();  // display binary number on diodes

  //   for (int i = diodesCount - 1; i >= 0; i--) {
  //     if (diodes[i].returnState() != binary_[i]) {
  //       diodes[i].change(binary_[i]);
  //     }
  //   }
  // }

};

Counter counter;


class RunningMessage {
private:
  const char* messagePointer_;
  const char* charPointer_;
  bool finished_;
  Timer timer_;
  char displayMessage_[numPositions];
  int numSpaces_;

public:
  void initialize(unsigned long currentTime) {
    messagePointer_ = serialInput.getMessage();  
    charPointer_ = messagePointer_;
    timer_.initialize(textDisplayInterval, currentTime);
    finished_ = true;
  }


  void update(unsigned long currentTime) {
    if (timer_.start(currentTime)) {
      if (finished_) {
        messagePointer_ = serialInput.getMessage();
        charPointer_ = messagePointer_;
        finished_ = false;
        numSpaces_ = spaceOffset;
      }


      const char* sourcePointer = charPointer_;
      int currentNumSp = numSpaces_;

      for (int i = 0; i <= spaceOffset; ++i) {
        if (currentNumSp > 0) {
          displayMessage_[i] = ' ';
          --currentNumSp;
        }else if (*sourcePointer != '\0') {
          displayMessage_[i] = *sourcePointer++;
        } else {
          displayMessage_[i] = ' ';
        }
      }

      --numSpaces_;
      
      if (numSpaces_ < 0) {
        charPointer_++; 
      } 
      textDisplay.setText(displayMessage_);
      checkFinished();
    }

  }

  void checkFinished(){
    bool allSpaces = true;
    for (int i = 0; i < numPositions; ++i) {
      if (displayMessage_[i] != ' ') {
        allSpaces = false;
        break;
      }
    }
      finished_ = allSpaces;
  }


};

RunningMessage runningMessage;

unsigned long currentTime;


void setup() {


}



void loop() {
  currentTime = millis();

}


