/* 
*  To Do
*  Make time between beacons optionally random.
*  Change time between beacons updatable programatically via DTMF.
*  Add a repeat count parameter to sendMessage to automatically repeat the message x number of times.
*  Maybe switch to dash duration, end of word duration, etc.
*  Possibly integrate a .mp3 player for pre-recorded audio.
*/


#include <Arduino.h>
// #include <Tone32.h>   // Only needed for ESP32.
#include <DTMF.h>
#include "pitches.h"
#include "alphabet.h"
#include "Timer.h"
#include "Switch.h"


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
bool ledsEnabled();
bool speakerEnabled();
bool isDefaultBeaconOn();
void ledRedOn();
void ledRedOff();
void ledYellowOn();
void ledYellowOff();
void ledBlueOn();
void ledBlueOff();
void ledTest();


// General configuration.
const int   PIN_PTT         { 6 };          // Pin for Push to talk relay.
const int   PIN_BUZZER      { 7 };          // Send music and CW out this pin to the buzzer.
const int   PIN_XMIT        { 2 };          // Send music and CW out this pin to the mic of the transceiver.
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
bool        beaconEnabled   { false };      // Keeps track of whether the beacon is enabled or not.
const int   dotDuration     { 60 };         // Length of time for one dot. Basic unit of measurement. 
const unsigned long xmitInterval { 20 * 1000 };  // Length of time between beacon intervals.
M3::Timer   timerBeacon(M3::Timer::COUNT_DOWN);

// LED configuration.
const int   PIN_LED_R       { 5 };          // Pin to the red LED.
const int   PIN_LED_Y       { 3 };          // Pin to the yellow LED.
const int   PIN_LED_B       { 4 };          // Pin to the blue LED.

// Switch configuration.
const int   PIN_SW_BEACON   { 8 };          // Pin to force beacon state switch.
const int   PIN_SW_LEDS     { 9 };          // Pin to the LEDs enabled switch.
const int   PIN_SW_SPEAKER  { 10 };         // Pin to the speaker enabled switch.
M3::Switch  switchBeacon(LOW, PIN_SW_BEACON);



void setup() {
    pinMode(PIN_SW_BEACON, INPUT_PULLUP);
    pinMode(PIN_SW_LEDS, INPUT_PULLUP);
    pinMode(PIN_SW_SPEAKER, INPUT_PULLUP);
    
    pinMode(PIN_LED_R, OUTPUT);
    pinMode(PIN_LED_B, OUTPUT);
    pinMode(PIN_LED_Y, OUTPUT);
    pinMode(PIN_PTT, OUTPUT);
    pinMode(PIN_BUZZER, OUTPUT);
    pinMode(PIN_XMIT, OUTPUT);

    ledRedOff();
    ledBlueOff();
    ledYellowOff();
    disablePTT();
  
    ALPHABET.toUpperCase();     // Make ALPHABET actually contain upper case characters.
    randomSeed(analogRead(0));  // In case random delays between transmissions are used.

    if (ledsEnabled()) {
        ledTest();
    }

    timerBeacon.setDuration(xmitInterval);
    timerBeacon.start();
}




void loop() {

    switch (state) {
        case State::WAIT:
        {
            // Always check to see if it is time to do beacon stuff.
            if ((beaconEnabled && timerBeacon.isTimerExpired()) || switchBeacon.isSwitchPressed()) {
                beaconEnabled = true;
                state = State::BEACON;
                break;
            }

            // Check for DTMF tone and do something with it if one exists.
            //char thisDtmfChar = getDtmfChar();

            // TODO: Delete this line and uncomment out the one above.
            char thisDtmfChar = 0;

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
    int melody[] = { NOTE_G4, NOTE_E4, NOTE_C4, NOTE_C4, NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4, NOTE_G4, NOTE_G4, NOTE_G4, NOTE_E4 };
    int noteDurations[] = { 8, 8, 4, 4, 8, 8, 8, 8, 4, 4, 4, 4 }; // 4 = quarter note, 8 = eighth note, etc.
    int noteSize = sizeof(melody)/sizeof(*melody);

    for (int thisNote = 0; thisNote < noteSize; thisNote++) {
        int noteDuration = 1000 / noteDurations[thisNote];
        if (speakerEnabled()) {
            tone(PIN_BUZZER, melody[thisNote], noteDuration);
        }
        if (ledsEnabled()) ledBlueOn();

        // To distinguish the notes, set a minimum time between them. The note's duration + 30% seems to work well.
        int pauseBetweenNotes = noteDuration * 1.30;
        delay(pauseBetweenNotes);

        // stop the tone playing:
        noTone(PIN_BUZZER);
        ledBlueOff();
    }
}



/**
 * @brief Sends an alphanumeric message in Morse Code.
 * @param msg The alphanumeric char string to send.
 */
void sendMessage(const char* msg) {
    for (unsigned int i = 0; i < strlen(msg); i++){
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
    ledYellowOff();
    morseToneWait(4);
}



/**
 * @brief Sends a Morse Code dot, of the proper frequency, to the appropriate pin on the controller.
 */
void sendDot() {
    // Send the dot.
    if (speakerEnabled()) {
        tone(PIN_BUZZER, CW_FREQ);
    }
    if (ledsEnabled()) ledYellowOn();
    morseToneWait(1);

    // Send the silent pause after the dot.
    noTone(PIN_BUZZER);
    ledYellowOff();
    morseToneWait(1);
}



/**
 * @brief Sends a Morse Code dash, of the proper frequency, to the appropriate pin on the controller.
 */
void sendDash() {
    // Send the dash.
    if (speakerEnabled()) {
        tone(PIN_BUZZER, CW_FREQ);
    }
    if (ledsEnabled()) ledYellowOn();
    morseToneWait(3);

    // Send the silent pause after the dash.
    noTone(PIN_BUZZER);
    ledYellowOff();
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
    digitalWrite(PIN_PTT, HIGH);
    if (ledsEnabled()) ledRedOn();
    delay(2000);
}



/**
 * @brief Disables the Push-to-talk pin/relay.
 */
void disablePTT() {
    digitalWrite(PIN_PTT, LOW);
    ledRedOff();
    delay(2000);
}



/**
 * @brief Used to determine if the LEDs are enabled or not.
 * @return Are the LEDs enabled or not.
 */
bool ledsEnabled() {
    int val = digitalRead(PIN_SW_LEDS);

    if (val == 0) {
        return true;
    } else {
        return false;
    }
}



/**
 * @brief Used to determine if the speaker is enabled or not.
 * @return Is the speaker enabled or not.
 */
bool speakerEnabled() {
    int val = digitalRead(PIN_SW_SPEAKER);
    if (val == 0) {
        return true;
    } else {
        return false;
    }
}



/**
 * @brief Used to determine if the default beacon switch is on or off.
 * @return Boolean value whether the default beacon switch is on or off.
 */
bool isDefaultBeaconOn() {
    int val = digitalRead(PIN_SW_BEACON);
    if (val == 0) {
        return true;
    } else {
        return false;
    }
}



/**
 * @brief Turns the red LED on.
 */
void ledRedOn() {
    digitalWrite(PIN_LED_R, LOW);
}



/**
 * @brief Turns the red LED off.
 */
void ledRedOff() {
    digitalWrite(PIN_LED_R, HIGH);
}



/**
 * @brief Turns the Yellow LED off.
 */
void ledYellowOn() {
    digitalWrite(PIN_LED_Y, LOW);
}



/**
 * @brief Turns the Yellow LED off.
 */
void ledYellowOff() {
    digitalWrite(PIN_LED_Y, HIGH);
}



/**
 * @brief Turns the blue LED on.
 */
void ledBlueOn() {
    digitalWrite(PIN_LED_B, LOW);
}



/**
 * @brief Turns the blue LED off.
 */
void ledBlueOff() {
    digitalWrite(PIN_LED_B, HIGH);
}



/**
 * @brief Performs an LED test by sequencing through the LEDs.
 */
void ledTest() {
    ledRedOn();
        delay(700);
    ledBlueOn();
        delay(700);
    ledYellowOn();
        delay(2000);
    ledRedOff();
        delay(700);
    ledBlueOff();
        delay(700);
    ledYellowOff();
}