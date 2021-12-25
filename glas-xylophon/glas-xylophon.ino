#include <USB-MIDI.h>
USBMIDI_CREATE_DEFAULT_INSTANCE();

#include <FastLED.h>
#define NUM 12 // number of tones, glasses and leds are the same
#define DATA_PIN 5
// This is an array of leds.
CRGB leds[NUM];

// Include the HCPCA9685 library
#include "HCPCA9685.h"
// I2C slave address for the device/module.
#define I2CAdd 0x40
// Create an instance of the library
HCPCA9685 HCPCA9685(I2CAdd);
// Determine the following values by test
#define SRV_RING 110	// servo pos when ringing
#define SRV_BACK 95	// servo pos when in zero position
#define SRV_DELAY 100	// milliseconds to wait for servo movement to complete

// Define MIDI note number for first glass
// Refer to https://upload.wikimedia.org/wikipedia/commons/2/27/NoteNamesFrequenciesAndMidiNumbers_v2.svg
#define FIRST_NOTE 60 // C4 or C' (261.6 Hz)

// Button at pin 10 can be used to toggle silent mode
# define BUTTON 10
// Silent mode (only use leds, no servos)
bool silent = 0; // default: off
bool button_in;

// Set DEBUG to 1 to enable serial debug output
#define DEBUG 1

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void setup()
{
  // sanity check delay - allows reprogramming if accidently blowing power w/leds
  delay(2000);
  
  Serial.begin(9600);
  // while (!Serial); // Wait here until serial connection is established

  pinMode(BUTTON, INPUT);

  // Initialise the HCPCA9685 library and set it to 'servo mode'
  HCPCA9685.Init(SERVO_MODE);
  // Wake the device up
  HCPCA9685.Sleep(false);

  // Initialize the FastLED library for WS2812B led string
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM);  // GRB ordering is typical

  // Loop over all leds and servos at the beginning
  for (int i = 0; i < NUM; i++) {
    // Set current led to a random color
    leds[i] = CHSV(random8(),255,255);
    //leds[i] = CRGB::Blue;
    FastLED.show();
    //delay(100);
    HCPCA9685.Servo(i, SRV_RING);
    delay(2 * SRV_DELAY);
    HCPCA9685.Servo(i, SRV_BACK);
    delay(2 * SRV_DELAY);
    // Turn current led back to black (off) before next loop
    leds[i] = CRGB::Black;
  }
  // Final call of FastLED.show() to turn off last led
  FastLED.show();

  // Listen for MIDI messages on all channels
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleNoteOn(OnNoteOn);
  //NoteOff makes no sense for percussion-like instr
  //MIDI.setHandleNoteOff(OnNoteOff);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void loop()
{
  // Listen to incoming notes
  MIDI.read();
  // Update leds
  //FastLED.show();
  // Read buttton and toogle silent mode if pressed
  if (1 == digitalRead(BUTTON)) {
    silent = !silent;
    delay(50); //debounce
    #ifdef DEBUG
      SPrint(F("Switched silent mode to "));
      if (silent) {
        SPrintln(F("on"));
      } else {
        SPrintln(F("off"));
      }
    #endif
    // Wait for button release (falling edge)
    while(digitalRead(BUTTON));
    delay(50); //debounce
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SPrint(String text) {
  Serial.print(text);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SPrintln(String text) {
  Serial.println(text);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
static void OnNoteOn(byte channel, byte note, byte velocity) {
#ifdef DEBUG
  SPrint(F("NoteOff from channel: "));
  SPrint(String(channel));
  SPrint(F(", note: "));
  SPrint(String(note));
  SPrint(F(", velocity: "));
  SPrintln(String(velocity));
#endif
  // Glasses should be on channel #1 and in the range [FIRST_NOTE; FIRST_NOTE + NUM]
  // Bell is on channel #2
  if (1 == channel && ((not (note < FIRST_NOTE)) || (not (note > (FIRST_NOTE + NUM - 1))))) {
    #ifdef DEBUG
      SPrint(F("Ringing glass "));
      SPrintln(String(note - FIRST_NOTE));
    #endif
    leds[note - FIRST_NOTE] = CHSV(random8(),255,255);
    FastLED.show();
    if (!silent) {
      HCPCA9685.Servo(note - FIRST_NOTE, SRV_RING);
      delay(SRV_DELAY);
      HCPCA9685.Servo(note - FIRST_NOTE, SRV_BACK);
      delay(SRV_DELAY);
    } else {
      delay(2 * SRV_DELAY);
    }
    leds[note - FIRST_NOTE] = CRGB::Black;
    FastLED.show();
  }
  
  // Bell is on channel #2 and is the last servo. It has no led.
  if (2 == channel) {
    #ifdef DEBUG
      SPrintln(F("Ringing bell "));
    #endif
    if (!silent) {
      HCPCA9685.Servo(NUM, SRV_RING);
      delay(SRV_DELAY);
      HCPCA9685.Servo(NUM, SRV_BACK);
      delay(SRV_DELAY);
    } else {
      delay(2 * SRV_DELAY);
    }
  }
}

/*
// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
static void OnNoteOff(byte channel, byte note, byte velocity) {
#ifdef DEBUG
  SPrint(F("NoteOff from channel: "));
  SPrint(String(channel));
  SPrint(F(", note: "));
  SPrint(String(note));
  SPrint(F(", velocity: "));
  SPrintln(String(velocity));
#endif
  if (1 == channel && ((not (note < FIRST_NOTE)) || (not (note > (FIRST_NOTE + NUM - 1))))) {
    leds[note - FIRST_NOTE] = CRGB::Black;
    if (!silent) {
      HCPCA9685.Servo(note - FIRST_NOTE, SRV_BACK);
      delay(SRV_DELAY);
    }
  }
}
*/
