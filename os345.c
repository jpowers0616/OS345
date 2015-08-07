// os345.c - OS Kernel	09/12/2013
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
#include "os345signals.h"
#include "os345config.h"
#include "os345lc3.h"
#include "os345fat.h"

#define my_printf	printf

// **********************************************************************
//	local prototypes
//
void pollInterrupts(void);
static int scheduler(void);
static int dispatcher(void);

//static void keyboard_isr(void);
//static void timer_isr(void);

int sysKillTask(int taskId);
static int initOS(void);

// **********************************************************************
// **********************************************************************
// global semaphores

Semaphore* semaphoreList;			// linked list of active semaphores

Semaphore* keyboard;				// keyboard semaphore
Semaphore* charReady;				// character has been entered
Semaphore* inBufferReady;			// input buffer ready semaphore

Semaphore* tics1sec;				// 1 second semaphore
Semaphore* tics10thsec;				// 1/10 second semaphore
Semaphore* tics10sec;               // 10 second semaphore

// **********************************************************************
// **********************************************************************
// global system variables

TCB tcb[MAX_TASKS];					// task control block
Semaphore* taskSems[MAX_TASKS];		// task semaphore
jmp_buf k_context;					// context of kernel stack
jmp_buf reset_context;				// context of kernel stack
volatile void* temp;				// temp pointer used in dispatcher

int scheduler_mode;					// scheduler mode
int superMode;						// system mode
int curTask;						// current task #
long swapCount;						// number of re-schedule cycles
char inChar;						// last entered character
int charFlag;						// 0 => buffered input
int inBufIndx;						// input pointer into input buffer
int inBufLen;
int inBufLenPrev;
int inBufIndxPrev;
char inBuffer[INBUF_SIZE+1];		// character input buffer
char inBufferPrev[INBUF_SIZE + 1];
//Message messages[NUM_MESSAGES];		// process message buffers

int pollClock;						// current clock()
int lastPollClock;					// last pollClock
bool diskMounted;					// disk has been mounted

time_t oldTime1;					// old 1sec time
time_t oldTime2;
clock_t myClkTime;
clock_t myOldClkTime;

PQ readyQ;


// **********************************************************************
// **********************************************************************
// OS startup
//
// 1. Init OS
// 2. Define reset longjmp vector
// 3. Define global system semaphores
// 4. Create CLI task
// 5. Enter scheduling/idle loop
//
int main(int argc, char* argv[])
{
	// save context for restart (a system reset would return here...)
	int resetCode = setjmp(reset_context);
	superMode = TRUE;						// supervisor mode

	switch (resetCode)
	{
		case POWER_DOWN_QUIT:				// quit
			powerDown(0);
			printf("\nGoodbye!!");
			return 0;

		case POWER_DOWN_RESTART:			// restart
			powerDown(resetCode);
			printf("\nRestarting system...\n");

		case POWER_UP:						// startup
			break;

		default:
			printf("\nShutting down due to error %d", resetCode);
			powerDown(resetCode);
			return resetCode;
	}

	// output header message
	printf("%s", STARTUP_MSG);

	// initalize OS
	if ( resetCode = initOS()) return resetCode;

	// create global/system semaphores here
	//?? vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

	charReady = createSemaphore("charReady", BINARY, 0);
	inBufferReady = createSemaphore("inBufferReady", BINARY, 0);
	keyboard = createSemaphore("keyboard", BINARY, 1);
	tics1sec = createSemaphore("tics1sec", BINARY, 0);
	tics10thsec = createSemaphore("tics10thsec", BINARY, 0);
	tics10sec = createSemaphore("tics10sec", COUNTING, 0);
	//?? ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	// schedule CLI task
	createTask("myShell",			// task name
					P1_shellTask,	// task
					MED_PRIORITY,	// task priority
					argc,			// task arg count
					argv);			// task argument pointers

	// HERE WE GO................

	// Scheduling loop
	// 1. Check for asynchronous events (character inputs, timers, etc.)
	// 2. Choose a ready task to schedule
	// 3. Dispatch task
	// 4. Loop (forever!)

	while(1)									// scheduling loop
	{
		// check for character / timer interrupts
		pollInterrupts();

		// schedule highest priority ready task
		if ((curTask = scheduler()) < 0) continue;

		// dispatch curTask, quit OS if negative return
		if (dispatcher() < 0) break;
	}											// end of scheduling loop

	// exit os
	longjmp(reset_context, POWER_DOWN_QUIT);
	return 0;
} // end main



// **********************************************************************
// **********************************************************************
// scheduler
//
static int scheduler()
{
	int nextTask;
	obj task;
	// ?? Design and implement a scheduler that will select the next highest
	// ?? priority ready task to pass to the system dispatcher.

	// ?? WARNING: You must NEVER call swapTask() from within this function
	// ?? or any function that it calls.  This is because swapping is
	// ?? handled entirely in the swapTask function, which, in turn, may
	// ?? call this function.  (ie. You would create an infinite loop.)

	// ?? Implement a round-robin, preemptive, prioritized scheduler.

	// ?? This code is simply a round-robin scheduler and is just to get
	// ?? you thinking about scheduling.  You must implement code to handle
	// ?? priorities, clean up dead tasks, and handle semaphores appropriately.

	// schedule next task
	if (scheduler_mode == 0)
	{
		task = deQ(&readyQ, -1);
		nextTask = task.taskId;
		if (nextTask >= 0)
		{
			enQ(&readyQ, task);
		}

		// mask sure nextTask is valid
		while (!tcb[nextTask].name)
		{
			if (++nextTask >= MAX_TASKS) nextTask = 0;
		}
		if (tcb[nextTask].signal & mySIGSTOP) return -1;
		return nextTask;
	}
	else
	{
		task = deQ(&readyQ, -1);
		nextTask = task.taskId;
		if (nextTask >= 0)
		{
			enQ(&readyQ, task);
		}

		if (tcb[task.taskId].taskTime > 0)
		{
			tcb[task.taskId].taskTime--;
			return nextTask;
		}
		else
		{
			if(Zero(&readyQ))
			{ 
				setSchedule(&readyQ);
				
				task = deQ(&readyQ, -1);
				nextTask = task.taskId;
				if (nextTask >= 0)
				{
					enQ(&readyQ, task);
				}
				tcb[task.taskId].taskTime--;
				return nextTask;
			}
			else
			{
				while (tcb[task.taskId].taskTime < 1)
				{
					task = deQ(&readyQ, -1);
					nextTask = task.taskId;
					if (nextTask >= 0)
					{
						enQ(&readyQ, task);
					}
				}

				while (!tcb[nextTask].name)
				{
					if (++nextTask >= MAX_TASKS) nextTask = 0;
				}
				if (tcb[nextTask].signal & mySIGSTOP) return -1;
				tcb[task.taskId].taskTime--;
				return nextTask;
			}
		}
	}
} // end scheduler



// **********************************************************************
// **********************************************************************
// dispatch curTask
//
static int dispatcher()
{
	int result;

	if (tcb[curTask].signal == mySIGSTOP)
	{
		return 0;
	}
	// schedule task
	switch(tcb[curTask].state)
	{
		case S_NEW:
		{
			// new task
			printf("\nNew Task[%d] %s", curTask, tcb[curTask].name);
			tcb[curTask].state = S_RUNNING;	// set task to run state

			// save kernel context for task SWAP's
			if (setjmp(k_context))
			{
				superMode = TRUE;					// supervisor mode
				break;								// context switch to next task
			}

			// move to new task stack (leave room for return value/address)
			temp = (int*)tcb[curTask].stack + (STACK_SIZE-8);
			SET_STACK(temp);
			superMode = FALSE;						// user mode

			// begin execution of new task, pass argc, argv
			result = (*tcb[curTask].task)(tcb[curTask].argc, tcb[curTask].argv);

			// task has completed
			if (result) printf("\nTask[%d] returned %d", curTask, result);
			else printf("\nTask[%d] returned %d", curTask, result);
			tcb[curTask].state = S_EXIT;			// set task to exit state

			// return to kernal mode
			longjmp(k_context, 1);					// return to kernel
		}

		case S_READY:
		{
			tcb[curTask].state = S_RUNNING;			// set task to run
		}

		case S_RUNNING:
		{
			if (setjmp(k_context))
			{
				// SWAP executed in task
				superMode = TRUE;					// supervisor mode
				break;								// return from task
			}
			if (signals()) break;
			longjmp(tcb[curTask].context, 3); 		// restore task context
		}

		case S_BLOCKED:
		{
			// ?? Could check here to unblock task
			break;
		}

		case S_EXIT:
		{
			if (curTask == 0) return -1;			// if CLI, then quit scheduler
			// release resources and kill task
			sysKillTask(curTask);					// kill current task
			break;
		}

		default:
		{
			printf("Unknown Task[%d] State", curTask);
			longjmp(reset_context, POWER_DOWN_ERROR);
		}
	}
	return 0;
} // end dispatcher



// **********************************************************************
// **********************************************************************
// Do a context switch to next task.

// 1. If scheduling task, return (setjmp returns non-zero value)
// 2. Else, save current task context (setjmp returns zero value)
// 3. Set current task state to READY
// 4. Enter kernel mode (longjmp to k_context)

void swapTask()
{
	assert("SWAP Error" && !superMode);		// assert user mode

	// increment swap cycle counter
	swapCount++;

	// either save current task context or schedule task (return)
	if (setjmp(tcb[curTask].context))
	{
		superMode = FALSE;					// user mode
		return;
	}

	// context switch - move task state to ready
	if (tcb[curTask].state == S_RUNNING) tcb[curTask].state = S_READY;

	// move to kernel mode (reschedule)
	longjmp(k_context, 2);
} // end swapTask



// **********************************************************************
// **********************************************************************
// system utility functions
// **********************************************************************
// **********************************************************************

// **********************************************************************
// **********************************************************************
// initialize operating system
static int initOS()
{
	int i;

	// make any system adjustments (for unblocking keyboard inputs)
	INIT_OS

	// reset system variables
	curTask = 0;						// current task #
	swapCount = 0;						// number of scheduler cycles
	scheduler_mode = 0;					// default scheduler
	inChar = 0;							// last entered character
	charFlag = 0;						// 0 => buffered input
	inBufIndx = 0;						// input pointer into input buffer
	semaphoreList = 0;					// linked list of active semaphores
	diskMounted = 0;					// disk has been mounted

	// malloc ready queue and blocked queue
	init_PQ(&readyQ);
	//rq = (int*)malloc(MAX_TASKS * sizeof(int));
	if (&readyQ == NULL) return 99;

	// capture current time
	lastPollClock = clock();			// last pollClock
	time(&oldTime1);
	time(&oldTime2);

	// init system tcb's
	for (i=0; i<MAX_TASKS; i++)
	{
		tcb[i].name = NULL;				// tcb
		taskSems[i] = NULL;				// task semaphore
	}

	// init tcb
	for (i=0; i<MAX_TASKS; i++)
	{
		tcb[i].name = NULL;
	}

	// initialize lc-3 memory
	initLC3Memory(LC3_MEM_FRAME, 0xF800>>6);

	// ?? initialize all execution queues

	return 0;
} // end initOS



// **********************************************************************
// **********************************************************************
// Causes the system to shut down. Use this for critical errors
void powerDown(int code)
{
	int i;
	printf("\nPowerDown Code %d", code);

	// release all system resources.
	printf("\nRecovering Task Resources...");

	// kill all tasks
	for (i = MAX_TASKS-1; i >= 0; i--)
		if(tcb[i].name) sysKillTask(i);

	// delete all semaphores
	while (semaphoreList)
		deleteSemaphore(&semaphoreList);

	// free ready queue
	freeQ(&readyQ);

	// ?? release any other system resources
	// ?? deltaclock (project 3)

	RESTORE_OS
	return;
} // end powerDown

void init_obj(obj* item, int priority, int taskId)
{
	item->priority = priority;
	item->taskId = taskId;
}

void init_PQ(PQ* pq)
{
	pq->items = (obj*)malloc(sizeof(obj) * MAX_TASKS);
	pq->count = 0;
}

void enQ(PQ* pq, obj item)
{
	pq->items[pq->count] = item;
	for (int i = pq->count; (i > 0) && ((pq->items[i].priority) > (pq->items[i - 1].priority)); i--)
	{
		obj save = pq->items[i - 1];
		pq->items[i - 1] = pq->items[i];
		pq->items[i] = save;
	}
	pq->count++;
}

bool Zero(PQ* pq)
{
	bool allZero = TRUE;
	for (int i = 0; i < pq->count; i++)
	{
		if (tcb[pq->items[i].taskId].taskTime != 0)
		{
			allZero = FALSE;
		}
	}
	return allZero;
}

void setSchedule(PQ* pq)
{
	int j = 0;
	int parent_num = 0;
	int parentId[] = { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };
	int children[MAX_TASKS];
	for (int i = 0; i < MAX_TASKS; i++)
	{
		children[i] = 0;
	}
	
	for (int i = 0; i < pq->count; i++)
	{
		//printf("task id: %d name: %s \n", pq->items[i].taskId, tcb[pq->items[i].taskId].name);
		if (tcb[pq->items[i].taskId].parent == 0)
		{
			parentId[j] = pq->items[i].taskId;
			children[pq->items[i].taskId]++;
			j++;
		}
		else
		{
			children[tcb[pq->items[i].taskId].parent]++;
		}
	}

	int greatest = children[parentId[0]];
	for (int i = 0; i < j; i++)
	{
		if (children[parentId[i]] > greatest)
		{
			greatest = children[parentId[i]];
		}
	}
	int units_per_parent[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	int units_per_child[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	for (int i = 0; i < j; i++)
	{
		int base = greatest / (children[parentId[i]]);
		units_per_parent[i] = base + (greatest % (children[parentId[i]]));
		units_per_child[i] = base;
	}

	for (int i = 0; i < pq->count; i++)
	{
		if (tcb[pq->items[i].taskId].parent == 0)
		{
			for (int k = 0; k < j; k++)
			{
				if (pq->items[i].taskId == parentId[k])
				{
					tcb[pq->items[i].taskId].taskTime = units_per_parent[k];
				}
			}
		}
		else
		{
			for (int k = 0; k < j; k++)
			{
				if (tcb[pq->items[i].taskId].parent == parentId[k])
				{
					tcb[pq->items[i].taskId].taskTime = units_per_child[k];
				}
			}
		}
	}
	tcb[0].taskTime = greatest;
}

obj deQ(PQ* pq, int type)
{
	obj toReturn;
	if (type == -1)
	{
		if (pq->count == 0)
		{
			init_obj(&toReturn, -1, -1);
			return toReturn;
		}
		else
		{
			toReturn = pq->items[0];
			for (int i = 0; i < pq->count; i++)
			{
				pq->items[i] = pq->items[i + 1];
			}
			pq->count--;
			return toReturn;
		}
	}
	else
	{
		int index = -1;
		for (int i = 0; i < pq->count; i++)
		{
			if (pq->items[i].taskId == type)
			{
				index = i;
			}
		}
		
		if (index == -1)
		{
			init_obj(&toReturn, -1, -1);
			return toReturn;
		}
		else
		{
			toReturn = pq->items[index];
			for (int i = index; i < pq->count; i++)
			{
				pq->items[i] = pq->items[i + 1];
			}
			pq->count--;
			return toReturn;
		}

	}
}

void freeQ(PQ* pq)
{
	free(pq->items);
}

void printQ(PQ* pq)
{
	for (int i = 0; i < pq->count; i++)
	{
		int tid = pq->items[i].taskId;
		if (tcb[tid].name)
		{
			printf("\n%4d/%-4d%20s%4d  ", tid, tcb[tid].parent,
				tcb[tid].name, tcb[tid].priority);
			if (tcb[tid].signal & mySIGSTOP) my_printf("Paused");
			else if (tcb[tid].state == S_NEW) my_printf("New");
			else if (tcb[tid].state == S_READY) my_printf("Ready");
			else if (tcb[tid].state == S_RUNNING) my_printf("Running");
			else if (tcb[tid].state == S_BLOCKED) my_printf("Blocked    %s",
				tcb[tid].name);
			else if (tcb[tid].state == S_EXIT) my_printf("Exiting");
			swapTask();
		}
	}
}

