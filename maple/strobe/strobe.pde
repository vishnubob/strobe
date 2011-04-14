#define TIM2_CH1    2

void handler_led(void);
void handler_count2(void);

void handler1(void);
int count;
int last_count;

#define PHASE_COUNT     1024
#define BASE_FREQUENCY  80
#define CLOCK_FREQUENCY 72000000

void setup()
{
    // Set up the LED to blink 
    pinMode(TIM2_CH1, PWM);

    // Setup LED Timer
    Timer2.pause();
    Timer2.setChannel1Mode(TIMER_OUTPUTCOMPARE);
    Timer2.setPrescaleFactor(CLOCK_FREQUENCY / (PHASE_COUNT * BASE_FREQUENCY));
    Timer2.setOverflow(BASE_FREQUENCY - 1);
    Timer2.setCompare1(100);

    /*
     * Write OCxM = 011 to toggle OCx output pin when CNT matches CCRx Write 
     * OCxPE = 0 to disable preload register 
     * Write CCxP = 0 to select active high polarity 
     * Write CCxE = 1 to enable the output
     */
    
    // OCM1M (bits 6:4) set to 3 (enable toggle on channel 1)
    timer_port *timer2 = timer_dev_table[TIMER2].base;
    /* enable output*/
    timer2->CCMR1 &= ~3;
    /* clear OCM1M */
    timer2->CCMR1 &= ~(7 << 4);
    /* set the toggle */
    timer2->CCMR1 |= (3 << 4);
    /* enable PE */
    // timer2->CCMR1 |= 8;
    /* disable PE */
    timer2->CCMR1 &= ~8;
    /* enable the channel */
    timer2->CCER |= 1;
    /* attach the interrupt */
    Timer2.attachCompare1Interrupt(handler1);
    count = 0;
    last_count = 0;

    Timer2.resume();
}

void loop() 
{
    SerialUSB.print(count - last_count);
    SerialUSB.print(" ");
    SerialUSB.println(count);
    last_count = count;
    delay(5);
}

//uint16  ch1[16] = {100, 108, 208, 8, 300, 8, 400, 8, 500, 8, 600, 8, 700, 8, 800, 8};
uint16 ch1[16] = {100, 108, 308, 316, 616, 624, 0, 8, 508, 516, 92, 100, 800, 808, 584, 592};


void handler1(void) 
{
    count += 1;
    Timer2.setCompare1(ch1[count % 16]);
} 
