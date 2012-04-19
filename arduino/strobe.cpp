#include "arduino/Arduino.h"
#include "arduino/EEPROM.h"
#include <math.h>
#include <util/delay.h>
#include <avr/wdt.h> 

// configurable options
#define CPU_FREQ            16000000
#define STROBE_PIN          12

#define MAX_BRIGHTNESS      10

// Helper macros for frobbing bits
#define bitset(var,bitno) ((var) |= (1 << (bitno)))
#define bitclr(var,bitno) ((var) &= ~(1 << (bitno)))
#define bittst(var,bitno) (var& (1 << (bitno)))

/******************************************************************************
 ** Globals
 ******************************************************************************/

volatile unsigned int       on_value;
volatile unsigned int       off_value;
volatile unsigned int       phase_counter;

/******************************************************************************
 ** Setup
 ******************************************************************************/

void setup(void)
{
    // setup serial
    Serial.begin(9600);
    pinMode(STROBE_PIN, OUTPUT);

    // disable global interrupts
    cli();

    // disable the timer0 interrupt
    bitclr(TIMSK0, TOIE0);

    // setup timer1 - 16bit
    TCCR1A = 0;
    TCCR1B = 0;
    // select CTC mode
    bitset(TCCR1B, WGM12);
    // 1:1
    bitset(TCCR1B, CS10);
    // base 32000 hz
    OCR1A = 500;
    // enable compare interrupt
    bitset(TIMSK1, OCIE1A);
    
    // enable global interrupts
    sei();
    Serial.println("init...");
    Serial.print("> ");
    Serial.flush();

    on_value = 3200;
    off_value = 1;
    phase_counter = 0;
}

/******************************************************************************
 ** Main Loop
 ******************************************************************************/

void loop(void)
{
    static long v = 0;

    if (Serial.available()) 
    {
        char ch = Serial.read();

        switch(ch) 
        {
            case '0'...'9':
                v = v * 10 + ch - '0';
                break;
            case 'z':
                v = 0;
                break;
            case '=':
                if (on_value < 50000)
                    ++on_value;
                break;
            case '-':
                if (on_value > 50)
                    --on_value;
                break;
            case 's':
                /* set */
                if ((v >= 50) && (v < 50000))
                    on_value = v;
                v = 0;
                break;
            case 'b':
                /* brightness */
                if ((v >= 1) && (v < MAX_BRIGHTNESS))
                    off_value = v;
                v = 0;
                break;
            case '.':
                /* brightness */
                if (off_value < MAX_BRIGHTNESS)
                    off_value++;
                break;
            case ',':
                /* brightness */
                if (off_value > 1)
                    off_value--;
                break;
        }
        Serial.println("");
        Serial.print("Value:");
        Serial.print(v);
        Serial.print(" on: ");
        Serial.print(on_value);
        Serial.print(" off: ");
        Serial.println(off_value);
        Serial.print("> ");
    }
}

/******************************************************************************
 ** Timer Interrupt
 ******************************************************************************/

ISR(TIMER1_COMPA_vect) 
{
    phase_counter += 1;
    if(phase_counter >= on_value)
    {
        digitalWrite(STROBE_PIN, HIGH);
        phase_counter = 0;
    } else
    if (phase_counter >= off_value)
    {
        digitalWrite(STROBE_PIN, LOW);
    }
}
