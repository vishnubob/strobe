#include <math.h>
#include <util/delay.h>
#include <avr/wdt.h> 
#include "arduino/Arduino.h"
#include "arduino/EEPROM.h"
#include "DualVNH5019MotorShield.h"

DualVNH5019MotorShield md;

#define STROBE_PIN              3 
#define VOICECOIL1_PIN          0
#define VOICECOIL2_PIN          1
#define VOICECOIL_COUNT         2
#define STROBE_INDEX            2
#define DEVICE_COUNT            3
#define PROMPT_ENABLE           1
#define MAX_BRIGHTNESS          10
#define CPU_FREQ                16000000

// Helper macros for frobbing bits
#define bitset(var,bitno) ((var) |= (1 << (bitno)))
#define bitclr(var,bitno) ((var) &= ~(1 << (bitno)))
#define bittst(var,bitno) (var& (1 << (bitno)))

const int _device_map[] = {VOICECOIL1_PIN, VOICECOIL2_PIN, STROBE_PIN};

/******************************************************************************
 ** Pin
 ******************************************************************************/

class Pin
{
public:
    void virtual init(unsigned char pin, 
                unsigned char step_on = 0, unsigned char step_off = 0, 
                bool enable = false, unsigned int max_off=-1)
    {
        _counter = 0;
        _pin = pin;
        _enable = enable;
        _max_off = max_off;
        pinMode(_pin, OUTPUT);
        set_step_on(step_on);
        set_step_off(step_off);
        off();
    }

    void inline virtual off() 
    {
        digitalWrite(_pin, LOW);
    }

    void inline virtual on()
    {
        digitalWrite(_pin, HIGH);
    }

    void disable() 
    { 
        _enable = false; 
        off(); 
    }
    void enable() 
    { 
        _enable = true; 
    }

    void sync()
    {
        off();
        _counter = 0;
    }

#if PROMPT_ENABLE
    void print()
    {
        char hz_value_str[12];
        double hz_value;
        char us_value_str[12];
        double us_value;
        
        // build the Hz string
        hz_value = (CPU_FREQ / OCR2A / 32.0 / (double)(_step_on + _step_off));
        dtostrf(hz_value, 6, 4, hz_value_str);
        // build the ms string
        us_value = 1.0 / (CPU_FREQ / OCR2A / 32.0 / (double)_step_off) * 1000000;
        dtostrf(us_value, 6, 1, us_value_str);

        Serial.print("Pin:  on: ");
        Serial.print(_step_on);
        Serial.print(" (");
        Serial.print(hz_value_str);
        Serial.print(" Hz) off: ");
        Serial.print(_step_off);
        Serial.print(" (");
        Serial.print(us_value);
        Serial.print(" us)");
        Serial.print(" offset: ");
        Serial.print(_step_offset, DEC);
        /*
        Serial.print(" counter: ");
        Serial.print(_counter, DEC);
        Serial.print(" state: ");
        Serial.print(_state, DEC);
        */
        Serial.print(" en: ");
        Serial.println(_enable, DEC);

    }
#endif // PROMPT_ENABLE

    void inline step()
    {
        if (!_enable) return;
        _counter++;
        if((_state) && (_counter >= _step_off))
        {
            off();
            _state = LOW;
            _counter = 0;
        } else
        if ((!_state) && (_counter >= (_step_on + _step_offset)))
        {
            on();
            _state = HIGH;
            _counter = 0;
        }
    }

    /* setters */
    void set_step_on(unsigned int step) { _step_on = step; }
    void set_step_off(unsigned int step) { _step_off = min(_max_off, step); }
    void set_offset(int offset) { _step_offset = offset; }
    void reset_offset() { _step_offset = 0; }
    void set_step(unsigned int step_on, unsigned int step_off) 
    { 
        set_step_on(step_on);
        set_step_off(step_off);
    }
    
    /* getters */
    unsigned int get_step_on() const { return _step_on; }
    unsigned int get_step_off() const { return _step_off; }
    unsigned char get_state() const { return _state; }
    
protected:
    unsigned int _counter;
    unsigned int _step_on;
    unsigned int _step_off;
    unsigned int _step_offset;
    unsigned int _max_off;
    unsigned char _pin;
    bool _enable;
    bool _state;
};

class VoiceCoilPin : public Pin
{
public:
    void init(unsigned char pin, 
                unsigned char step_on = 0, unsigned char step_off = 0, 
                bool enable = false, unsigned int max_off=-1)
    {
        _counter = 0;
        _pin = pin;
        _enable = enable;
        _max_off = max_off;
        _interval = false;
        set_step_on(step_on);
        set_step_off(step_off);
        off();
    }

    void inline on() 
    {
        if (_pin == 0)
        {
            md.setM1Speed(400);
        } else
        if (_pin == 1)
        {
            md.setM2Speed(400);
        }
    }

    void inline off()
    {
        if (_pin == 0)
        {
            md.setM1Speed(-400);
        } else
        if (_pin == 1)
        {
            md.setM2Speed(-400);
        }
    }
private:
    bool _interval;
};

/******************************************************************************
 ** PinSet
 ******************************************************************************/

class PinSet
{
public:
    void init()
    {
        for(unsigned char idx = 0; idx < DEVICE_COUNT; ++idx) 
        {
            if (idx == STROBE_INDEX)
            {
                _devices[idx] = new Pin;
                _devices[idx]->init(_device_map[idx], 0, 0, false, MAX_BRIGHTNESS);
            } else
            {
                _devices[idx] = new VoiceCoilPin;
                _devices[idx]->init(_device_map[idx]);
            }
        }
    }

    void voicecoils_enable()
    {
        for(unsigned char idx = 0; idx < VOICECOIL_COUNT; ++idx) 
            _devices[idx]->enable();
    }

    void voicecoils_disable()
    {
        for(unsigned char idx = 0; idx < VOICECOIL_COUNT; ++idx) 
            _devices[idx]->disable();
    }

    void strobe_enable()
    {
        _devices[STROBE_INDEX]->enable();
    }

    void strobe_disable()
    {
        _devices[STROBE_INDEX]->disable();
    }

    void enable()
    {
        voicecoils_enable();
        strobe_enable();
    }

    void disable()
    {
        voicecoils_disable();
        strobe_disable();
    }

    void reset()
    {
        set_default_timings();
        for(unsigned char idx = 0; idx < DEVICE_COUNT; ++idx) 
            _devices[idx]->reset_offset();
        cli();
        bitclr(TIMSK2, OCIE2A);
        for(unsigned char idx = 0; idx < DEVICE_COUNT; ++idx) 
        {
            _devices[idx]->sync();
        }
        bitset(TIMSK2, OCIE2A);
        sei();
    }

    void set_default_timings()
    {
        for(unsigned char idx = 0; idx < VOICECOIL_COUNT; ++idx) 
        {
            //_devices[idx]->set_step(130, 130);
            // Film rate
            _devices[idx]->set_step(157, 158);
        }
        //_devices[STROBE_INDEX]->set_step(259, 1);
        // Film rate
        _devices[STROBE_INDEX]->set_step(306, 9);
    }

    void voicecoil_set_step(unsigned int _on, unsigned int _off)
    {
        for(unsigned char idx = 0; idx < VOICECOIL_COUNT; ++idx) 
        {
            _devices[idx]->set_step_on(_on);
            _devices[idx]->set_step_off(_off);
        }
    }

    void strobe_set_step(unsigned int _on, unsigned int _off)
    {
        _devices[STROBE_INDEX]->set_step_on(_on);
        _devices[STROBE_INDEX]->set_step_off(_off);
    }

    void inline step()
    {
        for(unsigned char idx = 0; idx < DEVICE_COUNT; ++idx)
        {
            _devices[idx]->step();
        }
    }

    Pin &operator[] (int idx)
    {
        idx = min(STROBE_INDEX, max(0, idx));
        return *(_devices[idx]);
    }

private:
    //Pin _devices[DEVICE_COUNT];
    Pin *_devices[DEVICE_COUNT];
};

/******************************************************************************
 ** Ramp
 ******************************************************************************/

class Ramp
{
public:
    void init(int pt1, int pt2, unsigned long ttl, unsigned long delay=0, bool loop_flag=false, bool flip_flag=false)
    {
        _enable = false;
        _delay = 0;
        _loop_flag = loop_flag;
        _flip_flag = flip_flag;
        set_ramp(pt1, pt2, ttl);
    }

    void set_ramp(int pt1, int pt2, unsigned long ttl)
    {
        _value = 0;
        _pt1 = pt1;
        _pt2 = pt2;
        _ttl = ttl;
        set_clock();
    }

    void set_clock()
    {
        _timestamp = millis() + _delay;
        _slope = static_cast<float>(_pt2 - _pt1) / static_cast<float>(_ttl);
    }

    int step()
    {
        if (millis() < _timestamp)
        {
            return _value;
        }
        if (_enable) {
            if (timeout()) {
                _value = _pt2;
                _enable = _loop_flag;
                if (_flip_flag) {
                    flip();
                } else
                {
                    reset();
                }
            } else {
                _value = (millis() - _timestamp) * _slope + _pt1;
            }
        }
        return _value;
    }

    void print()
    {
        Serial.print("Ramp: [");
        Serial.print(_pt1, DEC);
        Serial.print("-");
        Serial.print(_pt2, DEC);
        Serial.print("]: ");
        Serial.print(_value, DEC);
        Serial.print(" delay: ");
        Serial.print(_delay, DEC);
        Serial.print(" ttl: ");
        Serial.print(_ttl, DEC);
        Serial.print(" flip: ");
        Serial.print(_flip_flag, DEC);
        Serial.print(" loop: ");
        Serial.print(_loop_flag, DEC);
        Serial.print(" en: ");
        Serial.println(_enable, DEC);
    }

    void flip() { set_ramp(_pt2, _pt1, _ttl); }
    void reset() { set_ramp(_pt1, _pt2, _ttl); }
    void enable() { _enable = true; reset(); }
    void disable()  { _enable = false; }
    bool is_enabled()  { return _enable; }
    void set_delay(unsigned long delay) { _delay = delay; }
    void set_flip(bool flag) { _flip_flag = flag; }
    void set_loop(bool flag) { _loop_flag = flag; }
    void set_pt1(int pt1) { _pt1 = pt1; reset(); }
    void set_pt2(int pt2) { _pt2 = pt2; reset(); }
    void set_ttl(unsigned long ttl) { _ttl = ttl; reset(); }
    bool timeout() { return ((millis() - _timestamp) >= _ttl); }
    int get_value() { return _value; }

private:
    int _pt1;
    int _pt2;
    int _value;
    unsigned long _ttl;
    unsigned long _delay;
    unsigned long _timestamp;
    float _slope;
    bool _enable;
    bool _flip_flag;
    bool _loop_flag;
};

/******************************************************************************
 ** Globals
 ******************************************************************************/

PinSet          pins;
Ramp            ramps[DEVICE_COUNT];

/******************************************************************************
 ** Setup
 ******************************************************************************/

void setup()
{
    // disable global interrupts
    cli();
    
    Serial.begin(57600);
    Serial.println("");
    Serial.print("[");
    
    // setup timer2 - 8bits 
    TCCR2A = 0;
    TCCR2B = 0;
    // 1:8
    bitset(TCCR2B, CS21);
    bitset(TCCR2B, CS20);

    // select CTC mode
    bitset(TCCR2A, WGM21);
    // start the exposure loop
    OCR2A = 32;
    // enable compare interrupt
    bitset(TIMSK2, OCIE2A);
    Serial.print("timer... ");

    // init hbridge
    Serial.print("hbridge... ");
    md.init();
    
    Serial.print("pins... ");
    pins.init();
    pins.reset();
    pins.enable();

    // enable global interrupts
    sei();
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
        case 'o':
            onoff = !onoff;
            Serial.println("");
            Serial.println("on/off toggled");
            break;
        case 's':
            if (onoff)
            {
                pins[channel].set_step_on(v);
            } else
            {
                pins[channel].set_step_off(v);
            }
            Serial.println("");
            Serial.println("pin on/off set");
            v = 0;
            break;
        case 'V':
            pins[channel].set_step_on(pins[channel].get_step_on() - 1);
            Serial.println("");
            Serial.println("pin frequency decreased");
            break;
        case 'v':
            pins[channel].set_step_on(pins[channel].get_step_on() + 1);
            Serial.println("");
            Serial.println("pin frequency increased");
            break;

        /* brightness */
        case 'b':
            pins[STROBE_INDEX].set_step_off(v);
            Serial.println("");
            Serial.println("strobe brightness set");
            v = 0;
            break;
        case '.':
            if (pins[STROBE_INDEX].get_step_off() < MAX_BRIGHTNESS)
            {
                pins[STROBE_INDEX].set_step_off(pins[STROBE_INDEX].get_step_off() + 1);
                pins[STROBE_INDEX].set_step_on(pins[STROBE_INDEX].get_step_on() - 1);
                Serial.println("");
                Serial.println("strobe brightness increased");
            } else
            {
                Serial.println("");
                Serial.println("strobe brightness at max");
            }
            break;
        case ',':
            if (pins[STROBE_INDEX].get_step_off() > 0)
            {
                pins[STROBE_INDEX].set_step_off(pins[STROBE_INDEX].get_step_off() - 1);
                pins[STROBE_INDEX].set_step_on(pins[STROBE_INDEX].get_step_on() + 1);
                Serial.println("");
                Serial.println("strobe brightness decreased");
            } else
            {
                Serial.println("");
                Serial.println("strobe brightness at min");
            }
            break;

        /* ramp operations */
        case 'd':
            ramps[channel].set_delay(v);
            v = 0;
            Serial.println("");
            Serial.println("ramp delay set");
            break;
        case 'R':
            ramps[channel].set_pt1(v);
            Serial.println("");
            Serial.println("ramp pt1 set");
            v = 0;
            break;
        case 'r':
            ramps[channel].set_pt2(v);
            Serial.println("");
            Serial.println("ramp pt2 set");
            v = 0;
            break;
        case 't':
            ramps[channel].set_ttl(v);
            Serial.println("");
            Serial.println("ramp ttl set");
            v = 0;
            break;
        case 'F':
            ramps[channel].set_flip(true);
            Serial.println("");
            Serial.println("ramp flip on");
            break;
        case 'f':
            ramps[channel].set_flip(false);
            Serial.println("");
            Serial.println("ramp flip off");
            break;
        case 'L':
            ramps[channel].set_loop(true);
            Serial.println("");
            Serial.println("ramp loop on");
            break;
        case 'l':
            ramps[channel].set_loop(false);
            Serial.println("");
            Serial.println("ramp loop off");
            break;
        case 'E':
            ramps[channel].enable();
            Serial.println("");
            Serial.println("ramp enabled");
            break;
        case 'e':
            ramps[channel].disable();
            Serial.println("");
            Serial.println("ramp disabled");
            break;
        case 'G':
            for (uint8_t idx = 0; idx < DEVICE_COUNT; ++idx)
            {
                ramps[idx].enable();
            }
            Serial.println("");
            Serial.println("all ramps enabled");
            break;
        case 'g':
            for (uint8_t idx = 0; idx < DEVICE_COUNT; ++idx)
            {
                ramps[idx].disable();
            }
            Serial.println("");
            Serial.println("all ramps disabled");
            break;

        /* debugging output */
        case 'p':
            Serial.println("");
            for (uint8_t idx = 0; idx < DEVICE_COUNT; ++idx)
            {
                Serial.print(idx, DEC);
                Serial.print(": ");
                pins[idx].print();
                Serial.print(idx, DEC);
                Serial.print(": ");
                ramps[idx].print();
            }
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
}
#endif // PROMPT_ENABLE

/******************************************************************************
 ** Main loop
 ******************************************************************************/

void _loop(void)
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
        case 'm':
            md.setM1Speed(v);
            v = 0;
            Serial.println("M1 Speed Set.");
            break;
        case 'M':
            md.setM2Speed(v);
            v = 0;
            Serial.println("M2 Speed Set.");
            break;
        case 'b':
            md.setM1Brake(v);
            v = 0;
            Serial.println("M1 Brake Set.");
            break;
        case 'B':
            md.setM2Brake(v);
            v = 0;
            Serial.println("M2 Brake Set.");
            break;
    }
    Serial.print("Value: ");
    Serial.println(v);
}

void loop()
{
    for(;;)
    {
        for (uint8_t ch = 0; ch < DEVICE_COUNT; ++ch)
        {
            if (ramps[ch].is_enabled())
            {
                pins[ch].set_offset(ramps[ch].step());
            }
        }
        wdt_reset();
#if PROMPT_ENABLE
        Prompt();
#endif // PROMPT_ENABLE
    }
}

ISR(TIMER2_COMPA_vect) 
{
    pins.step();
}
