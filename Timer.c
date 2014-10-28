/*
 * Timer.c
 * (c) 2014 flabbergast
 *  implements Arduino-like millis() function via RTC timer interrupt (on XMEGAs)
 *  Note: the XMEGA version counts in 10/1024 secs, so not exactly tens of milliseconds (2.4% error :)
 *  Note: the AVR8 version counts in 10.24 millisecs (wrong the other way than XMEGA :)
 *
 * Credits:
 *  - XMEGA code from: http://www.jtronics.de/avr-projekte/xmega-tutorial/xmega-tutorial-real-time-counter.html
 */

#include <avr/interrupt.h>

#include "Timer.h"

volatile uint32_t current_time;

#if (defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega32U2__)) // use TIMER0 compare interrupt to keep track of time
volatile uint8_t helper_counter;
void Timer_Init(void) {
  TCCR0B |= (1 << CS00)|(1 << CS01); // prescaler F_CPU/64
  TCNT0 = 0; // initalize the counter
  helper_counter = 0;
  TIMSK0 |= (1 << TOIE0); // enable TIMER0 overflow interrupt (fires every 256 prescaled cycles)
  // altogether the int fires every 1.024 ms on F_CPU=16MHz and 2.048 ms on F_CPU=8MHz
}

  #if (F_CPU == 16000000)
    #define TIMER_HELPER_CONSTANT 10
  #elif (F_CPU == 8000000)
    #define TIMER_HELPER_CONSTANT 5
  #else
    #error "Unusual F_CPU: you should define some constants in Timer.c."
  #endif
// TIMER0 overflow interrupt handler
ISR(TIMER0_OVF_vect) {
  helper_counter++;
  if(helper_counter>=TIMER_HELPER_CONSTANT) {
    current_time++;
    helper_counter = 0;
  }
}

#elif (defined(__AVR_ATxmega128A3U__)) // use internal RTC oscillator to generate interrupts
void Timer_Init(void) {
  current_time = 0;
  //############################### Clock für RTC aktivieren
  // Unlock access to protected IO register for 4 cycles
  CCP   = CCP_IOREG_gc;
  // Internen RTC 32768KHz Oszillator aktivieren
  OSC.CTRL  |= OSC_RC32KEN_bm;
  // Internen Oszillator mit 1024khz für RTC verwenden
  CLK.RTCCTRL = CLK_RTCSRC_RCOSC_gc|CLK_RTCEN_bm;
  // Do not prescale (fire 1024 times / second)
  RTC.CTRL  = RTC_PRESCALER_DIV1_gc;
  // Warten bis Takt und RTC synchronisiert ist
  while(RTC.STATUS & RTC_SYNCBUSY_bm);

  //############################### RTC --> 1Hz
  //Timertopwert einstellen
  RTC.PER   = 10; // fire an interrupt every 10 / 1024 sec
  //Timeroverflow Interrupt mit Interrupt Priorität Hoch einstellen
  RTC.INTCTRL |= RTC_OVFINTLVL_HI_gc;
  //Timerregister CNT auf 0 stellen
  RTC.CNT   = 0;
  //RTC.COMP  = 2; // note: if COMP>PER, no 'compare' interrupt will ever be generated
}

//################################################## ISR RTC 1Hz
ISR(RTC_OVF_vect) {
  current_time++;
  // testing: toggle LED on E0 approx every 1 sec
  //if(0 == (current_time % 102))
  //  PORTE.OUTTGL = 1;
}

#else
  #error "You should define some timer in Timer.c for your ATMEL chip."
#endif

uint32_t millis10(void) {
  return current_time;
}

