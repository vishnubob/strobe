#ifndef __TIMERCONTROL_H__
#define __TIMERCONTROL_H__

#include "defines.h"
#include "RingBuffer.h"

#define     STATE_OFF   0
#define     STATE_ON    1
#define     STATE_SPIN  2

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
        : _actual_phase(0), _spin_phase(0), _state(STATE_OFF)
    {}

    void init(const pin_timer_channel_t *tpin);
    void push_back(uint16 relative_phase);
    inline uint16 pop_front();
    inline void set_ocm(bool onoff);
    inline void isr(void);

private:
    uint32                  _actual_phase;
    uint32                  _spin_phase;
    RingBuffer<uint16>      _rbuf;
    timer_dev_num           _timer;
    uint8                   _channel;
    uint8                   _pin;
    uint8                   _state;
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
