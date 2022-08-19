#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "commandlineparser.h"
#include "envvars.h"
#include "od_ostream.h"
#include "oddirs.h"
#include "oscommand.h"

#ifdef __win__
# include <tchar.h>
# include <tlhelp32.h>
# include <Windows.h>
#else
# include "sys/resource.h"
#endif

/*
  The implementation of Execute_batch should be in the executable on
  windows, but can be in a .so on *nix.
  In order not to pollute batchprog.h, I've placed the implementation
  into a separate file, which is included trough batchprog.h on win32
  and included in batchprog.cc on *nix.
*/

#ifdef __win__
static void setBatchPriority( int argc, char** argv, float priority )
#else
static void setBatchPriority( int argc, char** argv, float priority, int pid )
#endif
{
    const CommandLineParser clp( argc, argv );
    //TODO: replace by reading the IOPar
    clp.getVal( OS::CommandExecPars::sKeyPriority(), priority );
#ifdef __unix__
    if ( mIsUdf(priority) )
    {
	int nicelvl = mUdf(int);
	if ( !clp.getVal("nice",nicelvl) )
	    return;

	setpriority( PRIO_PROCESS, pid, nicelvl );
    }
    else
    {
	const int machprio =
		  OS::CommandExecPars::getMachinePriority( priority, false );
	setpriority( PRIO_PROCESS, pid, machprio );
    }
#else
    if ( mIsUdf(priority) )
	return;

    const int machprio =
	      OS::CommandExecPars::getMachinePriority( priority, true );
    const DWORD threadpriority =
	machprio == 8 ? THREAD_PRIORITY_NORMAL
		      : ( machprio == 7 ? THREAD_PRIORITY_BELOW_NORMAL
					: THREAD_PRIORITY_LOWEST );
    if ( threadpriority != THREAD_PRIORITY_NORMAL )
	SetPriorityClass( GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS );

    HANDLE curthread = INVALID_HANDLE_VALUE;
    THREADENTRY32 threadlist;

    const DWORD dwOwnerPID( GetCurrentProcessId() );
    curthread = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, dwOwnerPID );
    if ( curthread == INVALID_HANDLE_VALUE )
	return;

    threadlist.dwSize = sizeof(THREADENTRY32);
    if ( !Thread32First(curthread,&threadlist) )
	{ CloseHandle(curthread); return; }

    do
    {
	if ( threadlist.th32OwnerProcessID != dwOwnerPID )
	    continue;

	SetThreadPriority(curthread,threadpriority );
    } while ( Thread32Next(curthread,&threadlist) );

    CloseHandle( curthread );
#endif
}

void Execute_batch( int* pargc, char** argv )
{
    PIM().loadAuto( false );
    if ( !BP().init() )
	return;

    PIM().loadAuto( true );
#ifdef __win__
    setBatchPriority( *pargc, argv, BP().getPriority() );
#else
    setBatchPriority( *pargc, argv, BP().getPriority(), GetPID() );
#endif
}

void loadModulesCB( CallBacker* )
{
    BP().loadModules();
    BP().modulesLoaded();
}


void doWorkCB( CallBacker* )
{
    BatchProgram& bp = BP();
    bp.initWork();
    const bool res = bp.doWork( *bp.strm_ );
    bp.postWork( res );
}


void launchDoWorkCB( CallBacker* cb )
{
    BatchProgram& bp = BP();
    if ( bp.canReceiveRequests() )
    {
        bp.startTimer();
        Threads::Locker lckr( bp.batchprogthreadlock_ );
        bp.thread_ = new Threads::Thread(mSCB(doWorkCB),
                    "Batch program executor");
    }
    else
    {
        doWorkCB( cb );
        bp.endWorkCB( cb );
    }
}
