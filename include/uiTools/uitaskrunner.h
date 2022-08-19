#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "threadlock.h"
#include "task.h"

class uiProgressBar;
class Timer;
namespace Threads { class Thread; }


mExpClass(uiTools) uiTaskRunner : public uiDialog
		   , public TaskRunner
{ mODTextTranslationClass(uiTaskRunner);
public:
			uiTaskRunner(uiParent*,bool dispmsgonerr=true);
			~uiTaskRunner();

    bool		execute(Task& t) override;
    virtual void	emitErrorMessage(const uiString&,bool) const;

    uiString		lastMsg() const	{ return prevmessage_; }
    uiRetVal		errorWithDetails() const	{ return errdetails_; }
    int			getState() const	{ return state_; }
    void		displayMsgOnError(bool yn)	{ dispmsgonerr_ = yn; }

protected:

    uiProgressBar*	progbar_;

    Timer&		tim_;
    int         prevtime_ = 0;
    BufferString	execnm_;

    Task*		task_;
    Threads::Lock	dispinfolock_;
    int			prevtotalnr_;
    int			prevnrdone_;
    int			prevpercentage_;
    uiString		prevmessage_;
    uiString		prevnrdonetext_;
    uiRetVal		errdetails_;
    bool		dispmsgonerr_;

    Threads::Lock	statelock_;
    int			state_; //-1 finished in error
				// 0 finished without error
				// 1 running
    Threads::Thread*	thread_;
    Threads::Lock	uitaskrunnerthreadlock_;
    void		doWork(CallBacker*);	//!< Method with work thread

    uiString		finalizeTask();
    void		updateFields();
    void		onFinalize(CallBacker*);
    void		timerTick(CallBacker*);
    bool		acceptOK(CallBacker*) override;

    bool		rejectOK(CallBacker*) override;

    void		init();

};
