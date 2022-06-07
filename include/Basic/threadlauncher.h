 #pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		May 2022
________________________________________________________________________

-*/

#include "basicmod.h"
#include "callback.h"
#include "commondefs.h"
#include "oscommand.h"


/*!\brief Starts a MachineCommand in a separate thread allowing execution to
 continue with minimal delay.

The command output and result can be accessed by attaching a callback to
the finished signal.

 */

namespace Threads
{

class Thread;

mExpClass(Basic) ThreadLauncher : public CallBacker
{
public:
			ThreadLauncher();
			~ThreadLauncher();

			mOD_DisableCopy(ThreadLauncher);

    void		execute(const OS::MachineCommand&,
				OS::LaunchType lt=OS::Wait4Finish,
				bool inpythonenv=false,
				const char* workdir=nullptr);

    void		execute(const OS::MachineCommand&,
				bool readstdoutput, bool readstderror,
				bool inpythonenv=false,
				const char* workdir=nullptr);
    void		execute(const OS::MachineCommand&,
				const OS::CommandExecPars&,
				bool inpythonenv=false);

    BufferString	getStdOutput() const;
    BufferString	getStdError() const;


    Notifier<ThreadLauncher>	finished;
protected:
    OS::MachineCommand	machcmd_;
    OS::CommandExecPars execpars_;
    BufferString*	stdoutput_ = nullptr;
    BufferString*	stderror_ = nullptr;
    Threads::Thread*	thread_ = nullptr;
    bool		inpythonenv_ = false;

    void		startCB(CallBacker*);
};

} //namespace
