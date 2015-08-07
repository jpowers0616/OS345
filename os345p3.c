// os345p3.c - Jurassic Park
// ***********************************************************************
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// **                                                                   **
// ** The code given here is the basis for the CS345 projects.          **
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
#include "os345park.h"

// ***********************************************************************
// project 3 variables

// Jurassic Park
extern JPARK myPark;
extern Semaphore* parkMutex;						// protect park access
extern Semaphore* fillSeat[NUM_CARS];			// (signal) seat ready to fill
extern Semaphore* seatFilled[NUM_CARS];		// (wait) passenger seated
extern Semaphore* rideOver[NUM_CARS];			// (signal) ride over

extern Semaphore* tickets;
extern Semaphore* cars;
extern Semaphore* drivers;
extern Semaphore* seats;
extern Semaphore* visitors;
extern Semaphore* inPark;
extern Semaphore* waitPark;
extern Semaphore* inMuseum;
extern Semaphore* inGiftshop;

// ***********************************************************************
// project 3 functions and tasks
void CL3_project3(int, char**);
void CL3_dc(int, char**);

// ***********************************************************************
// ***********************************************************************
// project3 command
int P3_project3(int argc, char* argv[])
{
	char buf[32];
	char* newArgv[2];

	// start park
	sprintf(buf, "jurassicPark");
	newArgv[0] = buf;
	createTask( buf,				// task name
		jurassicTask,				// task
		MED_PRIORITY,				// task priority
		1,								// task count
		newArgv);					// task argument

	// wait for park to get initialized...
	while (!parkMutex) SWAP;
	printf("\nStart Jurassic Park...");

	//?? create car, driver, and visitor tasks here
	//myPark.numInCarLine = myPark.numInPark = 9;

	for (int i = 0; i < NUM_CARS; i++)
	{
		char str[15]; SWAP;
		sprintf(str, "%d", i); SWAP;
		newArgv[0] = str; SWAP;
		createTask("Car", carTask, MED_PRIORITY, 1, newArgv); SWAP;
	}

	for (int i = 0; i < NUM_VISITORS; i++)
	{
		char str[15]; SWAP;
		sprintf(str, "%d", i); SWAP;
		newArgv[0] = str; SWAP;
		createTask("Visitor", visitorTask, MED_PRIORITY, 1, newArgv); SWAP;
	}

	for (int i = 0; i < NUM_DRIVERS; i++)
	{
		char str[15]; SWAP;
		sprintf(str, "%d", i); SWAP;
		newArgv[0] = str; SWAP;
		createTask("Driver", driverTask, MED_PRIORITY, 1, newArgv); SWAP;
	}
	createTask("dinoTask", dinoTask, MED_PRIORITY, 1, newArgv); SWAP;

	return 0;
} // end project3



// ***********************************************************************
// ***********************************************************************
// delta clock command
int P3_dc(int argc, char* argv[])
{
	printf("\nDelta Clock");
	//print delta clock contents
	printf("\nTo Be Implemented!");
	return 0;
} // end CL3_dc


/*
// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
// delta clock command
int P3_dc(int argc, char* argv[])
{
	printf("\nDelta Clock");
	// ?? Implement a routine to display the current delta clock contents
	//printf("\nTo Be Implemented!");
	int i;
	for (i=0; i<numDeltaClock; i++)
	{
		printf("\n%4d%4d  %-20s", i, deltaClock[i].time, deltaClock[i].sem->name);
	}
	return 0;
} // end CL3_dc


// ***********************************************************************
// display all pending events in the delta clock list
void printDeltaClock(void)
{
	int i;
	for (i=0; i<numDeltaClock; i++)
	{
		printf("\n%4d%4d  %-20s", i, deltaClock[i].time, deltaClock[i].sem->name);
	}
	return;
}


// ***********************************************************************
// test delta clock
int P3_tdc(int argc, char* argv[])
{
	createTask( "DC Test",			// task name
		dcMonitorTask,		// task
		10,					// task priority
		argc,					// task arguments
		argv);

	timeTaskID = createTask( "Time",		// task name
		timeTask,	// task
		10,			// task priority
		argc,			// task arguments
		argv);
	return 0;
} // end P3_tdc



// ***********************************************************************
// monitor the delta clock task
int dcMonitorTask(int argc, char* argv[])
{
	int i, flg;
	char buf[32];
	// create some test times for event[0-9]
	int ttime[10] = {
		90, 300, 50, 170, 340, 300, 50, 300, 40, 110	};

	for (i=0; i<10; i++)
	{
		sprintf(buf, "event[%d]", i);
		event[i] = createSemaphore(buf, BINARY, 0);
		insertDeltaClock(ttime[i], event[i]);
	}
	printDeltaClock();

	while (numDeltaClock > 0)
	{
		SEM_WAIT(dcChange)
		flg = 0;
		for (i=0; i<10; i++)
		{
			if (event[i]->state ==1)			{
					printf("\n  event[%d] signaled", i);
					event[i]->state = 0;
					flg = 1;
				}
		}
		if (flg) printDeltaClock();
	}
	printf("\nNo more events in Delta Clock");

	// kill dcMonitorTask
	tcb[timeTaskID].state = S_EXIT;
	return 0;
} // end dcMonitorTask


extern Semaphore* tics1sec;

// ********************************************************************************************
// display time every tics1sec
int timeTask(int argc, char* argv[])
{
	char svtime[64];						// ascii current time
	while (1)
	{
		SEM_WAIT(tics1sec)
		printf("\nTime = %s", myTime(svtime));
	}
	return 0;
} // end timeTask
*/

/*struct DeltaC
{
	int time;
	Semaphore* sem;
	struct DeltaC *next;
};

struct DeltaC *head = NULL;
struct DeltaC *back = NULL;

struct DeltaC* create_list(int time, Semaphore* sem)
{
	struct DeltaC *ptr = (struct DeltaC*)malloc(sizeof(struct DeltaC)); SWAP;
	if (ptr == NULL)
	{
		printf("\n Node creation failed \n"); SWAP;
		return NULL;
	}
	ptr->time = time; SWAP;
	ptr->sem = sem; SWAP;
	ptr->next = NULL; SWAP;

	head = back = ptr; SWAP;
	return ptr;
}

struct DeltaC* add_to_Dclock(int time, Semaphore* sem)
{
	if (head == NULL)
	{
		return (create_list(time, sem)); SWAP;
	}

	struct DeltaC *ptr = head; SWAP;
	struct DeltaC *ptr_add = (struct DeltaC*)malloc(sizeof(struct DeltaC)); SWAP;
	if (ptr == NULL)
	{
		printf("\n Node creation failed \n"); SWAP;
		return NULL;
	}
	
	if (ptr->time < time)
	{
		while (ptr->next != NULL)
		{
			if (ptr->next->time < time)
			{
				time = time - ptr->time; SWAP;
				ptr = ptr->next; SWAP;
			}
			else
			{
				ptr_add->time = time; SWAP;
				ptr->sem = sem; SWAP;
				ptr->next->time -= time; SWAP;
				ptr_add->next = ptr->next; SWAP;
				ptr->next = ptr_add; SWAP;
				return ptr_add;
			}
		}
		ptr_add->time = time - ptr->time; SWAP;
		ptr->sem = sem; SWAP;
		back->next = ptr_add; SWAP;
		back = ptr_add; SWAP;
		return ptr_add;
	}
	else
	{
		ptr_add->time = time; SWAP;
		ptr->sem = sem; SWAP;
		head->time -= time; SWAP;
		ptr_add->next = head; SWAP;
		head = ptr_add; SWAP;
		return ptr_add;
	}
}

struct DeltaC* search_in_list(int time, struct DeltaC **prev)
{
	struct DeltaC *ptr = head;
	struct DeltaC *tmp = NULL;
	bool found = FALSE;

	printf("\n Searching the list for value [%d] \n", time);

	while (ptr != NULL)
	{
		if (ptr->time == time)
		{
			found = TRUE;
			break;
		}
		else
		{
			tmp = ptr;
			ptr = ptr->next;
		}
	}

	if (TRUE == found)
	{
		if (prev)
			*prev = tmp;
		return ptr;
	}
	else
	{
		return NULL;
	}
}

int delete_from_Dclock()
{
	int status = -1;
	if (head != NULL)
	{
		while (head->time == 0)
		{
			struct DeltaC *del = head; SWAP;
			head = head->next; SWAP;
			SEM_SIGNAL(del->sem); SWAP;
			free(del); SWAP;
			del = NULL; SWAP;
			status = 0; SWAP;
		}
		head->time--; SWAP;
	}
	return status;
}

void print_list(void)
{
	struct DeltaC *ptr = head; SWAP;

	printf("\n -------Printing list Start------- \n"); SWAP;
	while (ptr != NULL)
	{
		printf("\n [%d] %s \n", ptr->time, ptr->sem->name); SWAP;
		ptr = ptr->next; SWAP;
	}
	printf("\n -------Printing list End------- \n"); SWAP;

	return;
}*/

