// Width of entire wave
int w;              
int segments = 12;
// which resonant vibration mode
int modeNumber = 5; 
// maximum (based on bit depth)
float phaseMax = 256.0; 

// animation variables
//int currentMode = 0; // what mode are we on?
// what frame on this mode are we on?
int currentAnimationStep = 0; 
// overall counter
int timeSoFar = 0; 
// just arbitrarily large anyway, so use time units * 256
int timeUntilChange = 4 * 256; 
 
// Start time
float t = 0.0;  
// Height of wave
float amplitude = 50.0;  
// How many pixels before the wave repeats
float period = 500.0;  
// Value for incrementing X, a function of period and xspacing
float dx;  
// geometric phases
float[] theta, a; 
// Using an array to store height values for the wave
float[] yvalues;  
// array of phases for animation
int[] phase; 
int[] previousPhase;

// these things I need for freakOutAndComeTogether
int[] startPosition;
long[] returnDistance;

// these are some mode counters we use for different purposes in each mode
int auxModeCounter1;
int auxModeCounter2;
int auxModeCounter3;
int auxModeCounter4;
int auxModeCounter5;
int auxModeCounter6;
int auxModeCounter7;

// the record of the full animation
class AnimationRecord 
{
    int modeType;
    int duration;

    // each step of this array has the type and how long
    AnimationRecord(int tempModeType, int tempDuration) 
    { 
        modeType = tempModeType;
        duration = tempDuration;
    }
}  

// each curent state has its type and time, for fading?
class ModeState
{
    long timeSoFar;
    int modeType;

    ModeState() 
    {
        modeType = 0;
        timeSoFar = 0;
    }
}

ModeState thisModeState = new ModeState(); 
ModeState nextModeState = new ModeState();

final int MAX_ANIMATION_STEPS = 12;

AnimationRecord[] animationRoutine = new AnimationRecord[MAX_ANIMATION_STEPS];

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void setup() 
{
    size(600, 150);
    // roughly the actual... this should be set accordingly to watch animations play out as they will
    frameRate(60); 
    colorMode(RGB, 255, 255, 255, 100);
    smooth();
    w = width;
    dx = (TWO_PI / period);
    yvalues = new float[w];

    // set envelope phase (sets which harmonic is being driven etc)
    // theta = new float[yvalues.length];
    a = new float[yvalues.length];
    for(int j = 0; j < yvalues.length; j++) 
    {
        a[j] = amplitude * sin((float)j * modeNumber * PI / yvalues.length);
    }    

    // initialize phases:
    phase = new int[segments];
    previousPhase = new int[segments];
    startPosition = new int[segments];
    returnDistance = new long[segments];
    
    // initialize
    for(int i = 0; i < segments; i++) 
    {
        phase[i] = 0;
    }  

    // animation layout
    animationRoutine[0] = new AnimationRecord(0, 4);
    animationRoutine[1] = new AnimationRecord(1, 2);
    animationRoutine[2] = new AnimationRecord(2, 3);
    animationRoutine[3] = new AnimationRecord(3, 4);
    animationRoutine[4] = new AnimationRecord(4, 12);
    animationRoutine[5] = new AnimationRecord(5, 5);
    animationRoutine[6] = new AnimationRecord(6, 5);
    animationRoutine[7] = new AnimationRecord(7, 2);
    animationRoutine[8] = new AnimationRecord(7, 2);
    animationRoutine[9] = new AnimationRecord(7, 2);
    animationRoutine[10] = new AnimationRecord(7, 2);
    animationRoutine[11] = new AnimationRecord(7, 2);

    // animation stuff
    currentAnimationStep = 0;
    thisModeState.timeSoFar = 0;
    thisModeState.modeType = animationRoutine[currentAnimationStep].modeType;
    timeUntilChange = animationRoutine[currentAnimationStep].duration * 256;
}

boolean step = false;
boolean keep_going = true;

void draw() 
{
    while((!step) && (!keep_going))
    {
        return;
    }

    // nice blue
    background(132, 112, 255); 

    // calculate all the next frame info
    if(timeUntilChange > 0) 
    {
        // for each strip
        for(int i = 0; i< segments ; i++) 
        { 
            previousPhase[i] = phase[i]; // save for printing relative coordinates
            phase[i] = calcNextFrame(thisModeState, i); // angular position
        }
        thisModeState.timeSoFar++;
        timeUntilChange--;
    } else 
    {
        // advance animation step
        currentAnimationStep++;
        if(currentAnimationStep >= MAX_ANIMATION_STEPS) 
        {
            // turn motor off, etc.. 
            // wait for reset button/footswitch/etc
            // this starts it over... instead, how to make it stop?
            currentAnimationStep = 0; 
        }

        timeUntilChange = animationRoutine[currentAnimationStep].duration * 256;
        thisModeState.modeType = animationRoutine[currentAnimationStep].modeType;
        thisModeState.timeSoFar = 0;
    }

    // draw it
    renderWaveSegments(); // draw slink segments
    renderLines(); // draw separators

    // write out useful info:
    fill(50, 50, 50);
    text("Time Until Change: " + (int)(timeUntilChange / 60), 5, 140);
    text("This modeType: " + thisModeState.modeType, 5, 120);
    for(int j = 0 ; j < segments ; j++)
    {
        //text(phase[j] - previousPhase[j], width / segments * j + 5, 15); 
        text(phase[j], width / segments * j + 5, 15); 
    }
    step = false;
}

void mouseClicked()
{
    step = true;
}

void keyPressed() 
{
    keep_going = !keep_going;
}

int calcNextFrame(ModeState myModeState, int strip) 
{
    switch(myModeState.modeType) 
    {
        case 0:
            return fadeBetween(40, 0, -1, 50, 20, myModeState.timeSoFar, strip);
        case 1:
            return fadeBetween(0, 3, 1, 0, 20, myModeState.timeSoFar, strip);
        case 2:
            return strobeStepping(1, 13, myModeState.timeSoFar, strip);
        case 3:
            // switchBetween(slow, fast, timePerStep, stepDelta, minStep, initialSetupTime, tsf, strip);
            // fast was 216 (=-40)
            return switchBetween(1, 83, 165, 40, 45, 165, myModeState.timeSoFar, strip); 
        case 4:
            // freezeAndFan(setupTime, deltaDistance, stepDelay, velocityInMiddle, timeInMiddle, tsf, strip) {
            // deltaDistance was 20 originally
            return freezeAndFan(200, 20, 10, 2, 250, myModeState.timeSoFar, strip); 
        case 5:
            // note, the velocity (8) * the stepsPerStrip (32) must = the phases steps per period (256)
            return bumpAndGrind(100, 0, 8, 32, false, myModeState.timeSoFar, strip);
        case 6:
            return bumpAndGrind(0, 0, 8, 32, true, myModeState.timeSoFar, strip);
        case 7:
            return freakOutAndComeTogether(7, myModeState.timeSoFar, strip);
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
// RENDER DESCRIPTIONS                                                  //
//////////////////////////////////////////////////////////////////////////

void renderLines() 
{
    // draw the 11 separating lines for the sine wave components
    for(int i = 1; i <= segments ; i++) 
    {
        stroke(50);
        line(width / segments * i, 0, width/segments * i, height);
    }
}

void renderWaveSegments() 
{
    // render all twelve waves
    for(int k = 0; k < segments ; k++) 
    {
        // each segment:
        renderWave(k);
    }
}

void renderWave(int k) 
{
    // A simple way to draw the wave with an ellipse at each location, for each section k
    for (int i = 0; i < width / segments; i++) 
    {
        // which segments are we on: k
        // calc it
        // phase[i] is set by animation 0-1024
        yvalues[i] = a[i + k * width / segments] * sin((float)phase[k] / phaseMax * 2 * PI); 

        // render it
        noStroke();
        fill(0);
        ellipseMode(CENTER);
        // h/2 centers thiis
        ellipse((i + k * width / segments), height / 2 + yvalues[i], 3, 3); 
    }
}



//////////////////////////////////////////////////////////////////////////
// MODE DESCRIPTIONS                                                    //
//////////////////////////////////////////////////////////////////////////


// note, delta must go evenly!!
int fadeBetween(int begin, int end, int delta, int timeTillSlow, int slowDownDelay, long tsf, int strip) 
{
    // only update counters and such during strip 0, otherwise this gets called 12 times...
    if(strip == 0)
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
        return phase[strip];
    } else 
    {
        return phase[strip] + auxModeCounter3;
    }
}


int strobeStepping(int velocity, int delayBetweenSteps, long tsf, int strip) 
{
    //only update counters and such during strip 0, otherwise this gets called 12 times...
    if(strip == 0) 
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
    return phase[strip]+auxModeCounter3;		
}

int switchBetween(int slow, int fast, int timePerStep, int stepDelta, int minStep, int initialSetupTime, long tsf, int strip)
{
    // only update counters and such during strip 0, otherwise this gets called 12 times...
    if(strip == 0) 
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
    return phase[strip] + auxModeCounter2;
}

int breakInTwoAndMove(long holdTime, int velocity, long tsf, int strip) 
{
    if(tsf <= holdTime) 
    {
        // break in two pieces down the center
        if(strip < 2) 
        {
            return 0;
        } else 
        {
            return 128;
        }
    } else 
    {
        return phase[strip] + velocity;
    }
}

int freezeAndFan(long setupTime, int deltaDistance, int stepDelay, int velocityInMiddle, int timeInMiddle,  long tsf, int strip) 
{
    if(strip == 0) 
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

    if(strip <= (5 - auxModeCounter1)) 
    {
        return phase[strip] + auxModeCounter4 + auxModeCounter5;
    } else if(strip >= (6 + auxModeCounter1)) 
    {
        return phase[strip]- auxModeCounter4 + auxModeCounter5;
    } else 
    {
        return phase[strip] + auxModeCounter5;
    }
}

int bumpAndGrind(int setupTime, int stepTime, int velocity, int stepsPerStrip, boolean useParity, long tsf, int strip)
{
    if(strip == 0) 
    {
        if(tsf<=setupTime) 
        {
            // init
            // the strip we're currently moving
            auxModeCounter1 = 0;	
            // how much the strip should move this step
            auxModeCounter2 = 0;	
            // the counter used to see if we're at stepTime
            auxModeCounter3 = 0;	
            // when this hits stepsPerStrip, we move strips
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
                    // advance the strip we're doing
                    auxModeCounter1 += auxModeCounter5;
                    if(auxModeCounter1 == segments) 
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
        // if(tsf <= setupTime || (strip > auxModeCounter1 || ((strip ^ auxModeCounter1) & 1) == 1 )) 
        if(tsf <= setupTime || (strip > auxModeCounter1 || ((strip % 2) == (auxModeCounter1 % 2)))) 
        {
            return phase[strip];
        } else 
        {
            return phase[strip] + auxModeCounter2;
        }
    } else 
    {
        // if(tsf <= setupTime || auxModeCounter1 != strip)
        if((tsf <= setupTime) || (auxModeCounter1 != strip)) 
        {
            return phase[strip];
        } else 
        {
            return phase[strip] + auxModeCounter2;
        }
    }
}

int freakOutAndComeTogether(int returnStepsPower, long tsf, int strip) 
{
    int tempPos;

    if(tsf == 0) 
    {
        // check this, must be even multiple of returnSteps
        // startPosition[strip]=rand() & 255;	
        // 0 to 255
        startPosition[strip] = (int)random(255); 
        if(startPosition[strip] > 128) 
        {
            returnDistance[strip] = 256 - startPosition[strip];
        } else 
        {
            returnDistance[strip] = 256 - startPosition[strip];
            // weird it's the same, but I think it is
        }
        return startPosition[strip];
    } else if (tsf < (1 << returnStepsPower)) 
    {
        // return ((long)startPosition[strip]) + ( (returnDistance[strip]*tsf) >> (long)returnStepsPower );
        return startPosition[strip] + ((int)(returnDistance[strip] * tsf) >> returnStepsPower);
    } else 
    {
        return phase[strip];
    }
}

