#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          December 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "threadlock.h"
#include "task.h"

class uiProgressBar;
class uiLabel;
class Timer;
namespace Threads { class Thread; }


mExpClass(uiTools) uiTaskRunner : public uiDialog
		   , public TaskRunner
{  mODTextTranslationClass(uiTaskRunner);
public:
			uiTaskRunner(uiParent*,bool dispmsgonerr=true);
			~uiTaskRunner();

    bool		execute(Task& t);
    uiString		lastMsg() const 	{ return prevmessage_; }
    int			getState() const	{ return state_; }
    void		displayMsgOnError(bool yn)	{ dispmsgonerr_ = yn; }

protected:

    uiProgressBar*	progbar_;
    uiLabel*		proglbl_;

    Timer&		tim_;
    BufferString	execnm_;
    int			symbidx_;

    Task*		task_;
    Threads::Lock	dispinfolock_;
    int			prevtotalnr_;
    int			prevnrdone_;
    int			prevpercentage_;
    uiString		prevmessage_;
    uiString		prevnrdonetext_;
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
    void		onFinalise(CallBacker*);
    void		timerTick(CallBacker*);
    virtual bool        acceptOK(CallBacker*);

    virtual bool        rejectOK(CallBacker*);

    void		init();
};

