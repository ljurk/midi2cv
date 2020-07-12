#include <Arduino.h>
#include <AH_MCP4921.h>
#include <MIDI.h>

// ATmega ADC intern
#define div_pin A5
#define vel_pin 10

// cc
#define cc1 102
#define cc2 103
#define cc3 104
#define cc4 105
#define cc1pin 3
#define cc2pin 5
#define cc3pin 6
#define cc4pin 9

// digitale pins
#define clk_pin 8
#define gate_pin 4

// mcp4921
#define mcp_mosi 11
#define mcp_sck 13
#define mcp_cs 7
// update MCP4921
// Aref vom DAC ist genau 5V -> (1/12)*5V = 0,0833
#define VOctLinCoeff 0.0833
// DAC ist auf A0 bis A5 gestimmt!
#define VOctShift -1.75

// MIDI Sachen
#define MIDI_CHANNEL 13
#define MIDI_CLOCK_DIVIDER 8

AH_MCP4921 AnalogOutput(mcp_mosi, mcp_sck, mcp_cs);

// clock
long previousMillis;
bool clockHigh = false;
byte dividerCounter = 0;

MIDI_CREATE_DEFAULT_INSTANCE();

void handleClock(void) {
    if (dividerCounter == MIDI_CLOCK_DIVIDER) {
        clockHigh = true;
        previousMillis = millis();
        digitalWrite(clk_pin, HIGH);
        dividerCounter = 0;
    } else {
        dividerCounter++;
    }
}

void  handleNoteOn(byte channel, byte pitch, byte velocity) {
    digitalWrite(gate_pin, HIGH);

    // funktion in der Klammer gibt werte von 0 - 5 raus
    // *1000 weil map funktion die kommastellen abschneidet
    float cv_out = 1000*(VOctLinCoeff*pitch+VOctShift);

    AnalogOutput.setValue(constrain(map(cv_out, 0, 5000, 0, 4095), 0, 4095));

    // update velocity pin
    analogWrite(vel_pin, map(velocity, 0, 127, 0, 255));
}

void handleNoteOff(byte channel, byte pitch, byte velocity) {
    digitalWrite(gate_pin, LOW);

    // update velocity
    analogWrite(vel_pin, 0);

    // update v/oct
    AnalogOutput.setValue(0);
}

void handleControlChange(byte channel, byte number, byte value) {
    switch (number) {
        case cc1:
            analogWrite(cc1pin, map(value, 0, 127, 0, 255));
            break;
        case cc2:
            analogWrite(cc2pin, map(value, 0, 127, 0, 255));
            break;
        case cc3:
            analogWrite(cc3pin, map(value, 0, 127, 0, 255));
            break;
        case cc4:
            analogWrite(cc4pin, map(value, 0, 127, 0, 255));
            break;
        case default:
            break;
  }
}

void setup() {
  pinMode(clk_pin, OUTPUT);
  pinMode(gate_pin, OUTPUT);

  pinMode(cc1pin, OUTPUT);
  pinMode(cc2pin, OUTPUT);
  pinMode(cc3pin, OUTPUT);
  pinMode(cc4pin, OUTPUT);

  pinMode(vel_pin, OUTPUT);

  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandleControlChange(handleControlChange);
  MIDI.setHandleClock(handleClock);

  MIDI.begin(MIDI_CHANNEL);

  digitalWrite(gate_pin, LOW);
  digitalWrite(clk_pin, LOW);
}

void loop() {
  MIDI.read();
  if (clockHigh == true && millis() - previousMillis >= 50) {
    digitalWrite(clk_pin, LOW);
    clockHigh = false;
  }
}
