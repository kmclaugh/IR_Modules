#include "IR_Codes.h"

/*Pin delcarations*/
#define IRInterrupt   0 //IR Detector Interrupt correspondes to pin 2
const int ledPin    = 13;

/*IR Declarations*/
#define MAXPULSE 1000 //longest we will for a single pulse 
#define FUZZINESS 20 // What percent we will allow in variation to match the same code
volatile int signal[100]; //array to store the recieved IR signal
int recieved_signal = 0;
volatile unsigned long current_pulses;
volatile int IR_input = 0;
volatile int IR_state = 0;
volatile boolean Read_IR = false;
static unsigned long last_interrupt_time = 0;
boolean same_code = false;
int data_number = 0; //which command is being decoded. 0 is NUMBER
/*End IR Declarations*/

/*Recieved signal handling declarations*/
volatile boolean recieve_commands = false; //determines if NUMBER has been recieved before command
volatile int timer_counter = 1; //used for doubling the period of the timer interrupt
/*End recieved signal handling declarations*/


int counter = 0;

void setup(void) {
  
  Serial.begin(9600);   
  
  pinMode(ledPin, OUTPUT); 
  digitalWrite(ledPin,LOW); 
  
  // initialize the IR Reciever interrupt to occur on both rising and falling edges
  attachInterrupt(IRInterrupt,IR_interrupt_handler, CHANGE);

  
  Serial.println("Ready to Decode IR!");  
}

void loop(){
  
  if (Read_IR == true){
    Read_IR_Routine();
  }//end if READ_IR
  
  else if (IR_state != 0){
    wait_for_IR();
  }//end else if
  
  else{
    /*What to do when not dealing with IR codes*/
    counter ++;
    delay(1000);
    //Serial.print("counter= ");Serial.println(counter);
  }//end else
  
}//end loop

void Read_IR_Routine(){
    /*If we have just read an complete IR_code this section determines 
  what to do with it*/
  boolean valid_signal = correctpulses();
  if (valid_signal == true){
    /***********Place what to do routines here*************************/
    compare_IR_arrays();
    /******************************************************************/
  }//end if valid signal
  Read_IR = false;
}//end Read_IR_Routine

void compare_IR_arrays(){
  /*Looks for a match between the recieved signal and those stored in EEPROM*/
  if (compare_IR(NUMBER_Data)){
    recieved_signal = 1;
    Serial.println("Number");
  }
  else if (compare_IR(UP_Data)){
    recieved_signal = 2;
    Serial.println("Up");
  }
  else if (compare_IR(DOWN_Data)){
    recieved_signal = 3;
    Serial.println("Down");
  }
  else if (compare_IR(STOP_Data)){
    recieved_signal = 4;
    Serial.println("Stop");
  }
  else{
    recieved_signal = 5; //signal did not match
    Serial.println("No Match");
  }
  
}//end compare routine

boolean compare_IR(int compare_array[]){
  /*compares the current signal to a given compare_array signal*/
  boolean same_code = true;
  int delta;
  for (int i=0; i<200; i++){
    
    //Serial.print("sig: ");Serial.print(signal[i]);Serial.print(" data: ");Serial.println(compare_array[i]);
    if ((signal[i] !=0) && (compare_array[i] != 0)){
      delta = signal[i]-compare_array[i];
      delta = abs(delta);
      if (delta >= signal[i]*FUZZINESS/100){ //delta is greater than FUZZINESS percentage
        same_code = false;
        break;
      }//end if
    }//end if
    
    else{ //if signal[i]==0
      break; //break because its the end of the transmission
    }//end else
    
  }//end for
  
  return(same_code);
}//end compare_IR

/*End Code handling routines**************************************************************************************/

/*Timer1 functions******************************************************************************/
ISR(TIMER1_COMPA_vect){
 /*Determines what to do when the Timer1 goes off, namely turn recieve commands off*/
 if (timer_counter >= 2){ //effectively doubles the period of the timer
   recieve_commands = false;
   timer_counter = 1;
   stop_timer();
 }
 else{
   timer_counter ++;
 }   
}

void start_timer(){
  /*Initializes timer1 interrupt for the maximum time. About 5 seconds*/
  cli();//stop interrupts

//set timer1 interrupt at 1Hz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 1hz increments
  OCR1A = 65000;// = (16*10^6) / (1*1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS12 and CS10 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);

  sei();//allow interrupts
}

void stop_timer(){
  /*Stops Timer1 interrupt*/
  TCCR1B = _BV(WGM13);
}

void restart_timer(){
  /*restarts the timer by resseting timer_counter to 1*/
  stop_timer();
  timer_counter = 1;
  start_timer();
}
/*End Timer1 Functions************************************************************************/

/*IR interrupt routines*******************************************************************************************/
void IR_interrupt_handler(){
  /*Works like a state machine. IR_states indicate not recieving, low, and high voltage.
  Since the interrupt will occur on both rising and falling edges we always know which state
  should be next. A delay() timer in the loop() determines when the transmission is finsihed*/
  unsigned long interrupt_time = micros(); //get the current time since the arduino was turned on
  unsigned long time_delta = interrupt_time - last_interrupt_time; //determine time since last interrupt
  last_interrupt_time = interrupt_time; //set last interrupt time to current
  
  switch (IR_state){ //controls what to do in each case. Controls have to be short so we don't miss the next interrupt
    
    case 0:{//0 is the idle state. On the first interrupt recieved we leave the idle state
      current_pulses = 0; //it's the first pulse being recieved
      IR_state = 1;//on the next interrupt we should be in IR_state 1
      stop_timer();
      break;
    }//end state 0
    
    case 1:{ //1 is LOW voltage
      signal[current_pulses] = time_delta;
      current_pulses ++;
      IR_state = 2;
      break;
    }//end state 1
    
    case 2:{ //1 is HIGH voltage
      signal[current_pulses] = time_delta;
      current_pulses ++;
      IR_state = 1;
      break;
    }//end state2
  }//end switch case
}//end IR_interrupt_handler

void wait_for_IR(){
  /*Serves as a time out to determine when the last IR Code has been sent.
   After completeing the delay this resets the IR_state and tells the loop()
   to Read the new IR code*/
  delay(MAXPULSE);
  IR_state = 0;
  Read_IR = true;
}//end wait for IR
    
boolean correctpulses(){
  /*Divides the recieved pulses by ten to throw out the least significant digit.
  Checks for valid signal (i.e. a long enough transmission) incase of an unwanted interrupt*/
  int pulse_counter = 0; //count number of recieved pulses
  for (int i=0; i<200; i++){
    signal[i] = signal[i]/10;
    if (signal[i] != 0){
      pulse_counter ++;
    }
    else{
      break;
    }//end else
  }//end for
  if (pulse_counter > 4){ //at least 5 pulses must be reciever for a valid input
    return(true);
  }//end if
  else{
    return(false);
  }//end else
}//end correct pulses

void print_signal(volatile uint16_t print_signal[]) {
 /*print IR code array data for debug only. Must use correct array in this function manually*/
  Serial.println("int NAME_Data[] = {");
  Serial.println("// LOW, HIGH Voltage (in 10's of microseconds)");
  for (int i = 0; i < 100; i=i+2) {
    Serial.print("\t"); // tab
    Serial.print(print_signal[i]);
    Serial.print(", ");
    Serial.print(print_signal[i+1]);
    Serial.println(",");
  }
  Serial.print("\t"); // tab
  Serial.print("};");
  Serial.println("");
}
/*End IR interrupt routines****************************************************************************/

void blink_led(int blink_count){
  /*Blinks an LED a given number of times*/
  for (int i=0; i < blink_count; i++){
      digitalWrite(ledPin, HIGH);
      delay(500);
      digitalWrite(ledPin, LOW);
      delay(500);
  } //end for loop
}//end blink_led

