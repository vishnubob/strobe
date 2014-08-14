#include <math.h>
#include <util/delay.h>
#include <avr/wdt.h> 
#include "arduino/Arduino.h"
#include "arduino/EEPROM.h"
#include "DualVNH5019MotorShield.h"
#include "TimerOne.h"
#include "FlexiTimer2.h"


#define STROBE_PIN              9 
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

uint16_t motor_power = 400;

typedef uint16_t quanta_t;

TimerOne timer_1;
FlexiTimer2 timer_2;
DualVNH5019MotorShield hbridge;

class Pump
{
public:
    void init_pump(uint16_t steps)
    {
        _steps = min(MAX_PUMP_STEPS, steps);
        float rad_step = (2 * math.pi) / _steps;
        for (uint16_t step = 0; step < _steps; ++step)
        {
            int16_t val = sin(step * rad_step) * motor_power;
            _sin_table[step] = val;
        }
        _counter = 0;

        // initialize the hbridge
        hbridge.init();
    }

    void inline virtual step()
    {
        _counter += 1;
        if (_counter > _steps)
            _counter = 0;

        uint16_t power = _sin_table[_counter];
        hbridge.setM1Speed(power);
    }

private:
    uint16_t counter;
    int16_t _sin_table[MAX_PUMP_STEPS];
};

Pump pump;

/******************************************************************************
 ** Setup
 ******************************************************************************/

void setup()
{
    // disable global interrupts
    Serial.begin(57600);
    Serial.println("");
    Serial.print("[");
    
    // configure timer 1
    timer_1.initialize(1 / DEFAULT_FREQUENCY * 1E6)
    timer_1.setPwmDuty(STROBE_PIN, 10);

    // configure timer 2
    timer_2.set(1, 1 / DEFAULT_FREQUENCY / MAX_PUMP_STEPS, timer2_callback)
    Serial.print("timers... ");

    // pump hbridge
    pump.init(MAX_PUMP_STEPS);
    Serial.print("pump... ");
    
    // enable global interrupts
    Serial.print("] ");

    // init
    Serial.println("init done!");
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

    char ch = Serial.read();
    Serial.println(ch);

    switch(ch) {
        /* numbers / values */
        case '0'...'9':
            v = v * 10 + ch - '0';
            break;
        case '-':
            v *= -1;
            break;
        case 'z':
            v = 0;
            break;
        case 'c':
            channel = v;
            Serial.println("");
            Serial.println("channel set");
            v = 0;
            break;

        /* pin operations */
        case 'x':
            pins[channel].disable();
            Serial.println("");
            Serial.println("pin disabled");
            break;
        case 'X':
            pins[channel].enable();
            Serial.println("");
            Serial.println("pin enabled");
            break;
        case 'u':
            pins.reset();
            Serial.println("");
            Serial.println("all pins reset");
            break;
        case 'q':
            pins.disable();
            Serial.println("");
            Serial.println("all pins disabled");
            break;
        case 'Q':
            pins.enable();
            Serial.println("");
            Serial.println("all pins enabled");
            break;
        case 'N':
            pins[channel].set_offset(1);
            delay(v);
            pins[channel].set_offset(0);
            Serial.println("forward bump");
            break;
        case 'n':
            pins[channel].set_offset(-1);
            delay(v);
            pins[channel].set_offset(0);
            Serial.println("back bump");
            break;
        case 's':
            pins[channel].set_period(v);
            Serial.println("period set");
            v = 0;
            break;
        case 'S':
            pins[channel].set_duty_cycle(v);
            Serial.println("duty cycle set");
            v = 0;
            break;
        case 'V':
            pins[channel].set_period(pins[channel].get_period() - 1);
            Serial.println("");
            Serial.println("period decreased");
            break;
        case 'v':
            pins[channel].set_period(pins[channel].get_period() + 1);
            Serial.println("");
            Serial.println("period increased");
            break;
        case 'y':
            unsigned int total_step;
            total_step = (CPU_FREQ / OCR2A / 32.0 / (float)(v));
            for(uint8_t dev_idx = 0; dev_idx < DEVICE_COUNT; ++dev_idx)
            {
                pins[dev_idx].set_period(total_step);
                pins.reset(false);
            }
            Serial.println("");
            Serial.print(v);
            Serial.println("Hz value set!");
            v = 0;
            break;

        /* motor power */
        case 'm':
            motor_power = min(400, max(0, v));
            Serial.println("");
            Serial.println("motor power set");
            v = 0;
            break;

        /* brightness */
        case 'b':
            pins[STROBE_INDEX].set_duty_cycle(v);
            Serial.println("");
            Serial.println("strobe brightness set");
            v = 0;
            break;
        case '.':
            if (pins[STROBE_INDEX].get_duty_cycle() < MAX_BRIGHTNESS)
            {
                pins[STROBE_INDEX].set_duty_cycle(pins[STROBE_INDEX].get_duty_cycle() + 1);
                Serial.println("");
                Serial.println("strobe brightness increased");
            } else
            {
                Serial.println("");
                Serial.println("strobe brightness at max");
            }
            break;
        case ',':
            if (pins[STROBE_INDEX].get_duty_cycle() > 0)
            {
                pins[STROBE_INDEX].set_duty_cycle(pins[STROBE_INDEX].get_duty_cycle() - 1);
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
}
#endif // PROMPT_ENABLE

/******************************************************************************
 ** Main loop
 ******************************************************************************/

void loop()
{
    for(;;)
    {
        wdt_reset();
#if PROMPT_ENABLE
        Prompt();
#endif // PROMPT_ENABLE
    }
}

void timer2_callback
{
    pump.step();
}
