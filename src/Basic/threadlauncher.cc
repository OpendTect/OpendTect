/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		May 2022
________________________________________________________________________

-*/

#include "threadlauncher.h"
#include "pythonaccess.h"
#include "thread.h"

using namespace Threads;

ThreadLauncher::ThreadLauncher()
    : finished(this)
{}


ThreadLauncher::~ThreadLauncher()
{
    if ( thread_ )
    {
	pErrMsg("Waiting for thread to finish");
	thread_->waitForFinish();
	delete thread_;
    }
    deleteAndZeroPtr( stdoutput_ );
    deleteAndZeroPtr( stderror_ );
}


void ThreadLauncher::execute( const OS::MachineCommand& mc, OS::LaunchType lt,
			      bool inpythonenv, const char* workdir)
{
    OS::CommandExecPars execpars( lt );
    deleteAndZeroPtr( stdoutput_ );
    deleteAndZeroPtr( stderror_ );
    if ( workdir )
	execpars.workingdir( workdir );
    execute( mc, execpars, inpythonenv );
}


void ThreadLauncher::execute(const OS::MachineCommand& mc,
			     bool readstdoutput, bool readstderror,
			     bool inpythonenv,
			     const char* workdir)
{
    deleteAndZeroPtr( stdoutput_ );
    deleteAndZeroPtr( stderror_ );
    if ( readstdoutput )
	stdoutput_ = new BufferString;
    if ( readstderror )
	stderror_ = new BufferString;

    OS::CommandExecPars execpars( OS::Wait4Finish );
    if ( workdir )
	execpars.workingdir( workdir );

    execute( mc, execpars, inpythonenv);

}

void ThreadLauncher::execute( const OS::MachineCommand& mc,
			      const OS::CommandExecPars& xpars,
			      bool inpythonenv)
{
    if ( thread_ )
    {
	pErrMsg("Waiting for thread to finish");
	thread_->waitForFinish();
	delete thread_;
    }

    execpars_ = xpars;
    inpythonenv_ = inpythonenv;
    machcmd_ =	mc;
    thread_ = new Threads::Thread( mCB(this, ThreadLauncher, startCB) );
}


void ThreadLauncher::startCB( CallBacker* )
{
    if ( inpythonenv_ )
    {
	if ( stdoutput_ || stderror_ )
	{
	    BufferString out;
	    OD::PythA().execute( machcmd_, out, stderror_ );
	    if ( stdoutput_ )
		stdoutput_->set( out );
	}
	else
	    OD::PythA().execute( machcmd_, execpars_ );
    }
    else if ( stdoutput_ || stderror_ )
    {
	BufferString out;
	machcmd_.execute( out, stderror_, execpars_.workingdir_ );
	if ( stdoutput_ )
	    stdoutput_->set( out );
    }
    else
	machcmd_.execute( execpars_ );

    finished.trigger();
}


BufferString ThreadLauncher::getStdOutput() const
{
    BufferString res;
    if ( stdoutput_ )
	res.set( *stdoutput_ );
    return res;
}


BufferString ThreadLauncher::getStdError() const
{
    BufferString res;
    if ( stderror_ )
	res.set( *stderror_ );
    return res;
}
