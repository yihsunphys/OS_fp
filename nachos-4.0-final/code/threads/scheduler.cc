// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "scheduler.h"
#include "main.h"
#include <limits.h>

namespace
{
    const int L3_PRIORITY_LOWER_BOUND = 0;
    const int L2_PRIORITY_LOWER_BOUND = 50;
    const int L1_PRIORITY_LOWER_BOUND = 100;
    const int L1_PRIORITY_UPPER_BOUND = 150;
    const int RR_TIME_QUANTUM = 200;
}

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------



//<TODO>
// Declare sorting rule of SortedList for L1 & L2 ReadyQueue
// Hint: Funtion Type should be "static int"
//<TODO>
int Scheduler::L1Comparator(Thread* a, Thread* b)
{
    return a->getRemainingBurstTime() < b->getRemainingBurstTime() ? -1 : 1;
}
int Scheduler::L2Comparator(Thread* a, Thread* b)
{
    return a->getID() < b->getID() ? -1 : 1;
}

Scheduler::Scheduler()
{
//	schedulerType = type;
    // readyList = new List<Thread *>; 
    //<TODO>
    // Initialize L1, L2, L3 ReadyQueue
    //<TODO>
    L1ReadyQueue = new SortedList<Thread*>(&Scheduler::L1Comparator);
    L2ReadyQueue = new SortedList<Thread*>(&Scheduler::L2Comparator);
    L3ReadyQueue = new List<Thread*>();
	toBeDestroyed = NULL;
    start = kernel->stats->totalTicks;
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    //<TODO>
    // Remove L1, L2, L3 ReadyQueue
    //<TODO>
    // delete readyList;
    delete L1ReadyQueue;
    delete L2ReadyQueue;
    delete L3ReadyQueue;
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    // DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());
    
    Statistics* stats = kernel->stats;
    //<TODO>
    // According to priority of Thread, put them into corresponding ReadyQueue.
    // After inserting Thread into ReadyQueue, don't forget to reset some values.
    // Hint: L1 ReadyQueue is preemptive SRTN(Shortest Remaining Time Next).
    // When putting a new thread into L1 ReadyQueue, you need to check whether preemption or not.
    thread->setWaitTime(0);  // Reset wait time to 0
    thread->setStatus(READY);
    thread->setWaiting(kernel->stats->totalTicks);
    int priority = thread->getPriority();
    int level;
    if (priority >= L3_PRIORITY_LOWER_BOUND && priority < L2_PRIORITY_LOWER_BOUND) 
    {
        L3ReadyQueue->Append(thread);
        level = 3;
    } 
    else if (priority < L1_PRIORITY_LOWER_BOUND) 
    {
        L2ReadyQueue->Insert(thread);
        level = 2;
    } 
    else if (priority < L1_PRIORITY_UPPER_BOUND) 
    {
        L1ReadyQueue->Insert(thread);
        level = 1;
    } 
    else 
    {
        cout << "Invalid Priority!" << endl;
        Abort();
    }
    DEBUG('z', "[InsertToQueue] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getID() << "] is inserted into queue L" << level);
    //<TODO>
    // readyList->Append(thread);

    
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun ()
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    /*if (readyList->IsEmpty()) {
    return NULL;
    } else {
        return readyList->RemoveFront();
    }*/

    //<TODO>
    // a.k.a. Find Next (Thread in ReadyQueue) to Run
    Thread* thread = NULL;
    if (!L1ReadyQueue->IsEmpty()) 
    {
        currentLayer = 1;
        thread = L1ReadyQueue->RemoveFront();
    }
    else if (!L2ReadyQueue->IsEmpty())
    {
        currentLayer = 2;
        thread = L2ReadyQueue->RemoveFront();
    }
    else if (!L3ReadyQueue->IsEmpty())
    {
        currentLayer = 3;
        thread = L3ReadyQueue->RemoveFront();
    }
    if (thread != NULL)
        DEBUG('z', "[RemoveFromQueue] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getID() <<"] is removed from queue L"<< currentLayer);
    
    return thread;
    //<TODO>

}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread, bool finishing)
{
    Thread *oldThread = kernel->currentThread;
    
	//cout << "Current Thread" <<oldThread->getName() << "    Next Thread"<<nextThread->getName()<<endl;
   
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (finishing) {	// mark that we need to delete current thread
         ASSERT(toBeDestroyed == NULL);
	     toBeDestroyed = oldThread;
    }
   
#ifdef USER_PROGRAM			// ignore until running user programs 
    if (oldThread->space != NULL) {	// if this thread is a user program,

        oldThread->SaveUserState(); 	// save the user's CPU registers
	    oldThread->space->SaveState();
    }
#endif
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    kernel->currentThread = nextThread;  // switch to the next thread
    nextThread->setStatus(RUNNING);      // nextThread is now running
    
    // DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());
    if(oldThread->getID()!=nextThread->getID())
      DEBUG('z', "[ContextSwitch] Tick [" << kernel->stats->totalTicks << "]: Thread ["<< nextThread->getID() <<"] is now selected for execution, thread [" <<oldThread->getID()<<"] is replaced, and it has executed [" << RunTime()<<"]");
    start = kernel->stats->totalTicks;
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".
    if(oldThread->getID()!=nextThread->getID())
      cout << "Switching from: " << oldThread->getID() << " to: " << nextThread->getID() << endl;
    SWITCH(oldThread, nextThread);

    // we're back, running oldThread
      
    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    DEBUG(dbgThread, "Now in thread: " << kernel->currentThread->getID());

    CheckToBeDestroyed();		// check if thread we were running
					// before this one has finished
					// and needs to be cleaned up
    
#ifdef USER_PROGRAM
    if (oldThread->space != NULL) {	    // if there is an address space
        oldThread->RestoreUserState();     // to restore, do it.
	    oldThread->space->RestoreState();
    }
#endif
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------

void
Scheduler::CheckToBeDestroyed()
{
    if (toBeDestroyed != NULL) {
        DEBUG(dbgThread, "toBeDestroyed->getID(): " << toBeDestroyed->getID());
        delete toBeDestroyed;
	    toBeDestroyed = NULL;
    }
}
 
//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    cout << "Ready list contents:\n";
    // readyList->Apply(ThreadPrint);
    L1ReadyQueue->Apply(ThreadPrint);
    L2ReadyQueue->Apply(ThreadPrint);
    L3ReadyQueue->Apply(ThreadPrint);
}

// <TODO>

// Function 1. Function definition of sorting rule of L1 ReadyQueue

// Function 2. Function definition of sorting rule of L2 ReadyQueue

// Function 3. Scheduler::UpdatePriority()
// Hint:
// 1. ListIterator can help.
// 2. Update WaitTime and priority in Aging situations
// 3. After aging, Thread may insert to different ReadyQueue

void Scheduler::UpdatePriority() {
    // L1: Update Priority if necessary
    int time = kernel->stats->totalTicks;
    ListIterator<Thread *> iter1(L1ReadyQueue); // It's also ok to not update in L1
    for (; !iter1.IsDone(); iter1.Next()) 
        iter1.Item()->updatePriority(time);

    // L2: We need to record those threads that update their priority
    ListIterator<Thread *> iter2(L2ReadyQueue); 
    List<Thread *> uplevel2; // Threads that goes to L1
    List<Thread *> upgrade; // Threads that has updated their priority
    // First update the priorities
    for (; !iter2.IsDone(); iter2.Next()) {
        int old = iter2.Item()->getPriority();
        int priority = iter2.Item()->updatePriority(time);
        if (old != priority) {
            if (priority >= 100) // update of level
                uplevel2.Append(iter2.Item());
            else 
                upgrade.Append(iter2.Item());
        }
    }
    // Then insert to corresponding queues
    ListIterator<Thread *> uplevel2Iter(&uplevel2);
    for (; !uplevel2Iter.IsDone(); uplevel2Iter.Next()) {
        L2ReadyQueue->Remove(uplevel2Iter.Item());
        DEBUG('z', "[RemoveFromQueue] Tick [" << kernel->stats->totalTicks << "]: Thread [" << uplevel2Iter.Item()->getID() <<"] is removed from queue L2");
        L1ReadyQueue->Insert(uplevel2Iter.Item());
        DEBUG('z', "[InsertToQueue] Tick [" << kernel->stats->totalTicks << "]: Thread [" << uplevel2Iter.Item()->getID() << "] is inserted into queue L1");
    }
    ListIterator<Thread *> upgradeIter(&upgrade);
    for (; !upgradeIter.IsDone(); upgradeIter.Next()) {
        L2ReadyQueue->Remove(upgradeIter.Item());
        DEBUG('z', "[RemoveFromQueue] Tick [" << kernel->stats->totalTicks << "]: Thread [" << upgradeIter.Item()->getID() <<"] is removed from queue L2");
        L2ReadyQueue->Insert(upgradeIter.Item());
        DEBUG('z', "[InsertToQueue] Tick [" << kernel->stats->totalTicks << "]: Thread [" << upgradeIter.Item()->getID() << "] is inserted into queue L2");
    }

    // L3: Similarly use a list to store who will go uplevel
    ListIterator<Thread *> iter3(L3ReadyQueue); 
    List<Thread *> uplevel3;
    for (; !iter3.IsDone(); iter3.Next()) {
        int priority = iter3.Item()->updatePriority(time);
        if (priority >= 50)  // update of level 
            uplevel3.Append(iter3.Item());
    }
    ListIterator<Thread *> uplevel3Iter(&uplevel3);
    for (; !uplevel3Iter.IsDone(); uplevel3Iter.Next()) {
        L3ReadyQueue->Remove(uplevel3Iter.Item());
        DEBUG('z', "[RemoveFromQueue] Tick [" << kernel->stats->totalTicks << "]: Thread [" << uplevel3Iter.Item()->getID() <<"] is removed from queue L3");
        L2ReadyQueue->Insert(uplevel3Iter.Item());
        DEBUG('z', "[InsertToQueue] Tick [" << kernel->stats->totalTicks << "]: Thread [" << uplevel3Iter.Item()->getID() << "] is inserted into queue L2");
    }
}
bool Scheduler::ToYield () {
    bool yield = false;
    int currentThreadRemainTime = kernel->currentThread->getRemainingBurstTime() - RunTime();
    int L1MinRemainTime = L1ReadyQueue->IsEmpty() ? INT_MAX : L1ReadyQueue->Front()->getRemainingBurstTime();

    if (currentLayer == 3 && RunTime() >= RR_TIME_QUANTUM)
    {
        if (RunTime() >= RR_TIME_QUANTUM || !L1ReadyQueue->IsEmpty() || !L2ReadyQueue->IsEmpty())
            yield = true;
    }
    else if (currentLayer == 2 && !L1ReadyQueue->IsEmpty())
        yield = true;
    else if (currentLayer == 1 && L1MinRemainTime < currentThreadRemainTime)
        yield = true;
    return yield;
}
int Scheduler::RunTime() {
    return kernel->stats->totalTicks - start;
}
// <TODO>