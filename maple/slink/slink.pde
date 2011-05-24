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
int32 timeSoFar = 0; 

// just arbitrarily large anyway, so use time units * PHASE_COUNT
int32 timeUntilChange;

int32 current_animation;
int32 current_mode;

// Start time
float t = 0.0;  

// these are some mode counters we use for different purposes in each mode
int32 auxModeCounter1;
int32 auxModeCounter2;
int32 auxModeCounter3;
int32 auxModeCounter4;
int32 auxModeCounter5;
int32 auxModeCounter6;
int32 auxModeCounter7;

// initialize phases:
int32 phase[CHANNEL_COUNT];
int32 previous_phase[CHANNEL_COUNT];
int32 startPosition[CHANNEL_COUNT];
int32 returnDistance[CHANNEL_COUNT];

/*******************************************************************************
 ** Setup / Loop
 ******************************************************************************/

void prime_buffers()
{
    for(int32 pc = 0; pc < PRELOAD_COUNT; ++pc)
    {
        // for each channel
        for(int32 ch = 0; ch < CHANNEL_COUNT; ++ch) 
        { 
            previous_phase[ch] = phase[ch];
            phase[ch] = calcNextFrame(ch);
            TimerChannels[ch].push_back(phase[ch] - previous_phase[ch]);
        }
        timeSoFar++;
        timeUntilChange--;
    }
}

void reset_runtime()
{
    /* configure timers */
    configure_timers();

    /* Turn off the PINs (safety) */
    for(int32 ch = 0; ch < CHANNEL_COUNT; ++ch)
    {
        phase[ch] = 0;
        previous_phase[ch] = 0;
    }  

    /* initialize runtime variables */
    timeSoFar = 0;
    current_animation = 0;
    timeUntilChange = animation_info[current_animation].duration * 256;
    current_mode = animation_info[current_animation].mode_number;

#ifndef SERIAL_DEBUG
    /* prime the ring buffers */
    prime_buffers();
    start_timers();
#endif
}

void setup()
{
#ifndef SERIAL_DEBUG
    /* Turn off USB */
    SerialUSB.end();
#endif
    /* random seed */
    pinMode(RANDOM_PIN, INPUT_ANALOG);
    randomSeed(analogRead(0));

    /* Debug LED */
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    /* reset runtime */
    reset_runtime();
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
            SerialUSB.print(phase[ch] - previous_phase[ch]);
            SerialUSB.print(" ");
#ifndef SERIAL_DEBUG
            TimerChannels[ch].push_back(phase[ch] - previous_phase[ch]);
#endif
        }

#ifdef SERIAL_DEBUG
        SerialUSB.print(timeUntilChange);
        SerialUSB.print(") ");
        for(int ch = 0; ch < CHANNEL_COUNT; ++ch) 
        { 
            SerialUSB.print(phase[ch] - previous_phase[ch]);
            SerialUSB.print(" ");
        }
        SerialUSB.print(" ");
#endif

        timeSoFar++;
        timeUntilChange--;
    } else
    {
        //digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        // advance animation step
        current_animation++;
        if(current_animation >= MODE_COUNT) 
        {
            // XXX: turn motor off, etc.. 
            // XXX: wait for reset button/footswitch/etc
            current_animation = 0; 
        }
        timeUntilChange = animation_info[current_animation].duration * 256;
        current_mode = animation_info[current_animation].mode_number;
        timeSoFar = 0;
#ifdef SERIAL_DEBUG
        SerialUSB.print("Mode: ");
        SerialUSB.println(current_mode);
#else
        /* flush the ring buffers */
        bool all_channels_empty = false;
        while(!all_channels_empty)
        {
            all_channels_empty = true;
            for(int ch = 0; ch < CHANNEL_COUNT; ++ch) 
            { 
                all_channels_empty &= TimerChannels[ch].is_empty();
            }
        }
#endif
        if (current_animation == 0)
        {
            reset_runtime();
        }
    }
}

// note, delta must go evenly!!
int32 fadeBetween(int32 begin, int32 end, int32 delta, int32 timeTillSlow, int32 slowDownDelay, int32 tsf, uint8 channel) 
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


int32 strobeStepping(int32 velocity, int32 delayBetweenSteps, int32 tsf, uint8 channel) 
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

int32 switchBetween(int32 slow, int32 fast, int32 timePerStep, int32 stepDelta, int32 minStep, int32 initialSetupTime, int32 tsf, uint8 channel)
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

int32 breakInTwoAndMove(int32 holdTime, int32 velocity, int32 tsf, uint8 channel) 
{
    if(tsf <= holdTime) 
    {
        // break in two pieces down the center
        if(channel < 2) 
        {
            return 0;
        } else 
        {
            return 128;
        }
    } else 
    {
        return phase[channel] + velocity;
    }
}

int32 freezeAndFan(int32 setupTime, int32 deltaDistance, int32 stepDelay, int32 velocityInMiddle, int32 timeInMiddle,  int32 tsf, uint8 channel) 
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
                        auxModeCounter6 = 255;
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

int32 bumpAndGrind(int32 setupTime, int32 stepTime, int32 velocity, int32 stepsPerStrip, boolean useParity, int32 tsf, uint8 channel)
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
                        auxModeCounter5 = 255;
                        // advance twice to no duplicate the one just done
                        auxModeCounter1 += (auxModeCounter5 << 1);
                    } else if(auxModeCounter1 == 255) 
                    {
                        // we're done, so never move anything anymore...
                        velocity = 0;
                        auxModeCounter5 = 0;
                        // keep auxModeCounter just because
                    }

                }
            }
            // the !=255 check is to make sure we don't move once we've gone all the way through
            if(auxModeCounter3 >= stepTime && auxModeCounter1 != 255) 
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
        //if(tsf <= setupTime || (channel > auxModeCounter1 || ((channel ^ auxModeCounter1) & 1) == 1 )) 
        if(tsf <= setupTime || (channel > auxModeCounter1 || ((channel % 2) == (auxModeCounter1 % 2)))) 
        {
            return phase[channel];
        } else 
        {
            return phase[channel] + auxModeCounter2;
        }
    } else 
    {
        if((tsf <= setupTime) || (auxModeCounter1 != channel)) 
        {
            return phase[channel];
        } else 
        {
            return phase[channel] + auxModeCounter2;
        }
    }
}

int32 freakOutAndComeTogether(int32 returnStepsPower, int32 tsf, uint8 channel) 
{
    if(tsf == 0) 
    {
        // check this, must be even multiple of returnSteps
        // startPosition[channel]=rand() & 255;	
        // 0 to 255
        startPosition[channel] = (int32)random(256); 
        returnDistance[channel] = 256 - startPosition[channel];
        return phase[channel] + startPosition[channel];
    } else if (tsf < (1 << returnStepsPower)) 
    {
        //return ((int32)startPosition[channel]) + ( (returnDistance[channel]*tsf) >> (int32)returnStepsPower );
        return phase[channel] + startPosition[channel] + ((int32)(returnDistance[channel] * tsf) >> returnStepsPower);
    } else 
    {
        return phase[channel];
    }
}


/*******************************************************************************
 ** Modes
 ******************************************************************************/

int32 calcNextFrame(uint8 channel) 
{
    switch(current_mode)
    {
        case 0:
            return fadeBetween(40, 0, -1, 50, 20, timeSoFar, channel);
        case 1:
            return fadeBetween(0, 3, 1, 0, 20, timeSoFar, channel);
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

