#include <FastLED.h>

#define LED_DATA_PIN 6
#define NUM_LEDS 6
#define LED_MESSAGE_PREFIX_BYTE 17
#define RESET_MESSAGE_BYTE 89

enum MESSAGE_STATUS{ NOT_STARTED, LED_NUMBER, RED_VALUE, GREED_VALUE, BLUE_VALUE, CHECK_SUM, ERROR_MESSAGE};  

const int AxPin = A0;  
const int AyPin = A2;  
const int buttonPin = 3; 

int xValue, yValue, buttonState;
CRGB leds[NUM_LEDS];
unsigned long last_message_send = 0;

MESSAGE_STATUS messageState = NOT_STARTED;
int currentLed = -1, red, green, blue;

void setup() { //setup the system
  Serial.begin(9600); //establish a serial communication
  pinMode(buttonPin, INPUT_PULLUP); //set buttonPin as input with pullup resistor
  FastLED.addLeds<WS2811, LED_DATA_PIN>(leds, NUM_LEDS); //set FastLED to work with ws2811 and LED_Data_PIN
  FastLED.show(); 
}

void loop() {
   updateJoystick(); //send joystick condition via serial
   updateLeds(); //check for commands for the leds
}

void updateJoystick(){
  unsigned long now = millis();
  if(now - last_message_send < 100)
    return;
  
  int Ax = analogRead(AxPin); //read values
  int Ay = analogRead(AyPin);
  buttonState = digitalRead(buttonPin);

  xValue = map(Ax, 0, 1023, 0, 255); 
  yValue = map(Ay, 0, 1023, 0, 255);
  
  Serial.print(xValue); //send via serial
  Serial.print(" ");
  Serial.print(yValue);
  Serial.print(" ");
  Serial.println(buttonState);
  
  last_message_send = millis();
}

void updateLeds(){
  bool problem=false;
  while (Serial.available() > 0 && messageState != ERROR_MESSAGE)
  {
      switch(messageState) //message in the protocol
      {
        case NOT_STARTED:
          not_started_fucn();
          break;
        case LED_NUMBER:
          led_number_func();
          break;
        case RED_VALUE:
          red = Serial.read();       
          messageState = GREED_VALUE;
          break;
        case GREED_VALUE:
          green = Serial.read();       
          messageState = BLUE_VALUE;
          break;
        case BLUE_VALUE:
          blue = Serial.read();       
          messageState = CHECK_SUM;
          break;
        case CHECK_SUM:
          check_sum_func();
          break;
      }
  }
  if(messageState == ERROR_MESSAGE)
    error_func();
}

void not_started_fucn(){ //start led command
  int tempByte = Serial.read();
  if(tempByte == LED_MESSAGE_PREFIX_BYTE)
    messageState = LED_NUMBER;
  else if(tempByte == RESET_MESSAGE_BYTE)
  {
    FastLED.clear();
    FastLED.show(); 
  }
  else {
    messageState = ERROR_MESSAGE;
    Serial.print("message start");
  }
}

void led_number_func(){ //set led number
  currentLed = Serial.read();
  if(currentLed < 0 || currentLed >= NUM_LEDS)
  {
    messageState = ERROR_MESSAGE;
    Serial.print("current led - ");
    Serial.println(currentLed);
  }
  else
    messageState = RED_VALUE;
}

void check_sum_func(){ //verify checksum
  int checkSum = (currentLed + red + green + blue + LED_MESSAGE_PREFIX_BYTE) % 256;
  if(Serial.read() != checkSum)
  {
      messageState = ERROR_MESSAGE;
      Serial.println("checksum");
  }
  else
  {
      leds[currentLed] = CRGB( red, green, blue);
      FastLED.show(); 
      messageState = NOT_STARTED;
  }
}

void error_func(){ //error handler
  delay(20);
  
  while (Serial.available())
  {
    Serial.read();
    delay(1);
  }
  messageState = NOT_STARTED;
}


