/**
 * MyArduboyPlaytune
 *
 * Derived from ArduboyPlaytune
 * https://github.com/Arduboy/ArduboyPlaytune
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 OBONO
 * Based on work (c) Copyright 2016, Chris J. Martinez, Kevin Bates, Josh Goebel, Scott Allen
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "MyArduboy2.h"
#include <avr/power.h>

/*  Defines  */

#define AVAILABLE_CHANNELS  2

#define TUNE_OP_PLAYNOTE    0x90 // play a note, note is next byte
#define TUNE_OP_STOPNOTE    0x80 // stop a note
#define TUNE_OP_MARK        0xD0 // set mark to repeat
#define TUNE_OP_REPEAT      0xE0 // repeat the score from the marked point
#define TUNE_OP_STOP        0xF0 // stop playing

#define FREQUENCY_MAX   (0xFFFF / DUTY_CYCLE_MAX)
#define FREQUENCY_IDLE  500
#define DUTY_CYCLE_MIN  2
#define DUTY_CYCLE_MAX  5
#define NOTE_MAX        127
#define NOTE_MIDDLE     sizeof(midiByteNoteFrequencies)

/*  Local Functions  */

static void stepScore(void);
static void playNote(uint8_t chan, uint8_t note, uint8_t dutyCycle);
static void stopNote(uint8_t chan);
static void setupTimer(uint8_t timer, uint16_t frequency, uint8_t dutyCycle);

/*  Local Functions (macros)  */

#define getTimerNum(chan)   ((chan == 0) ? 3 : 1)

#define setHighTimerPin(t)  (*timer ## t ## PinPort |= timer ## t ## PinMask)
#define setLowTimerPin(t)   (*timer ## t ## PinPort &= ~(timer ## t ## PinMask))
#define toggleTimerPin(t)   (*timer ## t ## PinPort ^= timer ## t ## PinMask)

#define initTimer(t, port, mask) do { \
            TCCR ## t ## A = 0; \
            TCCR ## t ## B = _BV(WGM ## t ## 2) | _BV(CS ## t ## 0); \
            timer ## t ## PinPort = (port); \
            timer ## t ## PinMask = (mask); \
        } while (false)

#define startTimer(t, ocr, prescalarBits) do { \
            TCCR ## t ## B = (TCCR ## t ## B & 0b11111000) | (prescalarBits); \
            OCR ## t ## A = (ocr); \
            TCNT ## t = 0; /*LJS*/ \
            bitSet(TIMSK ## t, OCIE ## t ## A); \
        } while (false)

#define stopTimer(t) do { \
            bitClear(TIMSK ## t, OCIE ## t ## A); \
            setLowTimerPin(t); \
        } while (false)

/*  Local Variables  */

static uint8_t numChans = 0;
static uint8_t audioPriority;

static volatile byte *timer1PinPort;
static volatile byte timer1PinMask;
static volatile byte *timer3PinPort;
static volatile byte timer3PinMask;

static volatile uint8_t timer1DutyCycle;
static volatile unsigned long timer1ToggleCount;
static volatile uint8_t timer3DutyCycle;
static volatile unsigned long timer3ToggleCount;
static volatile uint16_t timer3Frequency;

static volatile bool isAllMuted = false;         // indicates all sound is muted
static volatile bool isMuteScore = false;        // indicates tone playing so mute other channels
static volatile bool isScorePlaying = false;     // is the score still playing?
static volatile bool isWaitTimerPlaying = false; // is it currently playing a note?
static volatile bool isTonePlaying = false;      // is the tone still playing?

// pointer to a function that indicates if sound is enabled
static boolean (*outputEnabled)();

// pointers to your musical score and your position in said score
static volatile const byte *scoreStart = 0;
static volatile const byte *scoreCursor = 0;

// score tuning parameters
static volatile int8_t scorePitchDefault, scorePitch;
static volatile uint8_t scoreRepeat;

// Table of midi note frequencies
// The lowest notes might not work, depending on the Arduino clock frequency
// Ref: http://www.phy.mtu.edu/~suits/notefreqs.html
PROGMEM static const uint8_t midiByteNoteFrequencies[] = {
    8,8,9,9,10,11,11,12,13,14,14,15,16,17,18,19,20,22,23,24,26,27,29,31,32,34,
    36,39,41,43,46,49,52,55,58,61,65,69,73,78,82,87,92,98,104,110,116,123,131,
    138,147,155,165,174,185,196,207,220,233,247
};
PROGMEM static const uint16_t midiWordNoteFrequencies[(NOTE_MAX + 1) - NOTE_MIDDLE] = {
    261,277,293,311,329,349,370,392,415,440,466,494,523,554,587,622,659,698,
    740,784,830,880,932,988,1046,1108,1174,1244,1318,1397,1480,1568,1661,
    1760,1864,1975,2093,2217,2349,2489,2637,2794,2960,3136,3322,3520,3729,
    3951,4186,4435,4698,4978,5274,5587,5920,6272,6645,7040,7458,7902,8372,
    8870,9397,9956,10548,11175,11840,12544
};
PROGMEM static const int8_t tuneNoteTable[] = {
    0, 1, 2, 3, 4, 6, 8, 12, 0, -12, -8, -6, -4, -3, -2, -1
};

/*---------------------------------------------------------------------------*/
/*                            Initialize & Finalize                          */
/*---------------------------------------------------------------------------*/

void MyArduboy2::initAudio(uint8_t chans)
{
    outputEnabled = audio.enabled;
    if (chans == 0 || chans > AVAILABLE_CHANNELS) return;
    if (numChans > 0) closeAudio();
    numChans = chans;
    for (uint8_t chan = 0; chan < chans; chan++) {
        byte pin;
#ifdef PIN_SPEAKER_2
        pin = (chan == 0) ? PIN_SPEAKER_1 : PIN_SPEAKER_2;
#else
        pin = PIN_SPEAKER_1;
#endif
        byte pinPort = digitalPinToPort(pin);
        byte pinMask = digitalPinToBitMask(pin);
        volatile byte *outReg = portOutputRegister(pinPort);

        *portModeRegister(pinPort) |= pinMask; // set pin to output mode

        switch (getTimerNum(chan)) {
        case 1:
            initTimer(1, outReg, pinMask);
            break;
        case 3:
            power_timer3_enable();
            initTimer(3, outReg, pinMask);
            stopNote(0); // start timer 3
            break;
        }
    }
}

void MyArduboy2::closeAudio(void)
{
    for (uint8_t chan = 0; chan < numChans; chan++) {
        switch (getTimerNum(chan)) {
        case 1:
            stopTimer(1);
            TCCR1B = _BV(CS10) | _BV(CS11);
            break;
        case 3:
            stopTimer(3);
            power_timer3_disable();
            break;
        }
    }
    numChans = 0;
    isScorePlaying = isTonePlaying = isMuteScore = false;
}

/*---------------------------------------------------------------------------*/
/*                             Control parameters                            */
/*---------------------------------------------------------------------------*/

bool MyArduboy2::isAudioEnabled(void)
{
    return audio.enabled();
}

void MyArduboy2::setAudioEnabled(bool on)
{
    (on) ? audio.on() : audio.off();
}

void MyArduboy2::toggleAudioEnabled(void)
{
    audio.toggle();
}

void MyArduboy2::saveAudioOnOff(void)
{
    audio.saveOnOff();
}

/*---------------------------------------------------------------------------*/
/*                                 Play sounds                               */
/*---------------------------------------------------------------------------*/

void MyArduboy2::playTone(uint16_t frequency, uint16_t duration,
        uint8_t priority = 0xFF, uint8_t dutyCycle = DUTY_CYCLE_MIN)
{
    if (numChans == 0) return;
    if (numChans == 1 && isScorePlaying) {
        if (priority > audioPriority) return;
        stopScore();
    }

    isAllMuted = !outputEnabled();
    frequency = min(frequency, FREQUENCY_MAX);
    dutyCycle = constrain(dutyCycle, DUTY_CYCLE_MIN, DUTY_CYCLE_MAX);
    unsigned long toggle_count = ((unsigned long)duration * frequency * dutyCycle + 500) / 1000;
    if (toggle_count == 0) toggle_count = 1;

    uint8_t chan = numChans - 1;
    if (chan == 0) {
        isWaitTimerPlaying = true;
        timer3ToggleCount = toggle_count;
    } else {
        isTonePlaying = true;
        timer1ToggleCount = toggle_count;
        if (isScorePlaying) {
            isMuteScore = (priority <= audioPriority);
        } else {
            audioPriority = priority;
        }
    }
    setupTimer(getTimerNum(chan), frequency, dutyCycle);
}

void MyArduboy2::stopTone(void)
{
    if (numChans == 0) return;
    if (numChans == 1) {
        if (!isScorePlaying) stopNote(0);
    } else {
        stopTimer(1);
        isTonePlaying = isMuteScore = false;
    }
}

void MyArduboy2::playScore(const byte *score, uint8_t priority = 0, int8_t pitch = 0)
{
    if (isScorePlaying) {
        if (priority > audioPriority) return;
        stopScore();
    } else if (isTonePlaying) {
        isMuteScore = (priority > audioPriority);
    }
    audioPriority = priority;
    scoreStart = score;
    scoreCursor = scoreStart;
    scorePitchDefault = scorePitch = pitch;
    scoreRepeat = 0;
    isScorePlaying = true;
    stepScore(); // execute initial commands
}

void MyArduboy2::stopScore(void)
{
    for (uint8_t chan = 0; chan < numChans; chan++) {
        stopNote(chan);
    }
    isScorePlaying = false;
}

/*---------------------------------------------------------------------------*/

/*
Do score commands until a "wait" is found, or the score is stopped.
This is called initially from playScore(), but then is called
from the interrupt routine when waits expire.

If CMD < 0x80, then the other 7 bits and the next byte are a
15-bit big-endian number of msec to wait
*/
static void stepScore(void)
{
    while (true) {
        byte command = pgm_read_byte(scoreCursor++);
        uint8_t opcode = command & 0xF0;
        uint8_t opvalue = command & 0x0F;
        uint8_t chan = opvalue & 0x03;
        if (opcode == TUNE_OP_STOPNOTE) { // stop note
            stopNote(chan);
        } else if (opcode == TUNE_OP_PLAYNOTE) { // play note
            uint8_t dutyCycle = ((opvalue & 0x0C) >> 2) + 2;
            isAllMuted = !outputEnabled();
            playNote(chan, pgm_read_byte(scoreCursor++) + scorePitch, dutyCycle);
        } else if (opcode < 0x80) { // wait count in msec.
            uint16_t duration = (uint16_t)command << 8 | pgm_read_byte(scoreCursor++);
            timer3ToggleCount = ((unsigned long)duration * timer3Frequency + 500) / 1000;
            if (timer3ToggleCount == 0) timer3ToggleCount = 1;
            break;
        } else if (opcode == TUNE_OP_MARK) { // set mark to repeat
            scoreStart = scoreCursor - 1;
            if (scoreRepeat > 0) scorePitch += (int8_t)pgm_read_byte(tuneNoteTable + opvalue);
        } else if (opcode == TUNE_OP_REPEAT) { // repeat score
            if (opvalue == 0 || ++scoreRepeat < (1 << opvalue)) {
                scoreCursor = scoreStart;
            } else {
                scoreStart = scoreCursor;
                scorePitch = scorePitchDefault;
                scoreRepeat = 0;
            }
        } else if (opcode == TUNE_OP_STOP) { // stop score
            isScorePlaying = false;
            break;
        }
    }
}

static void playNote(uint8_t chan, uint8_t note, uint8_t dutyCycle)
{
    if (chan >= numChans) return;
    if (chan == 1 && isTonePlaying) return;
    if (note > NOTE_MAX) return;

    uint16_t frequency = (note < NOTE_MIDDLE) ?
            pgm_read_byte(midiByteNoteFrequencies + note) :
            pgm_read_word(midiWordNoteFrequencies + note - NOTE_MIDDLE);
    if (chan == 0) isWaitTimerPlaying = true;
    setupTimer(getTimerNum(chan), frequency, dutyCycle);
}

static void stopNote(uint8_t chan)
{
    switch (getTimerNum(chan)) {
    case 1:
        if (!isTonePlaying) stopTimer(1);
        break;
    case 3:
        isWaitTimerPlaying = false;
        if (!isMuteScore) setLowTimerPin(3);
        setupTimer(3, FREQUENCY_IDLE, DUTY_CYCLE_MIN);
        break;
    }
}

static void setupTimer(uint8_t timerNum, uint16_t frequency, uint8_t dutyCycle)
{
    frequency *= dutyCycle;
    unsigned long ocr = F_CPU / frequency - 1;
    byte prescalarBits = 0b001;
    if (ocr > 0xFFFF) {
        ocr = F_CPU / 64 / frequency - 1;
        prescalarBits = 0b011;
    }

    // Set the OCR for the given timer, then turn on the interrupts
    switch (timerNum) {
    case 1:
        timer1DutyCycle = dutyCycle;
        startTimer(1, ocr, prescalarBits);
        break;
    case 3:
        timer3DutyCycle = dutyCycle;
        timer3Frequency = frequency;
        startTimer(3, ocr, prescalarBits);
        break;
    }
}

/*---------------------------------------------------------------------------*/
/*                        Interrupt service routines                         */
/*---------------------------------------------------------------------------*/

ISR(TIMER1_COMPA_vect) // TIMER 1
{
    if (isTonePlaying) {
        if (timer1ToggleCount > 0) {
            if (!isAllMuted && timer1ToggleCount % timer1DutyCycle < 2) toggleTimerPin(1);
            if (timer1ToggleCount > 0) timer1ToggleCount--;
        } else {
            isTonePlaying = isMuteScore = false;
            stopTimer(1);
        }
    } else {
        if (!isAllMuted && timer1ToggleCount % timer1DutyCycle < 2) toggleTimerPin(1);
    }
}

ISR(TIMER3_COMPA_vect) // TIMER 3
{
    // Timer 3 is the one assigned first, so we keep it running always
    // and use it to time score waits, whether or not it is playing a note.
    if (isWaitTimerPlaying && !isMuteScore && !isAllMuted) {
        if (timer3ToggleCount % timer3DutyCycle < 2) toggleTimerPin(3);
    }
    if (timer3ToggleCount > 0 && --timer3ToggleCount == 0) {
        (isScorePlaying) ? stepScore() : stopNote(0);
    }
}
