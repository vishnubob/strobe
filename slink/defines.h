#ifndef  __DEFINES_H__
#define  __DEFINES_H__

#define CHANNEL_COUNT   12
#define PHASE_COUNT     1024
#define BASE_FREQUENCY  80
#define CLOCK_FREQUENCY 72000000
#define BUFFER_SIZE     0x7F
#define BRIGHTNESS      8
#define MODE_COUNT      12

typedef unsigned int size_t;

typedef struct animation_info 
{
    int mode_number;
    int duration;
} animation_info_t;  


#endif //  __DEFINES_H__
