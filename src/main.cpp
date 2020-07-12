#include <Arduino.h>
#include <AH_MCP4921.h>
#include <MIDI.h>

//ATmega ADC intern
#define div_pin A5
#define vel_pin 10
#define cc_four 9
#define cc_three 6
#define cc_two 5
#define cc_one 3

//digitale pins
#define clk_pin 8
#define gate_pin 4

//mcp4921 Pins
#define mcp_mosi 11
#define mcp_sck 13
#define mcp_cs 7

//MIDI Sachen
#define MIDI_CHANNEL 13

AH_MCP4921 AnalogOutput(mcp_mosi,mcp_sck,mcp_cs);

short divide;
short i = 0;

short CC_one = 102;
short CC_two = 103;
short CC_three = 104;
short CC_four = 105;

long previousMillis;
long currentMillis;

MIDI_CREATE_DEFAULT_INSTANCE();


void handleClock(void){
  i++;

}

void  handleNoteOn(byte channel, byte pitch, byte velocity) {
 
    digitalWrite(gate_pin, HIGH); 

    //update MCP4921
    const float VOctLinCoeff = 0.0833; //Aref vom DAC ist genau 5V -> (1/12)*5V = 0,0833
    //const float note = map(pitch, 0, 127, 0, 4095);
    const float VOctShift = -1.75; //DAC ist auf A0 bis A5 gestimmt!
    
    float cv_out = 1000*(VOctLinCoeff*pitch+VOctShift); // funktion in der Klammer gibt werte von 0 - 5 raus
                                                        // *1000 weil map funktion die kommastellen abschneidet
    unsigned int dac_out = constrain(map(cv_out, 0, 5000, 0, 4095), 0, 4095);  //map(cv_out, 0, 5000, 0, 4095)
    
    AnalogOutput.setValue(dac_out);

    //update velocity pin
    short velOut = map(velocity, 0, 127, 0, 255);
    analogWrite(vel_pin, velOut); 
  //check MIDI channel
}//handleNoteOn

void handleNoteOff(byte channel, byte pitch, byte velocity) {
  
    digitalWrite(gate_pin, LOW);

    //update velocity
    analogWrite(vel_pin, 0);
  
}//handleNoteOff

void handleControlChange (byte channel, byte number, byte value) {
  if(number == CC_one) {
    int ccOneOut = map(value, 0,127, 0, 255);
    analogWrite(cc_one, ccOneOut);
  }//CC_one

  if (number == CC_two){
    int ccTwoOut = map (value, 0, 127, 0, 255);
    analogWrite(cc_two, ccTwoOut);
  }//CC_two

  if(number == CC_three){
    int ccThreeOut = map(value, 0, 127, 0, 255);
    analogWrite(cc_three, ccThreeOut);
  }//CC_three

  if(number == CC_four){
    int ccFourOut = map(value, 0, 127, 0, 255);
    analogWrite(cc_four, ccFourOut);
  }//CC_four
}//handleControlChange

short clk_div() {
  int adc_val = analogRead(div_pin);

  divide = adc_val >> 7;

  switch(divide){
    case 0:
      return 3;
      break;
    case 1:
      return 6;
      break;
    case 2:
      return 12;
      break;
    case 3:
      return 24;
      break;
    case 4:
      return 24; 
      break;
    case 5:
      return 48;
      break;
    case 6:
      return 48;
      break;
    case 7:
      return 96;
      break;
  }
}//clk_div

void setup() {
  pinMode(clk_pin, OUTPUT);
  pinMode(gate_pin, OUTPUT);

  pinMode(cc_one, OUTPUT);
  pinMode(cc_two, OUTPUT);
  pinMode(cc_three, OUTPUT);
  pinMode(cc_four, OUTPUT);

  pinMode(vel_pin, OUTPUT);

  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandleControlChange(handleControlChange);
  MIDI.setHandleClock(handleClock);
  
  MIDI.begin(MIDI_CHANNEL);

  //Serial.begin(31250);

  //digitalWrite(mcp_cs, HIGH);
  digitalWrite(gate_pin, LOW);
  digitalWrite(clk_pin, LOW);
}

void loop() {
  MIDI.read();

  if (i == clk_div()) {
    digitalWrite(clk_pin, HIGH);
    previousMillis = millis();
  }

  if (millis() - previousMillis == 20) {
    digitalWrite(clk_pin, LOW);
    
  }

  if (i >= clk_div()) {
    i = 0;
  }
}
