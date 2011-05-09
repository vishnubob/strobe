#include "wirish.h"
#include "defines.h"
#include "TimerControl.h"

// overall counter
int timeSoFar = 0; 

// just arbitrarily large anyway, so use time units * 256
int timeUntilChange = 4 * PHASE_COUNT; 

// Start time
float t = 0.0;  

// these are some mode counters we use for different purposes in each mode
int auxModeCounter1;
int auxModeCounter2;
int auxModeCounter3;
int auxModeCounter4;
int auxModeCounter5;
int auxModeCounter6;
int auxModeCounter7;

// initialize phases:
int phase[CHANNEL_COUNT];
int previousPhase[CHANNEL_COUNT];
int startPosition[CHANNEL_COUNT];
int returnDistance[CHANNEL_COUNT];

// note, delta must go evenly!!
int fadeBetween(int begin, int end, int delta, int timeTillSlow, int slowDownDelay, long tsf, int channel) 
{
    // only update counters and such during channel 0, otherwise this gets called 12 times...
    if(channel == 0)
    {
        if(tsf < timeTillSlow) 
        {
            // init stuff
            auxModeCounter1 = 0;
            auxModeCounter2 = begin;
            auxModeCounter3 = slowDownDelay;
        } else 
        {
            // time to slow down
            auxModeCounter1++;
            if(auxModeCounter1 >= auxModeCounter2) 
            {
                auxModeCounter1 = 0;
                if(auxModeCounter3 != end) 
                {
                    auxModeCounter3 += delta;
                }
            }
        }
    }

    if(tsf <= 1) 
    {
        // start at whereever you were
        return phase[channel];
    } else 
    {
        return phase[channel] + auxModeCounter3;
    }
}


void setup()
{
    configure_timers();

    // initialize
    for(int i = 0; i < CHANNEL_COUNT; i++) 
    {
        phase[i] = 0;
    }  
}

void loop() 
{
    timeUntilChange = 4 * PHASE_COUNT;
    timeSoFar = 0;
    while(timeUntilChange)
    {
        // for each strip
        for(int i = 0; i < PHASE_COUNT; ++i) 
        { 
            // save for printing relative coordinates
            previousPhase[i] = phase[i]; 
            // angular position
            phase[i] = fadeBetween(40, 0, -1, 50, 20, timeSoFar, i);
        }

        timeSoFar++;
        timeUntilChange--;
    }
}

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
