/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          June 2000
 RCS:           $Id: sighndl.cc,v 1.5 2002-06-26 16:34:41 bert Exp $
________________________________________________________________________

-*/

static const char* rcsID = "$Id: sighndl.cc,v 1.5 2002-06-26 16:34:41 bert Exp $";

#include "sighndl.h"
#include "strmdata.h"
#include "strmprov.h"
#include "errh.h"


void SignalHandling::startNotify( SignalHandling::EvType et, const CallBack& cb)
{
    CallBackList& cbs = theinst_.getCBL( et );
    if ( cbs.indexOf(cb) < 0 ) cbs += cb;
}


void SignalHandling::stopNotify( SignalHandling::EvType et, const CallBack& cb )
{
    CallBackList& cbs = theinst_.getCBL( et );
    cbs -= cb;
}


CallBackList& SignalHandling::getCBL( SignalHandling::EvType et )
{
    switch ( et )
    {
    case ConnClose:	return conncbs;
    case ChldStop:	return chldcbs;
    case ReInit:	return reinitcbs;
    case Stop:		return stopcbs;
    case Cont:		return contcbs;
    default:		return killcbs;
    }
}

#ifdef __msvc__

SignalHandling::SignalHandling() {}

#else

#include <signal.h>
#include <unistd.h>


SignalHandling::SignalHandling()
{
    if ( !getenv("dGB_NO_OS_EVENT_HANDLING") )
    {

#define mCatchSignal(nr) (void)signal( nr, &SignalHandling::handle )

    // Fatal stuff
    mCatchSignal( SIGHUP );	/* Hangup */
    mCatchSignal( SIGINT );	/* Interrupt */
    mCatchSignal( SIGQUIT );	/* Quit */
    mCatchSignal( SIGILL );	/* Illegal instruction */
    mCatchSignal( SIGTRAP );	/* Trace trap */
    mCatchSignal( SIGIOT );	/* IOT instruction */
    mCatchSignal( SIGABRT );	/* Used by ABORT (IOT) */
    mCatchSignal( SIGFPE );	/* Floating point */
    mCatchSignal( SIGBUS );	/* Bus error */
    mCatchSignal( SIGSEGV );	/* Segmentation fault */
    mCatchSignal( SIGTERM );	/* Software termination */
    mCatchSignal( SIGXCPU );	/* Cpu time limit exceeded */
    mCatchSignal( SIGXFSZ );	/* File size limit exceeded */
#ifdef sun5
    mCatchSignal( SIGEMT );	/* Emulator trap */
    mCatchSignal( SIGSYS );	/* Bad arg system call */
#endif

    // Stuff to ignore
    mCatchSignal( SIGALRM );	/* Alarm clock */
    mCatchSignal( SIGURG );	/* Urgent condition */
    mCatchSignal( SIGTTIN );	/* Background read */
    mCatchSignal( SIGTTOU );	/* Background write */
    mCatchSignal( SIGVTALRM );	/* Virtual time alarm */
    mCatchSignal( SIGPROF );	/* Profiling timer alarm */
    mCatchSignal( SIGWINCH );	/* Window changed size */
#ifdef sun5
    mCatchSignal( SIGPOLL );	/* I/O is possible on a channel */
#else
    mCatchSignal( SIGIO );
#endif

    // Have to handle
    mCatchSignal( SIGSTOP );	/* Stop process */
    mCatchSignal( SIGTSTP );	/* Software stop process (e.g. ) */
    mCatchSignal( SIGCONT );	/* Continue after stop */
    mCatchSignal( SIGPIPE );	/* Write on a pipe, no one listening */
    mCatchSignal( SIGCLD );	/* Child status changed */

    }
}

void SignalHandling::handle( int signalnr )
{
    switch( signalnr )
    {
    case SIGINT: case SIGQUIT: case SIGILL: case SIGTRAP:
    case SIGABRT: case SIGFPE: case SIGBUS: case SIGSEGV:
    case SIGTERM: case SIGXCPU: case SIGXFSZ:
#ifdef sun5
    case SIGEMT: case SIGSYS:
#endif
					theinst_.doKill( signalnr );	break;

    case SIGSTOP: case SIGTSTP:		theinst_.doStop( signalnr );	return; 
    case SIGCONT:			theinst_.doCont();		return;

    case SIGPIPE:			theinst_.handleConn();		break;
    case SIGCLD:			theinst_.handleChld();		break;
    case SIGHUP:			theinst_.handleReInit();	break;

    }

    // re-set signal
    mCatchSignal( signalnr );
}

#define mReleaseSignal(nr) (void)signal( nr, SIG_DFL );

void SignalHandling::doKill( int signalnr )
{
    mReleaseSignal( signalnr );
    if ( signalnr != SIGTERM )
	ErrMsg(
#ifdef lux
	    	"Stopped by Linux"
#else
# ifdef __sun__
	    	"Stopped by Solaris"
# else
#  ifdef __win__
	    	"Killed by Windows"
#  else
	    	"Terminated by Operating System"
#  endif
# endif
#endif
		);
    killcbs.doCall( this );
    exit( 1 );
}


void SignalHandling::doStop( int signalnr )
{
    mReleaseSignal( signalnr );
    stopcbs.doCall( this );
    kill( getPID(), signalnr );
}


void SignalHandling::stopProcess( int pid, bool friendly )
{
    kill( pid, friendly ? SIGTERM : SIGKILL );
}


void SignalHandling::stopRemote( const char* mach, int pid, bool friendly )
{
    if ( !mach || !*mach )
	{ stopProcess( pid, friendly ); }

    BufferString cmd( mach );
    cmd += ":@kill ";
    cmd += friendly ? "-TERM " : "-9 ";
    cmd += pid;
    StreamData sd = StreamProvider( cmd ).makeOStream();
    sd.close();
}


void SignalHandling::doCont()
{
    mCatchSignal( SIGSTOP );
    contcbs.doCall( this );
}


void SignalHandling::handleConn()
{
    conncbs.doCall( this );
}


void SignalHandling::handleChld()
{
    chldcbs.doCall( this );
}


void SignalHandling::handleReInit()
{
    if ( reinitcbs.size() )
	reinitcbs.doCall( this );
    else
	doKill( SIGHUP );
}

#endif
