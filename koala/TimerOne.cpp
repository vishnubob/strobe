/*
 *  Interrupt and PWM utilities for 16 bit Timer1 on ATmega168/328
 *  Original code by Jesse Tane for http://labs.ideo.com August 2008
 *  Modified March 2009 by Jérôme Despatis and Jesse Tane for ATmega328 support
 *  Modified June 2009 by Michael Polli and Jesse Tane to fix a bug in setPeriod() which caused the timer to stop
 *  Modified June 2011 by Lex Talionis to add a function to read the timer
 *  Modified Oct 2011 by Andrew Richards to avoid certain problems:
 *  - Add (long) assignments and casts to TimerOne::read() to ensure calculations involving tmp, ICR1 and TCNT1 aren't truncated
 *  - Ensure 16 bit registers accesses are atomic - run with interrupts disabled when accessing
 *  - Remove global enable of interrupts (sei())- could be running within an interrupt routine)
 *  - Disable interrupts whilst TCTN1 == 0.  Datasheet vague on this, but experiment shows that overflow interrupt 
 *    flag gets set whilst TCNT1 == 0, resulting in a phantom interrupt.  Could just set to 1, but gets inaccurate
 *    at very short durations
 *  - startBottom() added to start counter at 0 and handle all interrupt enabling.
 *  - start() amended to enable interrupts
 *  - restart() amended to point at startBottom()
 * Modiied 7:26 PM Sunday, October 09, 2011 by Lex Talionis
 *  - renamed start() to resume() to reflect it's actual role
 *  - renamed startBottom() to start(). This breaks some old code that expects start to continue counting where it left off
 *
 *  This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  See Google Code project http://code.google.com/p/arduino-timerone/ for latest
 */
#ifndef TIMERONE_cpp
#define TIMERONE_cpp

#include "TimerOne.h"
#include "Arduino.h"

TimerOne Timer1;              // preinstatiate
volatile uint16_t _ocr1a_on;
volatile uint16_t _ocr1a_off;
volatile uint16_t _ocr1a_on_paged;
volatile uint16_t _ocr1a_off_paged;

ISR(TIMER1_COMPB_vect)
{
    TCCR1A ^= (1 << COM0B0);
    if (TCCR1A & (1 << COM0B0))
    {
        // set OCOB on compare match
        OCR1B = _ocr1a_on;
    } else
    {
        // clear OCOB on compare match
        OCR1B = _ocr1a_off;
        _ocr1a_off = _ocr1a_off_paged;
        _ocr1a_on = _ocr1a_on_paged;
    }
}

ISR(TIMER1_COMPA_vect)
{
}

ISR(TIMER1_CAPT_vect)
{
}

ISR(TIMER1_OVF_vect)
{
    TCNT0 = 0;
}


void TimerOne::initialize(long microseconds, uint8_t mode)
{
    _on = false;
    TCCR1A = 0;
    TCCR1B = 0;

    int8_t sawtooth_modes[] = {0, 4, 5, 6, 7, 12, 13, 14, 15, -1};
    int8_t idx = 0;
    _mode = mode;
    _sawtooth_mode = false;
    //bitSet(TIMSK1, TOIE1);
    //bitSet(TIMSK1, ICIE1);
    //bitSet(TIMSK1, OCIE1A);

    while (!_sawtooth_mode && (sawtooth_modes[idx] != -1))
        _sawtooth_mode = (_mode == sawtooth_modes[idx++]);
    if (_mode & _BV(0))
        bitSet(TCCR1A, WGM10);

    if (_mode & _BV(1))
        bitSet(TCCR1A, WGM11);

    if (_mode & _BV(2))
        bitSet(TCCR1B, WGM12);

    if (_mode & _BV(3))
        bitSet(TCCR1B, WGM13);

    setPeriod(microseconds);
}


void TimerOne::setPeriod(long microseconds)		// AR modified for atomic access
{
    long cycles = (F_CPU / 1E6) * microseconds;
    if (!_sawtooth_mode)
    {
        cycles /= 2;
    }

    if(cycles < RESOLUTION)              clockSelectBits = _BV(CS10);              // no prescale, full xtal
    else if((cycles >>= 3) < RESOLUTION) clockSelectBits = _BV(CS11);              // prescale by /8
    else if((cycles >>= 3) < RESOLUTION) clockSelectBits = _BV(CS11) | _BV(CS10);  // prescale by /64
    else if((cycles >>= 2) < RESOLUTION) clockSelectBits = _BV(CS12);              // prescale by /256
    else if((cycles >>= 2) < RESOLUTION) clockSelectBits = _BV(CS12) | _BV(CS10);  // prescale by /1024
    else        cycles = RESOLUTION - 1, clockSelectBits = _BV(CS12) | _BV(CS10);  // request was out of bounds, set as maximum

    oldSREG = SREG;				
    cli();							// Disable interrupts for 16 bit register access
    //ICR1 = pwmPeriod = cycles;                                          // ICR1 is TOP in p & f correct pwm mode
    pwmPeriod = cycles;                                          // ICR1 is TOP in p & f correct pwm mode
    OCR1A = pwmPeriod - 1;                                          // ICR1 is TOP in p & f correct pwm mode
    SREG = oldSREG;

    TCCR1B &= ~(_BV(CS10) | _BV(CS11) | _BV(CS12));
    TCCR1B |= clockSelectBits;                                          // reset clock select register, and starts the clock
}

void TimerOne::set_compare(uint16_t on, uint16_t off)
{
    _ocr1a_on_paged = on;
    _ocr1a_off_paged = off;
}

void TimerOne::setPwmDuty(char pin, int duty)
{
  unsigned long dutyCycle = pwmPeriod;
  
  dutyCycle *= duty;
  dutyCycle >>= 10;
  
  oldSREG = SREG;
  cli();
  if(pin == 1 || pin == 9)       OCR1A = dutyCycle;
  else if(pin == 2 || pin == 10) OCR1B = dutyCycle;
  SREG = oldSREG;
}

void TimerOne::configure_pin(uint8_t pin, uint8_t mode)
{
    uint8_t pbit, cbit;

    if (pin == 1) { pin = 9; }
    if (pin == 2) { pin = 10; }
    if (pin == 9) { pbit = PORTB1; cbit = COM1A0; }
    else if (pin == 10) { pbit = PORTB2; cbit = COM1B0; }
    else return;

    uint8_t mask = 0xFF ^ (0x3 << cbit);
    TCCR1A = (TCCR1A & mask) | ((mode & 0x3) << cbit);
    if (mode) DDRB |= _BV(pbit);
}

void TimerOne::pwm(char pin, int duty, long microseconds)  // expects duty cycle to be 10 bit (1024)
{
  if(microseconds > 0) setPeriod(microseconds);
  if(pin == 1 || pin == 9) {
    DDRB |= _BV(PORTB1);                                   // sets data direction register for pwm output pin
    TCCR1A |= _BV(COM1A1);                                 // activates the output pin
  }
  else if(pin == 2 || pin == 10) {
    DDRB |= _BV(PORTB2);
    TCCR1A |= _BV(COM1B1);
  }
  setPwmDuty(pin, duty);
  resume();			// Lex - make sure the clock is running.  We don't want to restart the count, in case we are starting the second WGM
					// and the first one is in the middle of a cycle
}

void TimerOne::disablePwm(char pin)
{
    configure_pin(pin, 0);
}

void TimerOne::attachInterrupt(void (*isr)(), long microseconds)
{
  if(microseconds > 0) setPeriod(microseconds);
  isrCallback = isr;                                       // register the user's callback with the real ISR
  TIMSK1 = _BV(OCIE1A);                                     // sets the timer overflow interrupt enable bit
	// might be running with interrupts disabled (eg inside an ISR), so don't touch the global state
//  sei();
  resume();												
}

void TimerOne::detachInterrupt()
{
  TIMSK1 &= ~_BV(TOIE1);                                   // clears the timer overflow interrupt enable bit 
															// timer continues to count without calling the isr
}

void TimerOne::resume()				// AR suggested
{ 
    TCCR1B |= clockSelectBits;
    GTCCR = 0;
}

void TimerOne::restart()		// Depricated - Public interface to start at zero - Lex 10/9/2011
{
	start();				
}

void TimerOne::start()	// AR addition, renamed by Lex to reflect it's actual role
{
    uint8_t _TCCR0A = TCCR0A;
    uint8_t _TCCR0B = TCCR0B;
    uint8_t _TCCR1A = TCCR1A;
    uint8_t _TCCR1B = TCCR1B;
    uint8_t _OCR0B = OCR0B;
    uint8_t _OCR1B = OCR1B;

    bitClear(TIMSK1, OCIE1B);
    GTCCR = (1 << TSM) | (1 << PSRSYNC);
    TCCR0A = 0x30; // set timer0 to normal mode
    TCCR0B = 0x01; // CK = CPU/1
    TCCR1A = 0x30; // same for timer1
    TCCR1B = 0x01;
    TCNT0 = 0;
    TCNT1 = 0; // set counters to 0
    OCR0B = 3; // as compare fails 1 ck cycle after TCNT change
    OCR1B = 3; // set compare match to low value larger than 1
    GTCCR = 0; // restart timers
    asm volatile ("nop"); // delay for the length of your count
    asm volatile ("nop");
    asm volatile ("nop");
    GTCCR = (1 << TSM) | (1 << PSRSYNC);

    TCCR0A = _TCCR0A;
    TCCR0B = _TCCR0B;
    TCCR1A = _TCCR1A;
    TCCR1B = _TCCR1B;
    OCR0B = _OCR0B;
    OCR1B = _OCR1B;

    TCNT1 = 1;                	
    TCNT0 = 1;
    bitSet(TIMSK1, OCIE1B);

    GTCCR = 0;
}

void TimerOne::stop()
{
  TCCR1B &= ~(_BV(CS10) | _BV(CS11) | _BV(CS12));          // clears all clock selects bits
}

unsigned long TimerOne::read()		//returns the value of the timer in microseconds
{									//rember! phase and freq correct mode counts up to then down again
  	unsigned long tmp;				// AR amended to hold more than 65536 (could be nearly double this)
  	unsigned int tcnt1;				// AR added

	oldSREG= SREG;
  	cli();							
  	tmp=TCNT1;    					
	SREG = oldSREG;

	char scale=0;
	switch (clockSelectBits)
	{
	case 1:// no prescalse
		scale=0;
		break;
	case 2:// x8 prescale
		scale=3;
		break;
	case 3:// x64
		scale=6;
		break;
	case 4:// x256
		scale=8;
		break;
	case 5:// x1024
		scale=10;
		break;
	}
	
	do {	// Nothing -- max delay here is ~1023 cycles.  AR modified
		oldSREG = SREG;
		cli();
		tcnt1 = TCNT1;
		SREG = oldSREG;
	} while (tcnt1==tmp); //if the timer has not ticked yet

	//if we are counting down add the top value to how far we have counted down
	tmp = (  (tcnt1>tmp) ? (tmp) : (long)(ICR1-tcnt1)+(long)ICR1  );		// AR amended to add casts and reuse previous TCNT1
	return ((tmp*1000L)/(F_CPU /1000L))<<scale;
}

#endif
