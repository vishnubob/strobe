#include "wirish.h"
#include "defines.h"
#include "TimerControl.h"

// animations
const animation_info_t animation_info[] =
{
    {0, 4}, {1, 2}, {2, 3}, {3, 4},
    {4, 12}, {5, 5}, {6, 5}, {7, 2},
    {7, 2}, {7, 2}, {7, 2}, {7, 2},
};

// TimerChannels
extern TimerChannel TimerChannels[CHANNEL_COUNT];

// overall counter
int timeSoFar = 0; 

// just arbitrarily large anyway, so use time units * PHASE_COUNT
int timeUntilChange;

int current_animation;
int mode_type;

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
uint32 phase[CHANNEL_COUNT];
uint32 previous_phase[CHANNEL_COUNT];
int startPosition[CHANNEL_COUNT];
int returnDistance[CHANNEL_COUNT];

/*******************************************************************************
 ** Setup / Loop
 ******************************************************************************/

void prime_buffers()
{
    for(int pc = 0; pc < PRELOAD_COUNT; ++pc)
    {
        // for each channel
        for(int ch = 0; ch < CHANNEL_COUNT; ++ch) 
        { 
            previous_phase[ch] = phase[ch];
            phase[ch] = calcNextFrame(ch);
            TimerChannels[ch].push_back(phase[ch] - previous_phase[ch]);
            SerialUSB.print("ch ");
            SerialUSB.print(ch);
            SerialUSB.print(" phase ");
            SerialUSB.println(phase[ch] - previous_phase[ch]);
        }
        timeSoFar++;
        timeUntilChange--;
    }
}

void setup()
{
    for(int ch = 0; ch < CHANNEL_COUNT; ++ch)
    {
        int _pin = ch_to_pin[ch];
        pinMode(_pin, OUTPUT);
        digitalWrite(_pin, LOW);
    }

    /*
    while(1)
    {
        if (SerialUSB.available()) 
        { 
            SerialUSB.read();
            break; 
        }
    }
    */

    for(int i = 0; i < CHANNEL_COUNT; i++) 
    {
        phase[i] = 0;
    }  
    configure_timers();

    /*
    SerialUSB.println("Gate1");
    while(1)
    {
        if (SerialUSB.available()) 
        { 
            SerialUSB.read();
            break; 
        }
    }
    */

    timeSoFar = 0;
    current_animation = 0;
    timeUntilChange = animation_info[current_animation].duration * PHASE_COUNT;
    prime_buffers();
    start_timers();

    /*
    SerialUSB.println("Gate2");
    while(1)
    {
        if (SerialUSB.available()) 
        { 
            SerialUSB.read();
            break; 
        }
    }
    */
}

void loop() 
{
    if (timeUntilChange > 0)
    {
        // for each channel
        for(int ch = 0; ch < CHANNEL_COUNT; ++ch) 
        { 
            // angular position
            previous_phase[ch] = phase[ch];
            phase[ch] = calcNextFrame(ch);
            TimerChannels[ch].push_back(phase[ch] - previous_phase[ch]);
            //SerialUSB.println(phase[ch]);
        }

        timeSoFar++;
        timeUntilChange--;
    } else
    {
        // advance animation step
        current_animation++;
        if(current_animation >= MODE_COUNT) 
        {
            // XXX: turn motor off, etc.. 
            // XXX: wait for reset button/footswitch/etc
            current_animation = 0; 
        }
        timeUntilChange = animation_info[current_animation].duration * PHASE_COUNT;
        timeSoFar = 0;
        SerialUSB.print("Mode: ");
        SerialUSB.println(current_animation);
    }
}

/*******************************************************************************
 ** Modes
 ******************************************************************************/

// note, delta must go evenly!!
uint32 fadeBetween(int begin, int end, int delta, int timeTillSlow, int slowDownDelay, long tsf, int channel) 
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

uint32 strobeStepping(int velocity, int delayBetweenSteps, long tsf, int channel) 
{
    //only update counters and such during channel 0, otherwise this gets called 12 times...
    if(channel == 0) 
    {
        if(tsf <= 1) 
        {
            //init
            auxModeCounter1 = 0;
            auxModeCounter2 = 0;
            auxModeCounter3 = velocity;
        } else 
        {
            //always store up what we would have moved (whether we move or not)
            //note, this will wrap around at 256 but that's okay
            //because the phase does too
            // XXX: PHASE DOES NOT WRAP AT 256
            auxModeCounter1 += velocity;

            //inc the first counter and if it hits our delay then add the velocity we've stored up
            if(auxModeCounter2 >= delayBetweenSteps) 
            {
                auxModeCounter2 = 0;
                //don't forget to add in the velocity used on this step!
                auxModeCounter3 = auxModeCounter1;
                //and clear the velocity store because we used it
                auxModeCounter1 = 0;
            } else 
            {
                auxModeCounter2++;
                auxModeCounter3 = 0;
            }
        }
    }
    return phase[channel]+auxModeCounter3;		
}

uint32 switchBetween(int slow, int fast, int timePerStep, int stepDelta, int minStep, int initialSetupTime, long tsf, int channel)
{
    // only update counters and such during channel 0, otherwise this gets called 12 times...
    if(channel == 0) 
    {
        if(tsf <= initialSetupTime) 
        {
            // init
            auxModeCounter1 = 0;
            auxModeCounter2 = slow;
            auxModeCounter3 = timePerStep;
        } else 
        {
            if(auxModeCounter1 >= auxModeCounter3) 
            {
                auxModeCounter1 = 0;
                if(auxModeCounter2 == slow) 
                {
                    auxModeCounter2 = fast;
                } else 
                {
                    auxModeCounter2 = slow;
                    // only when switching back to slow do we change time
                    if(auxModeCounter3 > minStep) 
                    {
                        auxModeCounter3 -= stepDelta;
                    }
                }
            } else 
            {
                auxModeCounter1++;
            }
        }
    }
    return phase[channel] + auxModeCounter2;
}

uint32 breakInTwoAndMove(long holdTime, int velocity, long tsf, int channel) 
{
    if(tsf <= holdTime) 
    {
        // break in two pieces down the center
        if(channel < 2) 
        {
            return 0;
        } else 
        {
            return (PHASE_COUNT / 2);
        }
    } else 
    {
        return phase[channel] + velocity;
    }
}

uint32 freezeAndFan(long setupTime, int deltaDistance, int stepDelay, int velocityInMiddle, int timeInMiddle,  long tsf, int channel) 
{
    if(channel == 0) 
    {
        if(tsf <= setupTime) 
        {
            //init
            auxModeCounter1 = 0;
            auxModeCounter2 = 0;
            auxModeCounter3 = 0;
            auxModeCounter4 = 0;
            auxModeCounter5 = 0;
            //the direction to move
            auxModeCounter6 = 1;
        } else 
        {
            // so, every stepDelay we want to advance by one all the guys that should be moving
            // how do we know who should be moving?
            // well, auxModeCounter1 is the guy we start at.
            // and if it's 0 then we only use half the delta distance because
            // we want everything evenly spaced (think about it)
            // then we can advance auxModeCounter1 ot stop moving those guys
            // also, things only happen every stepDelay number of steps,
            // so use auxCoutner3 for that
            // auxModeCounter4 is how much to advance by
            if(auxModeCounter1 < 6) 
            {
                if(auxModeCounter3 >= stepDelay) 
                {
                    auxModeCounter3 = 0;
                    // it's time to go a step,
                    // but which direction? back or forwards?
                    // well, it depends on what step of the thing we're in...
                    // how do we control
                    auxModeCounter4=auxModeCounter6;
                    // let's see if we made it all the way to where we go
                    if((auxModeCounter2 >= deltaDistance) || (auxModeCounter1 == 0 && auxModeCounter2 >= (deltaDistance >> 1))) 
                    {
                        auxModeCounter2 = 0;
                        //we made it...stop moving this guy
                        auxModeCounter1++;
                    } else 
                    {
                        auxModeCounter2++;
                    }					
                } else
                {
                    auxModeCounter3++;
                    auxModeCounter4 = 0;
                }
            } else 
            {
                // everything has gone already, so just rotate?
                // when do we go back the other direction?
                if(auxModeCounter6 == 1) 
                {
                    auxModeCounter5 = velocityInMiddle;
                } else 
                {
                    // we want to just stay in phase, so how do we do that?
                    auxModeCounter5=0;
                }

                auxModeCounter4 = 0;
                // this only needs to get set once and spin will be constant,
                // so then when it's time, we can set auxModeCounter to 1 again
                // so decide when to go back to doing the work...
                if(auxModeCounter3>=timeInMiddle) 
                {
                    auxModeCounter3 = 0;
                    // and tell it to flip the direction
                    if(auxModeCounter6 == 1) 
                    {
                        auxModeCounter6 = (PHASE_COUNT - 1);
                        // it's okay to reset and start again
                        auxModeCounter1 = 0;
                    } else 
                    {
                        auxModeCounter6 = 0;
                        // note, we don't reset auxModeCounter1
                        // because we don't want to go anywhere
                    }
                } else 
                {
                    auxModeCounter3++;
                }
            }
        }
    }

    if(channel <= (5 - auxModeCounter1)) 
    {
        return phase[channel] + auxModeCounter4 + auxModeCounter5;
    } else if(channel >= (6 + auxModeCounter1)) 
    {
        return phase[channel]- auxModeCounter4 + auxModeCounter5;
    } else 
    {
        return phase[channel] + auxModeCounter5;
    }
}

uint32 bumpAndGrind(int setupTime, int stepTime, int velocity, int stepsPerStrip, boolean useParity, long tsf, int channel)
{
    if(channel == 0) 
    {
        if(tsf<=setupTime) 
        {
            // init
            // the channel we're currently moving
            auxModeCounter1 = 0;	
            // how much the channel should move this step
            auxModeCounter2 = 0;	
            // the counter used to see if we're at stepTime
            auxModeCounter3 = 0;	
            // when this hits stepsPerStrip, we move channels
            auxModeCounter4 = 0;	
            auxModeCounter5 = 1;
        } else 
        {
            if(auxModeCounter2 == velocity) 
            {
                // we just did a step so
                auxModeCounter4++;
                if(auxModeCounter4 >= stepsPerStrip) 
                {
                    auxModeCounter4 = 0;
                    // advance the channel we're doing
                    auxModeCounter1 += auxModeCounter5;
                    if(auxModeCounter1 == CHANNEL_COUNT) 
                    {
                        auxModeCounter5 = (PHASE_COUNT - 1);
                        // advance twice to no duplicate the one just done
                        auxModeCounter1 += (auxModeCounter5 << 1);
                    } else if(auxModeCounter1 == (PHASE_COUNT - 1)) 
                    {
                        // we're done, so never move anything anymore...
                        velocity = 0;
                        auxModeCounter5 = 0;
                        // keep auxModeCounter just because
                    }

                }
            }
            // the !=(PHASE_COUNT - 1) check is to make sure we don't move once we've gone all the way through
            if(auxModeCounter3 >= stepTime && auxModeCounter1 != (PHASE_COUNT - 1)) 
            {
                auxModeCounter3 = 0;
                auxModeCounter2 = velocity;
            } else 
            {
                auxModeCounter3++;
                auxModeCounter2 = 0;
            }
        }
    }
    if(useParity) 
    {
        // this line is screwed up from translation from c?
        // if(tsf <= setupTime || (channel > auxModeCounter1 || ((channel ^ auxModeCounter1) & 1) == 1 )) 
        if(tsf <= setupTime || (channel > auxModeCounter1 || ((channel % 2) == (auxModeCounter1 % 2)))) 
        {
            return phase[channel];
        } else 
        {
            return phase[channel] + auxModeCounter2;
        }
    } else 
    {
        // if(tsf <= setupTime || auxModeCounter1 != channel)
        if((tsf <= setupTime) || (auxModeCounter1 != channel)) 
        {
            return phase[channel];
        } else 
        {
            return phase[channel] + auxModeCounter2;
        }
    }
}

uint32 freakOutAndComeTogether(int returnStepsPower, long tsf, int channel) 
{
    if(tsf == 0) 
    {
        // check this, must be even multiple of returnSteps
        // startPosition[channel]=rand() & (PHASE_COUNT - 1);	
        // 0 to (PHASE_COUNT - 1)
        startPosition[channel] = (int)random((PHASE_COUNT - 1)); 
        if(startPosition[channel] > (PHASE_COUNT / 2)) 
        {
            returnDistance[channel] = PHASE_COUNT - startPosition[channel];
        } else 
        {
            returnDistance[channel] = PHASE_COUNT - startPosition[channel];
            // weird it's the same, but I think it is
        }
        return startPosition[channel];
    } else if (tsf < (1 << returnStepsPower)) 
    {
        // return ((long)startPosition[channel]) + ( (returnDistance[channel]*tsf) >> (long)returnStepsPower );
        return startPosition[channel] + ((int)(returnDistance[channel] * tsf) >> returnStepsPower);
    } else 
    {
        return phase[channel];
    }
}

uint32 calcNextFrame(int channel) 
{
    switch(animation_info[current_animation].mode_number)
    {
        case 0:
            //return fadeBetween(40, 0, -1, 50, 20, timeSoFar, channel);
            return fadeBetween(160, 0, -4, 50, 80, timeSoFar, channel);
        case 1:
            //return fadeBetween(0, 3, 1, 0, 20, timeSoFar, channel);
            return fadeBetween(0, 12, 4, 0, 80, timeSoFar, channel);
        case 2:
            return strobeStepping(1, 13, timeSoFar, channel);
        case 3:
            // switchBetween(slow, fast, timePerStep, stepDelta, minStep, initialSetupTime, tsf, channel);
            // fast was 216 (=-40)
            return switchBetween(1, 83, 165, 40, 45, 165, timeSoFar, channel); 
        case 4:
            // freezeAndFan(setupTime, deltaDistance, stepDelay, velocityInMiddle, timeInMiddle, tsf, channel) {
            // deltaDistance was 20 originally
            return freezeAndFan(200, 20, 10, 2, 250, timeSoFar, channel); 
        case 5:
            // note, the velocity (8) * the stepsPerStrip (32) must = the phases steps per period (256)
            return bumpAndGrind(100, 0, 8, 32, false, timeSoFar, channel);
        case 6:
            return bumpAndGrind(0, 0, 8, 32, true, timeSoFar, channel);
        case 7:
            return freakOutAndComeTogether(7, timeSoFar, channel);
    }
    return 0;
}

