#ifndef  __DEFINES_H__
#define  __DEFINES_H__

#define CHANNEL_COUNT           12
#define PHASE_COUNT             1024
#define PHASE_SCALE_FACTOR      4
#define TIMER_COUNT             (PHASE_COUNT * 32)
#define BASE_FREQUENCY          50
#define CLOCK_FREQUENCY         72000000
#define BRIGHTNESS              3
#define MODE_COUNT              12 
#define BUFFER_SIZE             400
#define PRELOAD_COUNT           (BUFFER_SIZE / 2)
#define LED_PIN                 13
#define RANDOM_PIN              20
#define BUTTON_PIN              34
#define MOTOR_PWM_PIN           7
#define MOTOR_EN_PIN            36
#define MOTOR_MAX_SPEED         22500
//#define SERIAL_DEBUG

typedef unsigned int size_t;

typedef struct animation_info 
{
    int mode_number;
    int duration;
} animation_info_t;  

const int ch_to_pin[] = {D2, D3, D1, D0, D12, D11, D27, D28, D5, D9, D14, D24};

#endif //  __DEFINES_H__
