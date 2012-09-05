/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          June 2000
 RCS:           $Id: sighndl.cc,v 1.36 2012-09-05 07:28:26 cvsjaap Exp $
________________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id: sighndl.cc,v 1.36 2012-09-05 07:28:26 cvsjaap Exp $";

#include "sighndl.h"
#include "strmdata.h"
#include "strmprov.h"
#include "envvars.h"
#include "errh.h"

#include <signal.h>
#include <iostream>

#ifndef __win__
# include <unistd.h>
#endif

#ifdef __win__
# include "winterminate.h"
#endif
#ifdef __mac__
# define SIGCLD SIGCHLD
#endif

SignalHandling SignalHandling::theinst_;


void SignalHandling::startNotify( SignalHandling::EvType et, const CallBack& cb)
{
    CallBackSet& cbs = theinst_.getCBL( et );
    if ( cbs.indexOf(cb) < 0 ) cbs += cb;
#ifndef __win__
    if ( et == SignalHandling::Alarm )
    {
	/* tell OS not to restart system calls if a signal is received */
	/* see: http://www.cs.ucsb.edu/~rich/class/cs290I-grid/notes/Sockets/ */
	if ( siginterrupt(SIGALRM, 1) < 0 ) 
	{   
	    std::cout <<"WARNING: setting of siginterrupt failed. ";
	    std::cout <<"System operations might not be interuptable ";
	    std::cout <<"using alarm signals." << std::endl;
	}
    }
#endif
    
}


void SignalHandling::stopNotify( SignalHandling::EvType et, const CallBack& cb )
{
    CallBackSet& cbs = theinst_.getCBL( et );
    cbs -= cb;
}


CallBackSet& SignalHandling::getCBL( SignalHandling::EvType et )
{
    switch ( et )
    {
    case ConnClose:	return conncbs;
    case ChldStop:	return chldcbs;
    case ReInit:	return reinitcbs;
    case Stop:		return stopcbs;
    case Cont:		return contcbs;
    case Alarm:		return alarmcbs;
    default:		return killcbs;
    }
}


#define mCatchSignal(nr) (void)signal( nr, &SignalHandling::handle )

SignalHandling::SignalHandling()
{
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
    mCatchSignal( SIGTSTP );	/* Software stop process (e.g. ) */
    mCatchSignal( SIGCONT );	/* Continue after stop */
    mCatchSignal( SIGPIPE );	/* Write on a pipe, no one listening */
    mCatchSignal( SIGHUP );	/* Hangup */

#endif

    }
}


void SignalHandling::initFatalSignalHandling()
{
    if ( GetEnvVarYN( "DTECT_HANDLE_FATAL") )
    {
	// Fatal stuff
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
					theinst_.doKill( signalnr );	break;

#ifndef __win__
    case SIGSTOP: case SIGTSTP:		theinst_.doStop( signalnr );	return; 
    case SIGCONT:			theinst_.doCont();		return;

    case SIGALRM:			theinst_.handleAlarm();		break;
    case SIGPIPE:			theinst_.handleConn();		break;
    case SIGCLD:			theinst_.handleChld();		break;
    case SIGHUP:			theinst_.handleReInit();	break;
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
    killcbs.doCall( this, 0 );
    ExitProgram( 1 );
}


void SignalHandling::doStop( int signalnr, bool withcbs )
{
    mReleaseSignal( signalnr );

    if ( withcbs )
	stopcbs.doCall( this, 0 );

#ifdef __win__
    raise( signalnr );
#else
    kill( GetPID(), signalnr );
#endif
}


void SignalHandling::stopProcess( int pid, bool friendly )
{
#ifdef __win__
    TerminateApp( pid, 0 );
#else
    kill( pid, friendly ? SIGTERM : SIGKILL );
#endif
}


void SignalHandling::stopRemote( const char* mach, int pid, bool friendly,
				 const char* rshcomm )
{
    if ( !mach || !*mach )
	{ stopProcess( pid, friendly ); }

    BufferString cmd( mach );
    cmd += ":@'kill ";
    cmd += friendly ? "-TERM " : "-9 ";
    cmd += pid;
    cmd += " > /dev/null'";

    StreamProvider strmp( cmd );
    if ( rshcomm && *rshcomm )
	strmp.setRemExec( rshcomm );
    StreamData sd = strmp.makeOStream();
    sd.close();
}


void SignalHandling::doCont()
{
#ifndef __win__
    mCatchSignal( SIGSTOP );
#endif
    contcbs.doCall( this, 0 );
}


void SignalHandling::handleConn()
{
    conncbs.doCall( this, 0 );
}


void SignalHandling::handleChld()
{
    chldcbs.doCall( this, 0 );
}


void SignalHandling::handleReInit()
{
    reinitcbs.doCall( this, 0 );
}


void SignalHandling::handleAlarm()
{
    alarmcbs.doCall( this, 0 );
}
