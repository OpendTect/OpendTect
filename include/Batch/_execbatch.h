#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Lammertink
 Date:		30-10-2003
________________________________________________________________________

 The implementation of Execute_batch should be in the executable on
 windows, but can be in a .so on *nix.
 In order not to pollute batchprog.h, I've placed the implementation
 into a separate file, which is included trough batchprog.h on win32
 and included in batchprog.cc on *nix.

*/

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


#ifdef __win__
static void setBatchPriority( int argc, char** argv )
#else
static void setBatchPriority( int argc, char** argv, int pid )
#endif
{
    const CommandLineParser clp( argc, argv );
    const auto priority = clp.keyedValue<float>( "priority" );
#ifdef __unix__
    if ( mIsUdf(priority) )
    {
	const auto nicelvl = clp.keyedValue<int>( "nice" );
	if ( mIsUdf(nicelvl) )
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

    BP().init();
    if ( !BP().stillok_ )
	return;

    BatchProgram& bp = BP();
    bool allok = bp.initOutput();
    if ( allok )
    {
	od_ostream& logstrm = bp.strm_ ? *bp.strm_ : od_cout();
	const int pid = GetPID();
#ifdef __win__
	setBatchPriority( *pargc, argv );
#else
	setBatchPriority( *pargc, argv, pid );
#endif
	logstrm << "Starting program: " << argv[0] << " " << bp.name()
		<< od_endl;
	logstrm << "Processing on: " << GetLocalHostName() << od_endl;
	logstrm << "Process ID: " << pid << od_endl;
	allok = bp.go( logstrm );
    }

    bp.stillok_ = allok;
    const int ret = allok ? 0 : 1;
    BatchProgram::deleteInstance( ret );

    ApplicationData::exit( ret );	// never reached.
}
