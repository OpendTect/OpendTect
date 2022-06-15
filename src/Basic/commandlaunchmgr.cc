/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		June 2022
________________________________________________________________________

-*/

#include "commandlaunchmgr.h"
#include "pythonaccess.h"
#include "threadwork.h"

using namespace Threads;

CommandTask::CommandTask( const OS::MachineCommand& mc,
			  const OS::CommandExecPars& xpars,
			  bool inpythonenv )
    : Task()
    , machcmd_(mc)
    , execpars_(xpars)
    , inpythonenv_(inpythonenv)
{}


CommandTask::CommandTask( const OS::MachineCommand& mc,
			  OS::LaunchType lt,
			  bool inpythonenv,
			  const char* workdir )
    : CommandTask(mc, OS::CommandExecPars(lt), inpythonenv)
{
    deleteAndZeroPtr( stdoutput_ );
    deleteAndZeroPtr( stderror_ );
    if ( workdir )
	execpars_.workingdir( workdir );

}


CommandTask::CommandTask( const OS::MachineCommand& mc,
			  bool readstdoutput,bool readstderror,
			  bool inpythonenv,
			  const char* workdir )
    : CommandTask(mc, OS::CommandExecPars(OS::Wait4Finish), inpythonenv)
{
    deleteAndZeroPtr( stdoutput_ );
    deleteAndZeroPtr( stderror_ );
    if ( readstdoutput )
	stdoutput_ = new BufferString;
    if ( readstderror )
	stderror_ = new BufferString;
    if ( workdir )
	execpars_.workingdir( workdir );
}


CommandTask::~CommandTask()
{}


bool CommandTask::execute()
{
    bool res = false;
    if ( inpythonenv_ )
    {
	if ( stdoutput_ || stderror_ )
	{
	    BufferString out;
	    res = OD::PythA().execute( machcmd_, out, stderror_ );
	    if ( stdoutput_ )
		stdoutput_->set( out );
	}
	else
	    res = OD::PythA().execute( machcmd_, execpars_ );
    }
    else if ( stdoutput_ || stderror_ )
    {
	BufferString out;
	res = machcmd_.execute( out, stderror_, execpars_.workingdir_ );
	if ( stdoutput_ )
	    stdoutput_->set( out );
    }
    else
	res = machcmd_.execute( execpars_ );

    return res;
}


BufferString CommandTask::getStdOutput() const
{
    BufferString res;
    if ( stdoutput_ )
	res.set( *stdoutput_ );
    return res;
}


BufferString CommandTask::getStdError() const
{
    BufferString res;
    if ( stderror_ )
	res.set( *stderror_ );
    return res;
}


CommandLaunchMgr& CommandLaunchMgr::getMgr()
{
    mDefineStaticLocalObject(CommandLaunchMgr, mgrInstance,);
    return mgrInstance;
}


CommandLaunchMgr::CommandLaunchMgr()
{
    twm_queueid_ = WorkManager::twm().addQueue(
				WorkManager::MultiThread, "CommandLaunchMgr" );
}


CommandLaunchMgr::~CommandLaunchMgr()
{
    WorkManager::twm().removeQueue( twm_queueid_, false );
}


void CommandLaunchMgr::execute( const OS::MachineCommand& mc, OS::LaunchType lt,
				CallBack* finished, bool inpythonenv,
				const char* workdir )
{
    auto* cmdtask = new CommandTask( mc, lt, inpythonenv, workdir );
    execute( cmdtask, finished );
}


void CommandLaunchMgr::execute( const OS::MachineCommand& mc,
				bool readstdoutput, bool readstderror,
				CallBack* finished,
				bool inpythonenv, const char* workdir )
{
    auto* cmdtask = new CommandTask( mc, readstdoutput, readstderror,
				     inpythonenv, workdir );
    execute( cmdtask, finished );
}


void CommandLaunchMgr::execute( const OS::MachineCommand& mc,
				const OS::CommandExecPars& xpars,
				CallBack* finished,
				bool inpythonenv )
{
    auto* cmdtask = new CommandTask( mc, xpars, inpythonenv );
    execute( cmdtask, finished );
}


const CommandTask* CommandLaunchMgr::getCommandTask( CallBacker* cb ) const
{
    const Work* work = WorkManager::twm().getWork( cb );
    if ( !work )
	return nullptr;

    mDynamicCastGet( const CommandTask*, cmdtask, work->getTask() );
    return cmdtask;
}


void CommandLaunchMgr::execute( CommandTask* ct, CallBack* finished )
{
    WorkManager::twm().addWork( Work(*ct, true), finished, twm_queueid_, false,
				false, true );
}
