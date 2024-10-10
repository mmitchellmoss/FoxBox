# FoxBox
Controller for a "fox" radio transmitter.

---

## Usage
See OPERATING.md for Operating Instructions

### 2 Meter Frequency (VHF)
I have found no really definitive information on the Internet regarding which 2 meter frequency is best to use for ARDF/Fox Hunting. Some of the suggestions or practices that I have found are as following:

145.300 MHz (Someone claimed this is the national ARDF frequency, but I find no other reference to that)  
146.565 MHz (Found 7 times on the Internet)  
146.430 MHz (Found as secondary to 146.565 MHz)  
146.520 MHz (3rd harmonic receive frequency of 439.56MHz)  
146.535 MHz   
146.550 MHz (Suggested use with a PL tone)  
145.565 MHz (Found as secondary to 146.565 MHz)  
146.580 MHz (3rd harmonic receive frequency of 439.740)

**Stay away from 146.520 MHz**

Based on the collection of data above, it seems that **146.565 MHz** is the most popular option.

### 70 CM Frequency (UHF)
Any common simplex frequency other than 446.000.
446.0250 MHz would be fine

### Duty cycle (time transmitting vs. time silent)
This varies depending on the desired difficulty of the hunt.  
For easy hunts more frequent transmissions are desirable, whereas for experts longer pauses with shorter transmissions
    would be the way to go.  
Right now I am thinking that transmissions of about 15 seconds with a 1 minute pause is where I will start.

---

## Wiring the Baofeng transceiver
Notes on the wires within the official Baofeng earphone (Kenwood style).
* Blue and gold wires are connected to each other when the PTT button on the remote is pressed. These are the ones that would need to be connected via the relay to enable PTT.
* Green and blue are the wires connected to the speaker.
* Red and gold are the wires connected to the mic.

Wire colors connected to the HT connector
| Color  | Pin Size | Use         | Connection |
| -----  | -----    | -----       | -----      |
| Green  | Small    | Speaker     | Tip       |
| Blue   | Small    | Speaker     | Sleeve    |
| Red    | Large    | Mic         | Ring      |
| Gold   | Large    | Mic         | Sleeve    |

![Plug color code](https://github.com/user-attachments/assets/37c4ba3d-8209-4482-b44f-281adfa72733)

---

## Schematic
***Schematic Mistake*** I have the battery listed as being 5VDC. In practice, I use a LIPO battery, which is actually 7.2VDC (I think). Anything within the VIN spec of the Arduino Nano should be fine as the Nano supplies the 5VDC that is used by the rest of the circuitry. 

![Fox Box Schematic](https://github.com/user-attachments/assets/09bdead5-9cca-448e-bb17-bf2da77564a9)

Schematic created on https://www.circuit-diagram.org/

### Notes
Oddly to me, you don't need a common ground between the Arduino and the mic or speaker pins on the radio.
Electrically, I don't know how this works, but it kind of does. However, the transmitted audio volume varies a lot
depending on how the audio and mic wire are located in proximity to each other. I have found that grounding
the sleeve of the audio/speaker plug (blue wire) seems to clear that up. In this case the speaker which is 
active and being monitored during the wait state looking for DTMF signals has a ground which is cool. Also, when 
the PTT relay is engaged, it then also grounds the mic input which seems to clear up the transmitted audio.

![FoxControllerPhoto](https://github.com/user-attachments/assets/0d64c581-cddb-4c83-a528-850d736533af)


---

## To Do
* Have an array of messages that can be transmitted either sequentially, randomly, or as time progresses.
    This could be just to spice things up, but it could also be to drop messages containing hints as 
    the duration of the hunt becomes longer and longer.
* Add an overall watchdog type timer that shuts the beacon off if things go on for too long.
* Make time between beacons optionally random.
* Change time between beacons updatable programatically via DTMF. Im not certain this actually makes sense.
* Add a repeat count parameter to sendMessage to automatically repeat the message x number of times?
* Maybe switch to dash duration, end of word duration, etc. for code consistency.
* Possibly integrate a .mp3 player for pre-recorded audio.
* Maybe add back the buzzer/speaker for local sound. I couldn't do this as a seperate digital out as the 
    tone() function built into the Ardunio library does not allow tone() to work on more than one pin
    at a time. I could possibly add it back by including a SPDT switch that sends the audio either to the
    speaker or to the mic in on the HT, thus bypassing the Arduino entirely.
* Fix the schematic to show that the battery voltage is actually greater than 5V.
* Add photos and a description of the overal build as a whole.
