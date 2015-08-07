// os345interrupts.c - pollInterrupts	08/08/2013
// ***********************************************************************
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// **                                                                   **
// ** The code given here is the basis for the BYU CS345 projects.      **
// ** It comes "as is" and "unwarranted."  As such, when you use part   **
// ** or all of the code, it becomes "yours" and you are responsible to **
// ** understand any algorithm or method presented.  Likewise, any      **
// ** errors or problems become your responsibility to fix.             **
// **                                                                   **
// ** NOTES:                                                            **
// ** -Comments beginning with "// ??" may require some implementation. **
// ** -Tab stops are set at every 3 spaces.                             **
// ** -The function API's in "OS345.h" should not be altered.           **
// **                                                                   **
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// ***********************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <time.h>
#include <assert.h>

#include "os345.h"
#include "os345config.h"
#include "os345signals.h"

// **********************************************************************
//	local prototypes
//
void pollInterrupts(void);
static void keyboard_isr(void);
static void timer_isr(void);

// **********************************************************************
// **********************************************************************
// global semaphores

extern Semaphore* keyboard;				// keyboard semaphore
extern Semaphore* charReady;				// character has been entered
extern Semaphore* inBufferReady;			// input buffer ready semaphore

extern Semaphore* tics1sec;				// 1 second semaphore
extern Semaphore* tics10thsec;				// 1/10 second semaphore
extern Semaphore* tics10sec;            // 10 second semaphore

extern char inChar;				// last entered character
extern int charFlag;				// 0 => buffered input
extern int inBufIndx;				// input pointer into input buffer
extern int inBufLen;
extern int inBufLenPrev;
extern int inBufIndxPrev;
extern char inBuffer[INBUF_SIZE + 1];	// character input buffer
extern char inBufferPrev[INBUF_SIZE + 1];

extern time_t oldTime1;					// old 1sec time
extern time_t oldTime2;
extern clock_t myClkTime;
extern clock_t myOldClkTime;

extern int pollClock;				// current clock()
extern int lastPollClock;			// last pollClock

extern int superMode;						// system mode
extern TCB tcb[];


// **********************************************************************
// **********************************************************************
// simulate asynchronous interrupts by polling events during idle loop
//
void pollInterrupts(void)
{
	// check for task monopoly
	pollClock = clock();
	assert("Timeout" && ((pollClock - lastPollClock) < MAX_CYCLES));
	lastPollClock = pollClock;

	// check for keyboard interrupt
	if ((inChar = GET_CHAR) > 0)
	{
	  keyboard_isr();
	}

	// timer interrupt
	timer_isr();

	return;
} // end pollInterrupts


// **********************************************************************
// keyboard interrupt service routine
//
static void keyboard_isr()
{
	// assert system mode
	assert("keyboard_isr Error" && superMode);
	bool back = FALSE;

	semSignal(charReady);					// SIGNAL(charReady) (No Swap)
	if (charFlag == 0)
	{
		switch (inChar)
		{
		case '\r':
		case '\n':
		{
			inBufIndx = 0;				// EOL, signal line ready
			inBufLen = 0;
			semSignal(inBufferReady);	// SIGNAL(inBufferReady)
			break;
		}

		case 0x12:						// ^r
		{
			sigSignal(-1, mySIGCONT);
			for (int i = 0; i < MAX_TASKS; i++)
			{
				tcb[i].signal &= ~mySIGTSTP;
				tcb[i].signal &= ~mySIGSTOP;
			}
			break;
		}

		case 0x18:						// ^x
		{
			inBufIndx = 0;
			inBuffer[0] = 0;
			sigSignal(0, mySIGINT);		// interrupt task 0
			semSignal(inBufferReady);	// SEM_SIGNAL(inBufferReady)
			break;
		}

		case 0x17:						// ^w
		{
			sigSignal(-1, mySIGTSTP);
			break;
		}

		case 8:
		{
			if (inBufIndx > 0)
			{
				inBufIndx--;
				for (int i = inBufIndx; i < inBufLen; i++)
				{
					inBuffer[i] = inBuffer[i + 1];
				}
				inBufLen--;
				inBuffer[inBufLen] = 0;
				int amount = inBufLen - inBufIndx;
				for (int i = inBufIndx + 1; i > 0; i--)
				{
					printf("\b");
				}
				for (int i = 0; i < inBufLen + 1; i++)
				{
					printf("%c", inBuffer[i]);
				}
				for (int i = 0; i < amount+1; i++)
				{
					printf("\b");
				}

			}
			break;
		}

		case 0x48:
		{
			printf("%s", inBufferPrev);
			strcpy(inBuffer, inBufferPrev);
			inBufIndx = inBufLenPrev;
			inBufLen = inBufLenPrev;
			break;
		}

		case 0x4B:
		{
			printf("\b");
			inBufIndx--;
			break;
		}

		case 0x4D:
		{
			printf("%c", inBuffer[inBufIndx]);
			inBufIndx++;
			break;
		}

		default:
		{
			if (inBufIndx < INBUF_SIZE)
			{
				inBuffer[inBufIndx] = inChar;
				inBufferPrev[inBufIndx] = inChar;
				inBufIndx++;
				inBufLen++;
				inBuffer[inBufIndx] = 0;
				inBufferPrev[inBufIndx] = 0;
				inBufIndxPrev = inBufIndx;
				inBufLenPrev = inBufLen;
				printf("%c", inChar);	// echo character
			}
		}
		}
	}
	else
	{
		// single character mode
		inBufIndx = 0;
		inBuffer[inBufIndx] = 0;
	}
	return;
} // end keyboard_isr


// **********************************************************************
// timer interrupt service routine
//
static void timer_isr()
{
	time_t currentTime;						// current time

	// assert system mode
	assert("timer_isr Error" && superMode);

	// capture current time
  	time(&currentTime);

  	// one second timer
  	if ((currentTime - oldTime1) >= 1)
  	{
		// signal 1 second
  	   semSignal(tics1sec);
		oldTime1 += 1;
  	}

	if ((currentTime - oldTime2) >= 10)
	{
		// signal 10 seconds
		semSignal(tics10sec);
		oldTime2 += 10;
	}

	// sample fine clock
	myClkTime = clock();
	if ((myClkTime - myOldClkTime) >= ONE_TENTH_SEC)
	{
		myOldClkTime = myOldClkTime + ONE_TENTH_SEC;   // update old
		semSignal(tics10thsec);
		poll_removeDC();
	}

	//tics10secs
	/*
	Add a 10 second timer (tics10sec) counting semaphore to the timer_isr() function (os345interrupts.c). 
	Include the <time.h> header and call the C function time(time_t *timer). 
	SEM_SIGNAL the tics10sec semaphore every 10 seconds. 
	Create a reentrant, high priority task that blocks (SEM_WAIT) on the 10 second timer semaphore (tics10sec). 
	When activated, output a message with the current task number and time and then block again.
	*/

	return;
} // end timer_isr
