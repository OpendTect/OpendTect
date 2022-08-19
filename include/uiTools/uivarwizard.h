#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "iopar.h"
#include "uistring.h"
class uiParent;
class uiVarWizardDlg;


/*!\brief 'Server' for flexible wizards.

  This server launches subclasses of uiVarWizardDlg.

 */

mExpClass(uiTools) uiVarWizard : public CallBacker
{ mODTextTranslationClass(uiVarWizard);
public:

			uiVarWizard(uiParent*);
    virtual		~uiVarWizard()		{}

    IOPar&		pars()			{ return pars_; }
    const IOPar&	pars() const		{ return pars_; }
    int			state() const		{ return state_; }

    Notifier<uiVarWizard> processEnded;

    static int		cCancelled()		{ return 0; }
    static int		cFinished()		{ return 1; }
    static int		cWait4Dialog()		{ return 2; }

    virtual void	raiseCurrent()		= 0;

protected:

    uiParent*		parent_;
    IOPar		pars_;
    int			state_;
    int			afterfinishedstate_; //! default -1 meaning: close down

    virtual void	doPart()		= 0;
    virtual void	closeDown();

    void		nextAction();	//!< Call in constructor of subclass
    static bool		mustLeave(uiVarWizardDlg*);
};

#define mSetVWState(st) \
	{ state_ = st; nextAction(); return; }

#define mLaunchVWDialogOnly(dlg,clss,fn) \
	dlg->windowClosed.notify( mCB(this,clss,fn) ); \
	dlg->setDeleteOnClose( true ); dlg->go()

#define mLaunchVWDialog(dlg,clss,fn) \
	mLaunchVWDialogOnly(dlg,clss,fn); \
	mSetVWState( cWait4Dialog() )

#define mHandleVWCancel(dlg,backstate) \
    if ( !dlg ) return; \
    if ( !dlg->uiResult() ) \
    { \
	const bool doleave = mustLeave(dlg); \
	dlg = 0; \
	mSetVWState( doleave ? cCancelled() : ((int)backstate) ); \
    }
