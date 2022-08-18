/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "applicationdata.h"
#include "sighndl.h"
#include "envvars.h"
#include "oscommand.h"
#include "genc.h"

#include <signal.h>
#include <new>

#ifdef __win__
# include <windows.h>
  static DWORD WINAPI TerminateApp(DWORD,DWORD);
# define TA_FAILED 0
# define TA_SUCCESS_CLEAN 1
# define TA_SUCCESS_KILL 2
#else
# include <unistd.h>
#endif

#ifdef __mac__
# define SIGCLD SIGCHLD
#endif

static void mem_alloc_fail()
{
    ErrMsg("Memory allocation failed");
    DBG::forceCrash(true);
};



SignalHandling& SignalHandling::SH()
{
    mDefineStaticLocalObject( SignalHandling, theinst, );
    return theinst;
}


void SignalHandling::initClass()
{
    SH();
}


void SignalHandling::startNotify( SignalHandling::EvType et, const CallBack& cb)
{

    CallBackSet& cbs = SH().getCBL( et );
    if ( !cbs.isPresent(cb) ) cbs += cb;
#ifndef __win__
    if ( et == SignalHandling::Alarm )
    {
	/* tell OS not to restart system calls if a signal is received */
	/* see: http://www.cs.ucsb.edu/~rich/class/cs290I-grid/notes/Sockets/ */
	if ( siginterrupt(SIGALRM, 1) < 0 )
	{
	    // setting of siginterrupt failed
	    // System operations might not be interuptable using alarm signals.
	}
    }
#endif

}


void SignalHandling::stopNotify( SignalHandling::EvType et, const CallBack& cb )
{
    CallBackSet& cbs = SH().getCBL( et );

    cbs -= cb;
}


CallBackSet& SignalHandling::getCBL( SignalHandling::EvType et )
{
    switch ( et )
    {
    case ConnClose:	return conncbs_;
    case ChldStop:	return chldcbs_;
    case ReInit:	return reinitcbs_;
    case Stop:		return stopcbs_;
    case Cont:		return contcbs_;
    case Alarm:		return alarmcbs_;
    default:		return killcbs_;
    }
}


#define mCatchSignal(nr) (void)signal( nr, &SignalHandling::handle )

SignalHandling::SignalHandling()
    : conncbs_(*new CallBackSet)
    , chldcbs_(*new CallBackSet)
    , reinitcbs_(*new CallBackSet)
    , stopcbs_(*new CallBackSet)
    , contcbs_(*new CallBackSet)
    , alarmcbs_(*new CallBackSet)
    , killcbs_(*new CallBackSet)
{
    conncbs_.ref(); chldcbs_.ref(); reinitcbs_.ref(); stopcbs_.ref();
    contcbs_.ref(); alarmcbs_.ref(); killcbs_.ref();

    if ( !GetEnvVarYN("DTECT_NO_OS_EVENT_HANDLING") )
    {
    initFatalSignalHandling();

#ifndef __win__
    // Stuff to ignore
    mCatchSignal( SIGURG );	/* Urgent condition */
    mCatchSignal( SIGTTIN );	/* Background read */
    mCatchSignal( SIGTTOU );	/* Background write */
    mCatchSignal( SIGVTALRM );	/* Virtual time alarm */
    mCatchSignal( SIGPROF );	/* Profiling timer alarm */
    mCatchSignal( SIGWINCH );	/* Window changed size */

# ifdef sun5
    mCatchSignal( SIGPOLL );	/* I/O is possible on a channel */
# else
    mCatchSignal( SIGIO );
# endif

    // Have to handle
    mCatchSignal( SIGALRM );	/* Alarm clock */
    mCatchSignal( SIGSTOP );	/* Stop process */
    mCatchSignal( SIGTSTP );	/* Software stop process (e.g. Ctrl-Z) */
    mCatchSignal( SIGCONT );	/* Continue after stop */
    mCatchSignal( SIGPIPE );	/* Write on a pipe, no one listening */
    mCatchSignal( SIGHUP );	/* Hangup */

#endif

    }
}


SignalHandling::~SignalHandling()
{
    conncbs_.unRef(); chldcbs_.unRef(); reinitcbs_.unRef(); stopcbs_.unRef();
    contcbs_.unRef(); alarmcbs_.unRef(); killcbs_.unRef();
}


void SignalHandling::initFatalSignalHandling()
{
    if ( GetEnvVarYN( "DTECT_HANDLE_FATAL") )
    {
	// Fatal stuf
	std::set_new_handler(mem_alloc_fail );
	mCatchSignal( SIGINT );	/* Interrupt */
	mCatchSignal( SIGILL );	/* Illegal instruction */
	mCatchSignal( SIGFPE );	/* Floating point */
	mCatchSignal( SIGSEGV );/* Segmentation fault */
	mCatchSignal( SIGTERM );/* Software termination */
	mCatchSignal( SIGABRT );/* Used by ABORT (IOT) */

#ifdef __win__
	mCatchSignal( SIGBREAK );/* Control-break */
#else
	mCatchSignal( SIGQUIT );/* Quit */
	mCatchSignal( SIGTRAP );/* Trace trap */
	mCatchSignal( SIGIOT );	/* IOT instruction */
	mCatchSignal( SIGBUS );	/* Bus error */
	mCatchSignal( SIGXCPU );/* Cpu time limit exceeded */
	mCatchSignal( SIGXFSZ );/* File size limit exceeded */
# ifdef sun5
	mCatchSignal( SIGEMT );	/* Emulator trap */
	mCatchSignal( SIGSYS );	/* Bad arg system call */
# endif
#endif
    }
}



void SignalHandling::handle( int signalnr )
{
    switch( signalnr )
    {
    case SIGINT: case SIGFPE: case SIGSEGV: case SIGTERM:
    case SIGILL: case SIGABRT:
#ifdef __win__
    case SIGBREAK:
#else
    case SIGQUIT: case SIGTRAP: case SIGBUS: case SIGXCPU: case SIGXFSZ:
#endif
#ifdef sun5
    case SIGEMT: case SIGSYS:
#endif
					SH().doKill( signalnr );	break;

#ifndef __win__
    case SIGSTOP: case SIGTSTP:		SH().doStop( signalnr );	return;
    case SIGCONT:			SH().doCont();		return;

    case SIGALRM:			SH().handleAlarm();	break;
    case SIGPIPE:			SH().handleConn();		break;
    case SIGCLD:			SH().handleChld();		break;
    case SIGHUP:			SH().handleReInit();	break;
#endif
    }

    // re-set signal
    mCatchSignal( signalnr );
}

#define mReleaseSignal(nr) (void)signal( nr, SIG_DFL );

void SignalHandling::doKill( int signalnr )
{
    mReleaseSignal( signalnr );
    if ( signalnr != SIGTERM )
    {
	BufferString msg;
#ifdef __lux__
	msg = "Stopped by Linux";
# ifdef __debug__
	msg += ", received signal: ";

	#define SIGERRORMSG( x ) case x: msg += #x  ; break
	switch ( signalnr )
	{
	    SIGERRORMSG( SIGHUP );
	    SIGERRORMSG( SIGINT );
	    SIGERRORMSG( SIGQUIT );
	    SIGERRORMSG( SIGILL );
	    SIGERRORMSG( SIGTRAP );
	    SIGERRORMSG( SIGABRT );
	    SIGERRORMSG( SIGBUS );
	    SIGERRORMSG( SIGFPE );
	    SIGERRORMSG( SIGKILL );
	    SIGERRORMSG( SIGUSR1 );
	    SIGERRORMSG( SIGSEGV );
	    SIGERRORMSG( SIGUSR2 );
	    SIGERRORMSG( SIGPIPE );
	    SIGERRORMSG( SIGALRM );
	    SIGERRORMSG( SIGTERM );
	    SIGERRORMSG( SIGSTKFLT );
	    SIGERRORMSG( SIGCHLD );
	    SIGERRORMSG( SIGCONT );
	    SIGERRORMSG( SIGSTOP );
	    SIGERRORMSG( SIGTSTP );
	    SIGERRORMSG( SIGTTIN );
	    SIGERRORMSG( SIGTTOU );
	    SIGERRORMSG( SIGURG );
	    SIGERRORMSG( SIGXCPU );
	    SIGERRORMSG( SIGXFSZ );
	    SIGERRORMSG( SIGVTALRM );
	    SIGERRORMSG( SIGPROF );
	    SIGERRORMSG( SIGWINCH );
	    SIGERRORMSG( SIGIO );
	    SIGERRORMSG( SIGPWR );
	    SIGERRORMSG( SIGSYS );
	    default: msg += "unknown signal: "; msg += signalnr;
	}
# endif
#else
# ifdef __sun__
	msg = "Stopped by Solaris";
# else
#  ifdef __win__
	msg = "Killed by Windows";
#  else
	msg = "Terminated by Operating System";
#  endif
# endif
#endif
	ErrMsg( msg );
    }
    killcbs_.doCall( this );
    ApplicationData::exit( 1 );
}


void SignalHandling::doStop( int signalnr, bool withcbs )
{
    mReleaseSignal( signalnr );

    if ( withcbs )
	stopcbs_.doCall( this );

#ifdef __win__
    raise( signalnr );
#else
    kill( GetPID(), signalnr );
#endif
}


void SignalHandling::stopProcess( PID_Type pid, bool friendly )
{
#ifdef __win__
    TerminateApp( pid, 0 );
#else
    kill( pid, friendly ? SIGTERM : SIGKILL );
#endif
}


void SignalHandling::stopRemote( const char* mach, PID_Type pid, bool friendly,
				 const char* rshcomm )
{
    if ( !mach || !*mach )
	{ stopProcess( pid, friendly ); return; }

#ifdef __win__
    pFreeFnErrMsg( "Not impl: stopRemote() for Windows");
#else

    OS::MachineCommand machcomm( "kill", friendly ? "-TERM" : "-9",
					toString(pid) );
    machcomm.addFileRedirect( "/dev/null" );
    machcomm.execute( OS::RunInBG );

#endif
}


void SignalHandling::doCont()
{
#ifndef __win__
    mCatchSignal( SIGSTOP );
#endif
    contcbs_.doCall( this );
}


void SignalHandling::handleConn()
{
    conncbs_.doCall( this );
}


void SignalHandling::handleChld()
{
    chldcbs_.doCall( this );
}


void SignalHandling::handleReInit()
{
    reinitcbs_.doCall( this );
}


void SignalHandling::handleAlarm()
{
    alarmcbs_.doCall( this );
}


#ifdef __win__

 /* SOURCE   : http://support.microsoft.com/kb/178893 -*/

static BOOL CALLBACK TerminateAppEnum( HWND hwnd, LPARAM lParam )
{
    DWORD dwID;
    GetWindowThreadProcessId( hwnd, &dwID );
    if ( dwID == (DWORD)lParam )
	PostMessage( hwnd, WM_CLOSE, 0, 0 );

    return TRUE;
}

/*----------------------------------------------------------------
  Shut down a 32-Bit Process (or 16-bit process under Windows 95)
Parameters:
    dwPID:	Process ID of the process to shut down.
    dwTimeout:	Wait time in milliseconds before shutting down the process.
Return Value:
    TA_FAILED:		If the shutdown failed.
    TA_SUCCESS_CLEAN:	If the process was shutdown using WM_CLOSE.
    TA_SUCCESS_KILL:	if the process was shut down with TerminateProcess().
----------------------------------------------------------------*/

static DWORD WINAPI TerminateApp( DWORD dwPID, DWORD dwTimeout )
{
    // If we can't open the process with PROCESS_TERMINATE rights,
    // then we give up immediately.
    HANDLE hProc = OpenProcess( SYNCHRONIZE|PROCESS_TERMINATE, FALSE, dwPID );
    if ( hProc == NULL )
	return TA_FAILED;

    // TerminateAppEnum() posts WM_CLOSE to all windows whose PID
    // matches your process's.
    EnumWindows( (WNDENUMPROC)TerminateAppEnum, (LPARAM) dwPID );

    // Wait on the handle. If it signals, great. If it times out,
    // then you kill it.
    DWORD dwRet;
    if ( WaitForSingleObject(hProc,dwTimeout) != WAIT_OBJECT_0 )
	dwRet=(TerminateProcess(hProc,0)?TA_SUCCESS_KILL:TA_FAILED);
    else
	dwRet = TA_SUCCESS_CLEAN;

    CloseHandle( hProc );

    return dwRet;
}

#endif // __win__
