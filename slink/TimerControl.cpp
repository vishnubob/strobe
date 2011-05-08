#include "wirish.h"
#include "TimerControl.h"

// TimerChannel
TimerChannel TimerChannels[CHANNEL_COUNT];

// Channel Map
// from timers.h and boards.h
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

// TimerChannel

void TimerChannel::init(const pin_timer_channel_t *tpin)
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

inline void TimerChannel::push_back(uint16 relative_phase)
{
    _actual_phase = (relative_phase + _actual_phase) % PHASE_COUNT;
    _rbuf.push_back(_actual_phase);
}

inline uint16 TimerChannel::pop_front()
{
    return *(_rbuf.pop_front());
}

inline void TimerChannel::isr(void) 
{
    timer_set_compare_value(_timer, _channel, pop_front());
} 

// Configure Timers
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

    // Initialize the Timer Channels
    for(int x = 0; x < CHANNEL_COUNT; ++x)
    {
        TimerChannels[x].init(ChannelMap + x);
    }
}


/* low level interrupts */
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
