#ifndef __TIMERCONTROL_H__
#define __TIMERCONTROL_H__

#include "defines.h"

typedef struct pin_timer_channel
{
    uint8               pin;
    timer_dev_num       timer;
    uint8               channel;
    voidFuncPtr         isr;
} pin_timer_channel_t;

class TimerChannel
{
public:
    TimerChannel()
        : _actual_phase(0)
    {}

    inline void init(const pin_timer_channel_t *tpin);
    inline void push_back(uint16 relative_phase);
    inline uint16 pop_front();
    inline void isr(void);

private:
    uint16                  _actual_phase;
    RingBuffer<uint16>      _rbuf;
    timer_dev_num           _timer;
    uint8                   _channel;
    uint8                   _pin;
};

void configure_timers();

// Low level interrupts
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

#endif // __TIMERCONTROL_H__
