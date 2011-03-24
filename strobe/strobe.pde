// pins
#define PIN1_PIN            2
#define PIN2_PIN            3
#define PIN3_PIN            4
#define PIN4_PIN            5
#define PIN5_PIN            6
#define PIN6_PIN            7
#define PIN7_PIN            8
#define PIN8_PIN            9
#define PIN9_PIN            10
#define PIN10_PIN           11 
#define PIN11_PIN           12
#define PIN12_PIN           13
#define PIN13_PIN           14
#define PIN14_PIN           15
#define PIN15_PIN           16
#define PIN16_PIN           17

// configurable options
//#define PIN_COUNT           16
#define PIN_COUNT           4 
#define SLICE_COUNT         512
#define STROBE_FREQ         60
#define CPU_FREQ            16000000

// pin map
const int _pin_map[] = {PIN1_PIN, PIN2_PIN, PIN3_PIN, PIN4_PIN,
                        PIN5_PIN, PIN6_PIN, PIN7_PIN, PIN8_PIN,
                        PIN9_PIN, PIN10_PIN, PIN11_PIN, PIN12_PIN,
                        PIN13_PIN, PIN14_PIN, PIN15_PIN, PIN16_PIN};


// Helper macros for frobbing bits
#define bitset(var,bitno) ((var) |= (1 << (bitno)))
#define bitclr(var,bitno) ((var) &= ~(1 << (bitno)))
#define bittst(var,bitno) (var& (1 << (bitno)))

/******************************************************************************
 ** Pin
 ******************************************************************************/

class Pin
{
public:
    void init(unsigned char pin)
    { 
        _pin = pin;
        pinMode(_pin, OUTPUT);
        _step_on = 0;
        _step_off = 0;
    }

    void inline step(unsigned int step_val)
    {
        if(step_val == _step_on) { on(); }
        if(step_val == _step_off) { off(); }
    }

    /* setters */
    void inline on() { digitalWrite(_pin, HIGH); }
    void inline off() { digitalWrite(_pin, LOW); }
    void inline set_on(unsigned int step) { _step_on = step; }
    void inline set_off(unsigned int step) { _step_off = step; }
    void inline set_on_off(unsigned int step_on, unsigned int step_off) 
    { 
        set_on(step_on);
        set_off(step_off);
    }
    
    /* getters */
    unsigned char inline get_on() const { return _step_on; }
    unsigned char inline get_off() const { return _step_off; }
    
private:
    unsigned char _pin;
    volatile unsigned int _step_on;
    volatile unsigned int _step_off;
};

/******************************************************************************
 ** PinSet
 ******************************************************************************/

class PinSet
{
public:
    PinSet()
    {
        for(unsigned char idx = 0; idx < PIN_COUNT; ++idx) 
        {
            _pins[idx].init(_pin_map[idx]);
        }
    }

    void set_on_off(unsigned int _on, unsigned int _off)
    {
        for(unsigned char idx = 0; idx < PIN_COUNT; ++idx) 
        {
            _pins[idx].set_on_off(_on, _off);
        }
    }

    void inline step(unsigned int step_val)
    {
        for(unsigned char idx = 0; idx < PIN_COUNT; ++idx)
        {
            _pins[idx].step(step_val);
        }
    }

    Pin &operator[] (unsigned char idx) { return _pins[idx]; }

private:
    Pin _pins[PIN_COUNT];
};

/******************************************************************************
 ** Globals
 ******************************************************************************/

PinSet                      pins;
volatile unsigned int       GlobalStep;


/******************************************************************************
 ** Setup
 ******************************************************************************/

void test_config(void)
{
    unsigned int offset = 0;
    for(unsigned char idx = 0; idx < PIN_COUNT; ++idx)
    {
        unsigned int on = offset;
        //offset += (SLICE_COUNT / PIN_COUNT);
        offset += 1;
        unsigned int off = (offset >= SLICE_COUNT) ? 0 : offset;
        pins[idx].set_on_off(on, off);
    }

}

void setup(void)
{
    // setup serial
    Serial.begin(115200);
    test_config();

    // disable global interrupts
    cli();

    // disable the timer0 interrupt
    // we need the cycles!
    bitclr(TIMSK0, TOIE0);

    // reset global step
    GlobalStep = 0;

    // setup timer1 - 16bit
    TCCR1A = 0;
    TCCR1B = 0;
    // select CTC mode
    bitset(TCCR1B, WGM12);
    // 1:1
    bitset(TCCR1B, CS10);
    // set CTC overflow
    OCR1A = CPU_FREQ / SLICE_COUNT / STROBE_FREQ;
    // enable compare interrupt
    bitset(TIMSK1, OCIE1A);

    // enable global interrupts
    sei();

    Serial.println("SetupHardware()");
    Serial.flush();
}

/******************************************************************************
 ** Main Loop
 ******************************************************************************/

// Top level loop, call by the Arduino core
void loop(void)
{
}

/******************************************************************************
 ** Timer Interrupt
 ******************************************************************************/

ISR(TIMER1_COMPA_vect) 
{
    pins.step(GlobalStep);
    GlobalStep = (++GlobalStep < SLICE_COUNT) ? GlobalStep : 0;
}
