#ifndef uivarwizard_h
#define uivarwizard_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2010
 RCS:           $Id: uivarwizard.h,v 1.6 2012-08-07 04:00:22 cvsmahant Exp $
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "iopar.h"
class uiParent;
class uiVarWizardDlg;


/*!\brief 'Server' for flexible wizards.

  This server launches subclasses of uiVarWizardDlg.
 
 */

mClass(uiTools) uiVarWizard : public CallBacker
{
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
    if ( !dlg) return; \
    if ( !dlg->uiResult() ) \
    { \
	const bool doleave = mustLeave(dlg); \
	dlg = 0; \
	mSetVWState( doleave ? cCancelled() : ((int)backstate) ); \
    }



#endif

