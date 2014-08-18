#include <math.h>
#include <util/delay.h>
#include <avr/wdt.h> 
#include "arduino/Arduino.h"
#include "arduino/EEPROM.h"
#include "DualVNH5019MotorShield.h"
#include "TimerOne.h"
#include "FlexiTimer2.h"

#define STROBE_PIN              10 
#define PUMP_PIN                0 
#define PROMPT_ENABLE           1
#define MAX_BRIGHTNESS          15
#define CPU_FREQ                16000000
#define DEFAULT_FREQUENCY       80
#define MAX_PUMP_STEPS          100

// Helper macros for frobbing bits
#define bitset(var,bitno) ((var) |= (1 << (bitno)))
#define bitclr(var,bitno) ((var) &= ~(1 << (bitno)))
#define bittst(var,bitno) (var& (1 << (bitno)))

uint8_t motor_power = 50;

typedef uint16_t quanta_t;

TimerOne timer_1;
DualVNH5019MotorShield hbridge;

void timer2_callback(void);

class Pump
{
public:
    void init(uint16_t steps)
    {
        _steps = min(MAX_PUMP_STEPS, steps);
        float rad_step = (2 * M_PI) / _steps;
        for (uint16_t step = 0; step < _steps; ++step)
        {
            _sin_table[step] = sin(step * rad_step) * motor_power;
        }
        _counter = 0;

        // initialize the hbridge
        hbridge.init();

        // set CTC mode
        TCCR0A = 0x2;
        // prescale set to 8
        TCCR0B = 0x2;
        // set our period
        OCR0A = timer_1.pwmPeriod / _steps;
        OCR0B = 0;
        // enable our interrupt
        bitSet(TIMSK0, OCIE0A);
        bitSet(TCCR0A, COM0B0);
    }

    void inline virtual step()
    {
        hbridge.setM1Speed(_sin_table[_counter++]);
        _counter %= _steps;
    }

private:
    volatile uint16_t _counter;
    uint16_t _steps;
    int8_t _sin_table[MAX_PUMP_STEPS];
};

Pump pump;

/******************************************************************************
 ** Setup
 ******************************************************************************/

void set_exposure(uint16_t us_length, int16_t offset)
{
    //int16_t cycles = (1.0 / (F_CPU / 8.0 / 2.0)) * 1E6 * us_length;
    int16_t cycles = (1.0 / (F_CPU / 8)) * 1E6 * us_length;
    int16_t on_cycle = offset;
    int16_t off_cycle = cycles + offset;
    timer_1.set_compare(on_cycle, off_cycle);
    /*
    Serial.print("us_length: ");
    Serial.print(us_length);
    Serial.print(" cycles: ");
    Serial.print(cycles);
    Serial.print(" timer_1.pwmPeriod: ");
    Serial.print(timer_1.pwmPeriod);
    Serial.print(" on_cycle: ");
    Serial.print(on_cycle);
    Serial.print(" off_cycle: ");
    Serial.println(off_cycle);
    */
}

void setup()
{
    // disable global interrupts
    Serial.begin(57600);
    Serial.println("");
    Serial.print("[");
    
    // configure timer 1
    // mode 12: CTC mode
    pinMode(STROBE_PIN, OUTPUT);
    digitalWrite(STROBE_PIN, 0);
    pinMode(5, OUTPUT);
    timer_1.initialize((1.0 / DEFAULT_FREQUENCY) * 1E6, 4);
    timer_1.configure_pin(STROBE_PIN, 2);
    set_exposure(1000, 0);
    Serial.print("timers... ");
    
    // pump hbridge
    pump.init(MAX_PUMP_STEPS);
    Serial.print("pump... ");

    // enable global interrupts
    Serial.print("] ");

    timer_1.start();

    // init
    Serial.println("init done!");
    Serial.println("");
    Serial.print("pwm period ");
    Serial.println(timer_1.pwmPeriod);
    Serial.print("OCR0A 0x");
    Serial.println(OCR0A, DEC);
    Serial.print("TCCR1A 0x");
    Serial.println(TCCR1A, BIN);
    Serial.print("TCCR1B 0x");
    Serial.println(TCCR1B, BIN);
}


/******************************************************************************
 ** Serial
 ******************************************************************************/

#if PROMPT_ENABLE
void Prompt(void)
{
    if (!Serial.available()) return;

    static long v = 0;
    static unsigned char channel = 0;
    static unsigned char onoff = 0;
    static uint16_t exposure = 1000;
    static uint16_t offset = 0;

    char ch = Serial.read();
    Serial.println(ch);
    Serial.println("");
    bool v_change = false;
    long usecs;

    switch(ch) {
        /* numbers / values */
        case '0'...'9':
            v = v * 10 + ch - '0';
            v_change = true;
            break;
        case '-':
            v *= -1;
            v_change = true;
            break;
        case 'z':
            v = 0;
            break;

        /* pin operations */
        case 'g':
            while(1)
            {
                for (int x = 0; x < 24000; ++x)
                {
                    set_exposure(exposure, x);
                    delayMicroseconds(500);
                }
                for (int x = 24000; x > 0; --x)
                {
                    set_exposure(exposure, x);
                    delayMicroseconds(500);
                }
            }
            Serial.println("gogo");
            break;
        case 'x':
            exposure = v;
            set_exposure(exposure, offset);
            Serial.println("exposure set");
            break;
        case 'o':
            offset = v;
            set_exposure(exposure, offset);
            Serial.println("offset set");
            break;
        case 'd':
            timer_1.setPeriod(v);
            Serial.println("setting period");
            break;
        case 'N':
            Serial.println("forward bump");
            break;
        case 'n':
            Serial.println("back bump");
            break;
        case 's':
            Serial.println("period set");
            break;
        case 'S':
            Serial.println("duty cycle set");
            break;
        case 'V':
            Serial.println("period decreased");
            break;
        case 'v':
            Serial.println("period increased");
            break;
        case 'y':
            // calculate microseconds
            usecs = 1 / v * 1E6;
            // timer_1.set_period(usecs);
            Serial.print(v);
            Serial.println("Hz value set!");
            v = 0;
            break;

        /* motor power */
        case 'm':
            motor_power = min(400, max(0, v));
            Serial.println("motor power set");
            v = 0;
            break;

        /* brightness */
        case 'b':
            Serial.println("strobe brightness set");
            v = 0;
            break;
        case '.':
            //if (pins[STROBE_INDEX].get_duty_cycle() < MAX_BRIGHTNESS)
            if(0)
            {
                Serial.println("strobe brightness increased");
            } else
            {
                Serial.println("");
                Serial.println("strobe brightness at max");
            }
            break;
        case ',':
            //if (pins[STROBE_INDEX].get_duty_cycle() > 0)
            if(0)
            {
                Serial.println("");
                Serial.println("strobe brightness decreased");
            } else
            {
                Serial.println("");
                Serial.println("strobe brightness at min");
            }
            break;

        /* debugging output */
        case 'p':
            Serial.println("");
            Serial.print("OCR1A 0x");
            Serial.println(OCR1A, HEX);
            Serial.print("OCR1B 0x");
            Serial.println(OCR1B, HEX);
            Serial.print("ICR1 0x");
            Serial.println(ICR1, HEX);
            break;
        default:
            Serial.print("Unknown command: ");
            Serial.println(ch, DEC);
        }
        
        Serial.print("channel: ");
        Serial.print(channel, DEC);
        Serial.print(" value: ");
        Serial.print(v, DEC);
        Serial.print(" on/off: ");
        Serial.print(onoff, DEC);
        Serial.println("");
        Serial.print("> ");

        // this helps squash accidental value inputs
        if (!v_change)
            v = 0;
}
#endif // PROMPT_ENABLE

/******************************************************************************
 ** Main loop
 ******************************************************************************/

void loop()
{
    for(;;)
    {
        //wdt_reset();
#if PROMPT_ENABLE
        Prompt();
#endif // PROMPT_ENABLE
    }
}

ISR(TIMER0_COMPA_vect)
{
    pump.step();
}
