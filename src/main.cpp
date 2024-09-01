/* 
Arduino controlled Fox Hunting Xmtr and/or Beacon.
Code cobbled together from various sources.
http://ttg.org.au/arduino_fox.php
http://www.wt4y.com/arduino-fox-controller
http://www.wcar.ca/photo-gallery/fox-tx/

Pin A0 is the input for DTMF, see pic for pin out.
Pin 5 is the square wave output for the Morse Code, see pic for pin out.
Pin 7 is Push to Talk pin (low = engage relay for PTT).

Using your DTMF 
1 = Start
5 = hi in morse code
6 = Call sign you entered
7 = 73
Any other key = Stop
*/


/* To Do
*  Make time between beacons optionally random.
*  Change time between beacons updatable programatically via DTMF.
*  Add a repeat count parameter to sendMessage to automatically repeat the message x number of times.
*  Maybe switch to dash duration, end of word duration, etc.
*/


#include <Arduino.h>
// #include <Tone32.h>   // Only needed for ESP32.
#include <DTMF.h>
#include "pitches.h"
#include "alphabet.h"
#include "Timer.h"


// Function declarations.
char getDtmfChar();
void playMelody();
void sendMessage(const char* msg);
void sendCharacter(const char c);
void sendMorseCode(const String tokens);
void sendEndOfWord();
void sendDot();
void sendDash();
void morseToneWait(const int n);
void enablePTT();
void disablePTT();


// General configuration.
const int   PIN_PTT         { 7 };          // Pin for Push to talk relay.
const int   PIN_BUZZER      { 5 };          // Send music and CW out this pin.
const int   CW_FREQ         { 700 };        // CW pitch.
const int   ADC_BITDEPTH    { 1024 };       // Number of discrete reading the controllers ADC can distinguish. 
const char  beaconString[]   = "KI4OOK/FOX KI4OOK/FOX"; 
const char  idString[]       = "DE KI4OOK"; 
enum  class State           { WAIT, BEACON };
State  state = State::WAIT;

// DTMF related.
const int   PIN_DTMF        { A0 };         // Pin to read DTMF signals from.
const float dtmfBlockSize   { 128.0 };      // DTMF block size. I'm not sure why this is a float.
const float dtmfSampleRate  { 8926.0 };     // DTMF sample rate. I'm not sure why this is a float.
float       d_mags[8];                      // Array for DTMF magnitude readings.
DTMF        dtmf(dtmfBlockSize, dtmfSampleRate);

// Transmission related.
bool        beaconEnabled   { false }; 
const int   dotDuration     { 60 };         // Length of time for one dot. Basic unit of measurement. 
M3::Timer   timerBeacon(M3::Timer::COUNT_DOWN);




void setup() {
    pinMode(PIN_PTT, OUTPUT);
    disablePTT();
  
    ALPHABET.toUpperCase();
    randomSeed(analogRead(0));  // In case random delays between transmissions are used.
}




void loop() {

    switch (state) {
        case State::WAIT:
        {
            // Always check to see if it is time to do beacon stuff.
            if (beaconEnabled && timerBeacon.isTimerExpired()) {
                state = State::BEACON;
                break;
            }

            // Check for DTMF tone and do something with it if one exists.
            char thisDtmfChar = getDtmfChar();

            if (thisDtmfChar) {                             
                switch (thisDtmfChar) {
                    case 49:  // Number 1                   // Enables beacon transmission.
                        beaconEnabled = true;                           
                        state = State::BEACON;
                        break;
                    case 53:  // Number 5                   // Sends morse code for "hi".
                        enablePTT();
                        sendMessage("hi");
                        disablePTT();
                        break;
                    case 54:  // Number 6                   // Sends morse code with call sign. 
                        enablePTT();
                        sendMessage(idString);
                        disablePTT();
                        break;
                    case 55:  // Number 7                   // Sends morse code saying "73".
                        enablePTT();
                        sendMessage("73");
                        disablePTT();
                        break;
                    default:  // Any other number           // Turn off transmissions.
                        beaconEnabled = false;  
                        break;
                }
            }

            break;
        }

        case State::BEACON:
        {
            // If beaconEnabled is false then bail out of this state and go back to WAIT.
            if (!beaconEnabled) {
                state = State::WAIT;
                break;
            }

            // Send the beacon.
            enablePTT();
            playMelody();
            delay(1000);  
            sendMessage(beaconString);
            delay(500);
            disablePTT();

            // Restart the timer for the next beacon cycle.
            timerBeacon.reset();
            state = State::WAIT;
            break;
        }

        default:
        {
            // Strange state condition. Make everything safe.
            state = State::WAIT;
            beaconEnabled = false;
            break;
        }

    }
}
















/////////////////////////////////////////////////////////
//                     FUNCTIONS                       //
/////////////////////////////////////////////////////////

/**
 * @brief  Checks to see if there is a DTMF signal on the input pin.
 * @return Retuns the character number representing the DTMF buttoon presssed.
 */
char getDtmfChar() {
    dtmf.sample(PIN_DTMF);
    dtmf.detect(d_mags, ADC_BITDEPTH);
    return dtmf.button(d_mags, 1800.);
}



/**
 * @brief Sends the fox hunt melody.
 */
void playMelody() {
    // Melody related.
    int melody[] = { NOTE_G4, NOTE_E4, NOTE_C4, NOTE_C4, NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4, NOTE_G4, NOTE_G4, NOTE_G4, NOTE_E4 };
    int noteDurations[] = { 8, 8, 4, 4, 8, 8, 8, 8, 4, 4, 4, 4 }; // 4 = quarter note, 8 = eighth note, etc.
    int noteSize = sizeof(melody)/sizeof(*melody);

    for (int thisNote = 0; thisNote < noteSize; thisNote++) {
        int noteDuration = 1000 / noteDurations[thisNote];
        tone(PIN_BUZZER, melody[thisNote], noteDuration);

        // To distinguish the notes, set a minimum time between them. The note's duration + 30% seems to work well.
        int pauseBetweenNotes = noteDuration * 1.30;
        delay(pauseBetweenNotes);

        // stop the tone playing:
        noTone(PIN_BUZZER);
    }
}



/**
 * @brief Sends an alphanumeric message in Morse Code.
 * @param msg The alphanumeric char string to send.
 */
void sendMessage(const char* msg) {
    for (unsigned int i = 0; i < sizeof(msg); i++){
        sendCharacter(msg[i]);
    }
}



/**
 * @brief Converts a single alphanumeric character to Morse Code dot(s) and dash(es) and sends them.
 * @param c The alphanumeric character to send.
 */
void sendCharacter(const char c) {
    for (unsigned int i = 0; i < alphabet.length(); i = i + 1) {
        if (alphabet[i] == c || ALPHABET[i] == c) {
            sendMorseCode(morseCode[i]);
            return;
        }
    }
}



/**
 * @brief Takes a sequence of Morse Code dot(s) and dash(es) and sends them as individual dot(s) and dash(es).
 * @param tokens A string of Morse Code "tokens". This is a sequence of dot(s) and dash(es) which represent a Morse Code letter.
 */
void sendMorseCode(String tokens) {
   for (unsigned int i = 0; i < tokens.length(); i = i + 1) {
       switch (tokens[i]) {
           case '-':
               sendDash();
               break;
           case '.':
               sendDot();
               break;
           case ' ':
               sendEndOfWord();
               break;
       }
   }

    // Time to wait between each symbol.
    morseToneWait(2);
}



/**
 * @brief Sends a Morse Code dot, of the proper frequency, to the appropriate pin on the controller.
 */
void sendEndOfWord() {
    // Send a silent pause to signify the end of a word.
    noTone(PIN_BUZZER);
    morseToneWait(4);
}



/**
 * @brief Sends a Morse Code dot, of the proper frequency, to the appropriate pin on the controller.
 */
void sendDot() {
    // Send the dot.
    tone(PIN_BUZZER, CW_FREQ);
    morseToneWait(1);

    // Send the silent pause after the dot.
    noTone(PIN_BUZZER);
    morseToneWait(1);
}



/**
 * @brief Sends a Morse Code dash, of the proper frequency, to the appropriate pin on the controller.
 */
void sendDash() {
    // Send the dash.
    tone(PIN_BUZZER, CW_FREQ);
    morseToneWait(3);

    // Send the silent pause after the dash.
    noTone(PIN_BUZZER);
    morseToneWait(1);
}



/**
 * @brief Delays code execution for a period of time equal to the duration of a Morse Code dot times the n argument passed in.
 *        The reason for this to be called is that the tone function, that sends the CW signal, is non-blocking, but we
 *        actually need for the code to wait on the transmission of each CW tone prior to continuing. Thus we need to 
 *        force a deley of the proper amount of time.
 * @param n Number of dot durations to delay for.
 */
void morseToneWait(const int n) {
    delay(n * dotDuration);
}



/**
 * @brief Enables the Push-to-talk pin/relay.
 */
void enablePTT() {
    digitalWrite(PIN_PTT, LOW);
    delay(2000);
}



/**
 * @brief Disables the Push-to-talk pin/relay.
 */
void disablePTT() {
    digitalWrite(PIN_PTT, HIGH);
    delay(2000);
}