#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "callback.h"
#include "commondefs.h"
#include "oscommand.h"
#include "task.h"

namespace Threads
{
/*!\brief CommandTask class can be used in a CommandLaunchMgr callback function
to get information written to stdout or stderr streams by the MachineCommand

 */

mExpClass(Basic) CommandTask : public Task
{
public:
				CommandTask(const OS::MachineCommand&,
					    OS::LaunchType lt=OS::Wait4Finish,
					    bool inpythonenv=false,
					    const char* workdir=nullptr);
				CommandTask(const OS::MachineCommand&,
					    bool readstdoutput,
					    bool readstderror,
					    bool inpythonenv=false,
					    const char* workdir=nullptr);
				CommandTask(const OS::MachineCommand&,
					    const OS::CommandExecPars&,
					    bool inpythonenv=false);
    virtual			~CommandTask();
				mOD_DisableCopy(CommandTask);

    bool			execute() override;
    bool			getResult() const	{ return result_; }
    const OS::MachineCommand&	getMachineCommand() const { return machcmd_; }

    BufferString		getStdOutput() const;
    BufferString		getStdError() const;

protected:
    OS::MachineCommand		machcmd_;
    OS::CommandExecPars		execpars_;
    BufferString*		stdoutput_ = nullptr;
    BufferString*		stderror_ = nullptr;
    bool			inpythonenv_ = false;
    bool			result_ = false;

};

/*!\brief Starts MachineCommand's using a multi-threaded Threads::WorkManager
queue.

*/

mExpClass(Basic) CommandLaunchMgr
{
public:
    static CommandLaunchMgr&	getMgr();

    void			execute(const OS::MachineCommand&,
					OS::LaunchType lt=OS::Wait4Finish,
					CallBack* finished=nullptr,
					bool inpythonenv=false,
					const char* workdir=nullptr);
    void			execute(const OS::MachineCommand&,
					bool readstdoutput, bool readstderror,
					CallBack* finished=nullptr,
					bool inpythonenv=false,
					const char* workdir=nullptr);
    void			execute(const OS::MachineCommand&,
					const OS::CommandExecPars&,
					CallBack* finished=nullptr,
					bool inpythonenv=false);

    int				wmQueueID() const	{ return twm_queueid_; }
    const CommandTask*		getCommandTask(CallBacker*) const;

protected:
				CommandLaunchMgr();
				~CommandLaunchMgr();

    void			execute(CommandTask*, CallBack* finished);

    int				twm_queueid_;
};

} // namespace Threads
