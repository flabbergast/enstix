/*
 * Timer.h
 * (c) 2014 flabbergast
 *  implements Arduino-like millis() function via RTC timer interrupt (on XMEGAs)
 */

#ifndef _PROJECT_TIMER_H_
#define _PROJECT_TIMER_H_

void Timer_Init(void);
uint32_t millis10(void);

#endif
