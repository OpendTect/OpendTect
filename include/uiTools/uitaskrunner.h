#ifndef uitaskrunner_h
#define uitaskrunner_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          December 2007
 RCS:           $Id: uitaskrunner.h,v 1.1 2007-12-13 19:26:39 cvskris Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "thread.h"
#include "task.h"

class uiProgressBar;
class Timer;

class uiTaskRunner : public uiDialog
		   , public TaskRunner
{ 	
public:
			uiTaskRunner(uiParent*);
			~uiTaskRunner();

    bool		execute(Task& t);

protected:

    uiProgressBar*	progbar_;

    Timer&		tim_;
    BufferString	execnm_;

    Task*		task_;
    Threads::Mutex	dispinfomutex_;
    int			prevtotalnr_;
    int			prevnrdone_;
    int			prevpercentage_;
    BufferString	prevmessage_;
    BufferString	prevnrdonetext_;

    Threads::Mutex&	statemutex_;	
    int			state_; //-1 finished in error
    				// 0 finished without error
				// 1 running
    Threads::Thread*	thread_;
    void		doWork(CallBacker*);	//!< Method with work thread

    void		updateFields();
    void		onFinalise(CallBacker*);
    void		timerTick(CallBacker*);
    //virtual void	execFinished();	// Called when work is done (or aborted)
    virtual bool        acceptOK(CallBacker*);

    virtual void	doFinalise()		{}
    virtual bool        rejectOK(CallBacker*);

    void		init();
};

#endif
