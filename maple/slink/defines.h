#ifndef  __DEFINES_H__
#define  __DEFINES_H__

#define CHANNEL_COUNT           12
#define PHASE_COUNT             1024
#define PHASE_SCALE_FACTOR      4
#define TIMER_COUNT             (PHASE_COUNT * 4)
#define BASE_FREQUENCY          45
#define CLOCK_FREQUENCY         72000000
#define BRIGHTNESS              5
#define MODE_COUNT              4 
#define BUFFER_SIZE             125
#define PRELOAD_COUNT           50

typedef unsigned int size_t;

typedef struct animation_info 
{
    int mode_number;
    int duration;
} animation_info_t;  

const int ch_to_pin[] = {D2, D3, D1, D0, D12, D11, D27, D28, D5, D9, D14, D24};

#endif //  __DEFINES_H__
