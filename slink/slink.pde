#include "wirish.h"
#include "defines.h"
#include "TimerControl.h"

// TimerChannels
extern TimerChannel TimerChannels[CHANNEL_COUNT];

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
uint16 phase[CHANNEL_COUNT];
uint16 previousPhase[CHANNEL_COUNT];
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
    // initialize
    /*
    while(1)
    {
        if (SerialUSB.available()) { break; }
    }
    */
    for(int i = 0; i < CHANNEL_COUNT; i++) 
    {
        phase[i] = 0;
    }  
    configure_timers();
}

void loop() 
{
    timeUntilChange = 4 * PHASE_COUNT;
    timeSoFar = 0;
    while(timeUntilChange)
    {
        // for each strip
        for(int i = 0; i < CHANNEL_COUNT; ++i) 
        { 
            // save for printing relative coordinates
            previousPhase[i] = phase[i]; 
            // angular position
            phase[i] = fadeBetween(40, 0, -1, 50, 20, timeSoFar, i);
            TimerChannels[i].push_back(phase[i] - previousPhase[i]);
            TimerChannels[i].push_back(BRIGHTNESS);
        }

        timeSoFar++;
        timeUntilChange--;
    }
}