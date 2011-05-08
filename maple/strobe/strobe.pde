#include "wirish.h"

#define CHANNEL_COUNT   12
#define PHASE_COUNT     1024
#define BASE_FREQUENCY  80
#define CLOCK_FREQUENCY 72000000
#define BUFFER_SIZE     0x7F

typedef unsigned int size_t;

// from timers.h and boards.h

void timer2_ch1_interrupt(void);
void timer2_ch2_interrupt(void);
void timer2_ch3_interrupt(void);
void timer2_ch4_interrupt(void);
void timer3_ch1_interrupt(void);
void timer3_ch2_interrupt(void);
void timer3_ch3_interrupt(void);
void timer3_ch4_interrupt(void);
void timer4_ch1_interrupt(void);
void timer4_ch2_interrupt(void);
void timer4_ch3_interrupt(void);
void timer4_ch4_interrupt(void);

typedef struct pin_timer_channel
{
    uint8               pin;
    timer_dev_num       timer;
    uint8               channel;
    voidFuncPtr         isr;
} pin_timer_channel_t;

const pin_timer_channel_t ChannelMap[] = 
{
    // TIMER2
    {D2, TIMER2, 1, timer2_ch1_interrupt}, 
    {D3, TIMER2, 2, timer2_ch2_interrupt}, 
    {D1, TIMER2, 3, timer2_ch3_interrupt}, 
    {D0, TIMER2, 4, timer2_ch4_interrupt},

    // TIMER3
    {D12, TIMER3, 1, timer3_ch1_interrupt}, 
    {D11, TIMER3, 2, timer3_ch2_interrupt}, 
    {D27, TIMER3, 3, timer3_ch3_interrupt}, 
    {D28, TIMER3, 4, timer3_ch4_interrupt},

    // TIMER4
    {D5, TIMER4, 1, timer4_ch1_interrupt}, 
    {D9, TIMER4, 2, timer4_ch2_interrupt}, 
    {D14, TIMER4, 3, timer4_ch3_interrupt}, 
    {D24, TIMER4, 4, timer4_ch4_interrupt}
};

template <class T>
class RingBuffer
{
public:
    void init(size_t cap)
    {
        _size = 0;
        _capacity = cap;
        //_buffer = (T*)malloc(sizeof(T) * _capacity);
        _buffer_end = _buffer + _capacity;
        _head = _buffer;
        _tail = _head;
    }

    bool push_back(const T item)
    {
        if (_capacity == _size)
            return false;
        *_head = item;
        _head += 1;
        if (_head >= _buffer_end)
            _head = _buffer;
        _size += 1;
        return true;
    }

    T* pop_front()
    {
        if (_size == 0)
            return NULL;
        T* ret = _tail;
        _tail += 1;
        if(_tail >= _buffer_end)
            _tail = _buffer;
        _size -= 1;
        return ret;
    }

private:
    size_t      _size;
    size_t      _capacity;
    T           _buffer[BUFFER_SIZE];
    T           *_buffer_end;
    T           *_head;
    T           *_tail;
};

class TimerChannel
{
public:
    TimerChannel()
        : _actual_phase(0)
    {}

    void init(const pin_timer_channel_t *tpin)
    {
        _timer = tpin->timer;
        _channel = tpin->channel;
        _pin = tpin->pin;

        pinMode(_pin, PWM);
        timer_port *timer = timer_dev_table[_timer].base;
        timer_attach_interrupt(_timer, _channel, tpin->isr);
        switch(_channel)
        {
            case 1:
                timer->CCMR1 &= ~(0x00FF);
                timer->CCMR1 |= 0x0060;
                timer->CCER |= 0x0001;
                break;
            case 2:
                timer->CCMR1 &= ~(0xFF00);
                timer->CCMR1 |= 0x6000;
                timer->CCER |= 0x0010;
                break;
            case 3:
                timer->CCMR2 &= ~(0x00FF);
                timer->CCMR2 |= 0x0060;
                timer->CCER |= 0x0100;
                break;
            case 4:
                timer->CCMR2 &= ~(0xFF00);
                timer->CCMR2 |= 0x6000;
                timer->CCER |= 0x1000;
                break;
        }
    }

    void push_back(uint16 relative_phase)
    {
        _actual_phase = (relative_phase + _actual_phase) % PHASE_COUNT;
        _rbuf.push_back(_actual_phase);
    }

    uint16 pop_front()
    {
        return *(_rbuf.pop_front());
    }

    void isr(void) 
    {
        timer_set_compare_value(_timer, _channel, pop_front());
    } 

private:
    uint16                  _actual_phase;
    RingBuffer<uint16>      _rbuf;
    timer_dev_num           _timer;
    uint8                   _channel;
    uint8                   _pin;
};

TimerChannel TimerChannels[CHANNEL_COUNT];

void configure_timers()
{
    timer_port *timer2 = timer_dev_table[TIMER2].base;
    timer_port *timer3 = timer_dev_table[TIMER3].base;
    timer_port *timer4 = timer_dev_table[TIMER4].base;

    // Timer2
    Timer2.pause();
    Timer2.setPrescaleFactor(CLOCK_FREQUENCY / (PHASE_COUNT * BASE_FREQUENCY));
    Timer2.setOverflow(BASE_FREQUENCY - 1);

    // Timer3
    Timer3.pause();
    Timer3.setPrescaleFactor(CLOCK_FREQUENCY / (PHASE_COUNT * BASE_FREQUENCY));
    Timer3.setOverflow(BASE_FREQUENCY - 1);
    // Connect timer3 to timer2
    timer3->SMCR = 0x20;
    
    // Timer4
    Timer4.pause();
    Timer4.setPrescaleFactor(CLOCK_FREQUENCY / (PHASE_COUNT * BASE_FREQUENCY));
    Timer4.setOverflow(BASE_FREQUENCY - 1);
    // Connect timer4 to timer2
    timer4->SMCR = 0x20;

    // Turn on the timers
    timer2->CR2 |= 0x6;
}

void setup()
{
    configure_timers();
    for(int x = 0; x < CHANNEL_COUNT; ++x)
    {
        TimerChannels[x].init(ChannelMap + x);
    }
}

void loop() 
{
}

void timer2_ch1_interrupt(void) { TimerChannels[0].isr(); }
void timer2_ch2_interrupt(void) { TimerChannels[1].isr(); }
void timer2_ch3_interrupt(void) { TimerChannels[2].isr(); }
void timer2_ch4_interrupt(void) { TimerChannels[3].isr(); }
void timer3_ch1_interrupt(void) { TimerChannels[4].isr(); }
void timer3_ch2_interrupt(void) { TimerChannels[5].isr(); }
void timer3_ch3_interrupt(void) { TimerChannels[6].isr(); }
void timer3_ch4_interrupt(void) { TimerChannels[7].isr(); }
void timer4_ch1_interrupt(void) { TimerChannels[8].isr(); }
void timer4_ch2_interrupt(void) { TimerChannels[9].isr(); }
void timer4_ch3_interrupt(void) { TimerChannels[10].isr(); }
void timer4_ch4_interrupt(void) { TimerChannels[11].isr(); }

// Force init to be called *first*, i.e. before static object allocation.
// Otherwise, statically allocated object that need libmaple may fail.
__attribute__(( constructor )) void premain() 
{
    init();
}

int main(void) 
{
    setup();

    while (1) 
    {
        loop();
    }
    return 0;
}
