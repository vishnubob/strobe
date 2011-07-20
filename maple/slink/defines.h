#ifndef  __DEFINES_H__
#define  __DEFINES_H__

/* constants */
#define CHANNEL_COUNT           12
#define PHASE_COUNT             1024
#define PHASE_SCALE_FACTOR      4
//#define TIMER_COUNT             (PHASE_COUNT * 32)
#define BASE_FREQUENCY          50
#define CLOCK_FREQUENCY         72000000
#define MODE_COUNT              12 
#define BUFFER_SIZE             400
#define PRELOAD_COUNT           (BUFFER_SIZE / 2)
//#define MOTOR_MAX_SPEED         22500
#define MOTOR_MAX_SPEED         390

/* configuration variables */
#define DEFAULT_BRIGHTNESS      3
#define ADDRESS_BRIGHTNESS      0x10
#define MAX_BRIGHTNESS          6
#define MIN_BRIGHTNESS          2

#define DEFAULT_PRESCALE        (CLOCK_FREQUENCY / (PHASE_COUNT * BASE_FREQUENCY))
#define ADDRESS_PRESCALE        0x11
//#define MAX_PRESCALE            (DEFAULT_PRESCALE + 200)
//#define MIN_PRESCALE            (DEFAULT_PRESCALE - 200)
#define MAX_PRESCALE        ((unsigned int)(CLOCK_FREQUENCY / (PHASE_COUNT * (BASE_FREQUENCY - 2))))
#define MIN_PRESCALE        ((unsigned int)(CLOCK_FREQUENCY / (PHASE_COUNT * (BASE_FREQUENCY + 2))))

/* pins */
#define MOTOR_PWM_PIN           7
#define LED_PIN                 13
#define POT_BRIGHTNESS_PIN      18
#define POT_PRESCALE_PIN        19
#define RANDOM_PIN              20
#define BUTTON_STARTUP_PIN      34
#define BUTTON_MAINTENANCE_PIN  33
#define MOTOR_EN_PIN            36

//#define SERIAL_DEBUG

typedef unsigned int size_t;

typedef struct animation_info 
{
    int mode_number;
    int duration;
} animation_info_t;  

const int ch_to_pin[] = {D2, D3, D1, D0, D12, D11, D27, D28, D5, D9, D14, D24};

#endif //  __DEFINES_H__
