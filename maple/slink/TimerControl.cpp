#include "wirish.h"
#include "TimerControl.h"

// TimerChannel
TimerChannel TimerChannels[CHANNEL_COUNT];

// Brightness
uint16 BRIGHTNESS;

// Prescale
uint16 PRESCALE;

// Timer Count
uint16 TIMER_COUNT;

// Channel Map
// This associates a Pin with a timer, a channel, and a compare interrupt.
// It is used to initialize the individual TimerChannel objects.
// Pin mappings are sourced from libmaple/timers.h and libmaple/boards.h
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
    digitalWrite(tpin->pin, LOW);

    _timer = tpin->timer;
    _channel = tpin->channel;
    _pin = tpin->pin;
    _last_phase = 0;
    _state = STATE_OFF;

    pinMode(_pin, PWM);
    timer_port *timer = timer_dev_table[_timer].base;
    switch(_channel)
    {
        case 1:
            timer->CCMR1 &= ~(0x00FF);
            timer->CCMR1 |= 0x0040;
            timer->CCER |= 0x0001;
            break;
        case 2:
            timer->CCMR1 &= ~(0xFF00);
            timer->CCMR1 |= 0x4000;
            timer->CCER |= 0x0010;
            break;
        case 3:
            timer->CCMR2 &= ~(0x00FF);
            timer->CCMR2 |= 0x0040;
            timer->CCER |= 0x0100;
            break;
        case 4:
            timer->CCMR2 &= ~(0xFF00);
            timer->CCMR2 |= 0x4000;
            timer->CCER |= 0x1000;
            break;
    }
    timer_attach_interrupt(_timer, _channel, tpin->isr);
    timer_set_compare_value(_timer, _channel, 0);
}

bool TimerChannel::is_empty()
{
    return _rbuf.is_empty();
}


// This method is called by the user-code to push 
// phase information to this channel.
void TimerChannel::push_back(int16 relative_phase)
{
    _rbuf.push_back(relative_phase);
}

// This method is called by the interrupt service routine
// to schedule the next phase offset.
inline int16 TimerChannel::pop_front()
{
    int16 *phase = _rbuf.pop_front();
    if(phase == NULL)
    {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        if ((_last_phase % PHASE_COUNT) != 0)
        {
            _last_phase = (((_last_phase / PHASE_COUNT) + 1) * PHASE_COUNT) % TIMER_COUNT;
        }
        return 0;
    } 
    return *phase;
}

// This method manipulates the OC?M bits that dictate how a compare
// interrupt should affect its bound pin.  By setting onoff to true,
// a compare interrupt will set the mapped pin to high.  By setting
// onoff to false, a compare interrupt will set the mapped pin to low.
inline void TimerChannel::set_ocm(bool onoff)
{
    timer_port *timer = timer_dev_table[_timer].base;
    if (onoff)
    {
        switch(_channel)
        {
            case 1:
                timer->CCMR1 &= ~(0x00FF);
                timer->CCMR1 |= 0x0010;
                break;
            case 2:
                timer->CCMR1 &= ~(0xFF00);
                timer->CCMR1 |= 0x1000;
                break;
            case 3:
                timer->CCMR2 &= ~(0x00FF);
                timer->CCMR2 |= 0x0010;
                break;
            case 4:
                timer->CCMR2 &= ~(0xFF00);
                timer->CCMR2 |= 0x1000;
                break;
        }
    } else
    {
        switch(_channel)
        {
            case 1:
                timer->CCMR1 &= ~(0x00FF);
                timer->CCMR1 |= 0x0020;
                break;
            case 2:
                timer->CCMR1 &= ~(0xFF00);
                timer->CCMR1 |= 0x2000;
                break;
            case 3:
                timer->CCMR2 &= ~(0x00FF);
                timer->CCMR2 |= 0x0020;
                break;
            case 4:
                timer->CCMR2 &= ~(0xFF00);
                timer->CCMR2 |= 0x2000;
                break;
        }
    }
}

// The interrupt service routine is called at a compare event.
// It is a flip-flop state machine that schedules the next timing
// event, depending on the previous state.  If we are off, we pull
// the next relative phase from the ring buffer and configure the
// compare to turn off the mapped pin.  If we are 
inline void TimerChannel::isr(void) 
{
    int32 next_phase;

    switch(_state)
    {
        // we are currently off
        case STATE_OFF:
            set_ocm(true);
            next_phase = ((pop_front() - 128 + 512) % 256) + 128;
            next_phase = ((next_phase * PHASE_SCALE_FACTOR) + _last_phase) % TIMER_COUNT;
            //next_phase = ((pop_front() * PHASE_SCALE_FACTOR) + _last_phase + PHASE_COUNT) % TIMER_COUNT;
            timer_set_compare_value(_timer, _channel, next_phase);
            _state = STATE_ON;
            _last_phase = next_phase;
            break;
        case STATE_ON:
            set_ocm(false);
            next_phase = (_last_phase + BRIGHTNESS) % TIMER_COUNT;
            timer_set_compare_value(_timer, _channel, next_phase);
            _state = STATE_OFF;
            break;
    }
} 

void set_prescale(bool sync)
{
    if (sync)
    {
        // Busy wait until we are sure we can update all prescale values
        // before an update event
        while (Timer2.getCount() > (PHASE_COUNT - 100))
        {}
    }
    Timer2.setPrescaleFactor(PRESCALE);
    Timer3.setPrescaleFactor(PRESCALE);
    Timer4.setPrescaleFactor(PRESCALE);
}

// Configure Timers
void configure_timers(bool uev_enable)
{
    timer_port *timer2 = timer_dev_table[TIMER2].base;
    timer_port *timer3 = timer_dev_table[TIMER3].base;
    timer_port *timer4 = timer_dev_table[TIMER4].base;

    stop_timers();

    // Timer2
    //Timer2.setPrescaleFactor(CLOCK_FREQUENCY / (PHASE_COUNT * BASE_FREQUENCY));
    Timer2.setOverflow(TIMER_COUNT - 1);
    // Timer2 is configured as a master
    timer2->CR2 |= (1 << 4);

    // Timer3 */
    //Timer3.setPrescaleFactor(CLOCK_FREQUENCY / (PHASE_COUNT * BASE_FREQUENCY));
    Timer3.setOverflow(TIMER_COUNT - 1);
    // Connect timer3 to timer2 (ITR1), trigger mode
    timer3->SMCR = (1 << 4) | 6;
    
    // Timer4 */
    //Timer4.setPrescaleFactor(CLOCK_FREQUENCY / (PHASE_COUNT * BASE_FREQUENCY));
    Timer4.setOverflow(TIMER_COUNT - 1);
    // Connect timer4 to timer2 (ITR1), trigger mode
    timer4->SMCR = (1 << 4) | 6;

    // Select Update request source to be driven by overflow
    if (uev_enable)
    {
        timer4->CR1 |= (1 << 2);
        timer2->CR1 |= (1 << 2);
        timer3->CR1 |= (1 << 2);
    }

    // Set the timer prescales
    set_prescale();

    // Initialize the Timer Channels
    for(int x = 0; x < CHANNEL_COUNT; ++x)
    {
        TimerChannels[x].init(ChannelMap + x);
    }
}

void start_timers()
{
    // Turn on all timers.
    // Timer3 and Timer4 are linked to Timer2
    Timer2.resume();
}

void stop_timers()
{
    // stop the timers
    Timer2.pause();
    Timer3.pause();
    Timer4.pause();

    // Counters are set to non-zero to force a cycle before
    // the first compare/interrupt occurs.
    Timer2.setCount(0);
    Timer3.setCount(0);
    Timer4.setCount(0);
}

// Low level interrupt wrappers
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
