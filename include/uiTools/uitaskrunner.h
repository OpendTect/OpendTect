#ifndef uitaskrunner_h
#define uitaskrunner_h

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
#include "thread.h"
#include "task.h"

class uiProgressBar;
class uiLabel;
class Timer;

mExpClass(uiTools) uiTaskRunner : public uiDialog
		   , public TaskRunner
{ 	
public:
			uiTaskRunner(uiParent*,bool dispmsgonerr=true);
			~uiTaskRunner();

    bool		execute(Task& t);
    const char*		lastMsg() const		{ return prevmessage_.buf(); }
    int			getState() const	{ return state_; }
    void		displayMsgOnError(bool yn)	{ dispmsgonerr_ = yn; }

protected:

    uiProgressBar*	progbar_;
    uiLabel*		proglbl_;

    Timer&		tim_;
    BufferString	execnm_;
    int			symbidx_;

    Task*		task_;
    Threads::Mutex	dispinfomutex_;
    int			prevtotalnr_;
    int			prevnrdone_;
    int			prevpercentage_;
    BufferString	prevmessage_;
    BufferString	prevnrdonetext_;
    bool		dispmsgonerr_;

    Threads::Mutex&	statemutex_;	
    int			state_; //-1 finished in error
    				// 0 finished without error
				// 1 running
    Threads::Thread*	thread_;
    Threads::Mutex	uitaskrunnerthreadmutex_;
    void		doWork(CallBacker*);	//!< Method with work thread

    BufferString	finalizeTask();
    void		updateFields();
    void		onFinalise(CallBacker*);
    void		timerTick(CallBacker*);
    virtual bool        acceptOK(CallBacker*);

    virtual bool        rejectOK(CallBacker*);

    void		init();
};

#endif

