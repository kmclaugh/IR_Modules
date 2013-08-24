/*
  Sends out given IR codes through an IR LED using the PWM Library
 */

#include <PWM.h>

int led = 9;                // the pin that the LED is attached to
#define brightness 127         // brightness for 50% duty cycle
int32_t frequency = 38; //frequency (in Hz)

void setup()
{
  //initialize all timers except for 0, to save time keeping functions
  InitTimersSafe(); 

  //sets the frequency for the specified pin
  bool success = SetPinFrequencySafe(led, frequency);
  
  //if the pin frequency was set successfully, turn pin 13 on
  if(success) {
    pinMode(13, OUTPUT);
    digitalWrite(13, HIGH);    
  }
}

void loop()
{
  //use this functions instead of analogWrite on 'initialized' pins
  pwmWrite(led, 127);  
}

