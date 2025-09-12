/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivarwizard.h"
#include "uivarwizarddlg.h"

#include "uibutton.h"
#include "objdisposer.h"


#define mSetState(st) { state_ = st; nextAction(); return; }

uiVarWizard::uiVarWizard( uiParent* p )
    : state_(-1)
    , afterfinishedstate_(-1)
    , parent_(p)
    , processEnded(this)
{}


uiVarWizard::~uiVarWizard()
{}


void uiVarWizard::closeDown()
{
    processEnded.trigger();
    OBJDISP()->go( this );
}


void uiVarWizard::nextAction()
{
    if ( state_ <= cFinished() )
    {
	if ( state_ == cCancelled() || afterfinishedstate_ < 0 )
	{
	    closeDown();
	    return;
	}
	state_ = afterfinishedstate_;
    }

    if ( state_ != cFinished() && state_ != cWaitForDialog() )
	doPart();
}


bool uiVarWizard::mustLeave( uiVarWizardDlg* dlg )
{
    return dlg && dlg->leave_;
}


uiVarWizardDlg::uiVarWizardDlg( uiParent* p, const uiDialog::Setup& su,
				IOPar& pars, uiVarWizardDlg::Position pos,
				bool revbuttons )
    : uiDialog(p,Setup(su).modal(false).okcancelrev(revbuttons))
    , pars_(pars)
    , pos_(pos)
    , leave_(false)
{
    setOkCancelText( uiStrings::sNext(), uiStrings::sBack() );
    mAttachCB( postFinalize(), uiVarWizardDlg::doFinalizeCB );
}


uiVarWizardDlg::~uiVarWizardDlg()
{}


void uiVarWizardDlg::doFinalizeCB( CallBacker* )
{
    switch ( pos_ )
    {
	case Start:
	    setCancelText( uiStrings::sCancel() );
	    button(OK)->setIcon( "rightarrow" );
	    break;
	case Middle:
	    button(CANCEL)->setIcon( "leftarrow" );
	    button(OK)->setIcon( "rightarrow" );
	    break;
	case End://when a process finishes, there is no cancel button, right?
	    setOkText( uiStrings::sFinish() );
	    break;
	case DoWork:
	    button(CANCEL)->setIcon( "leftarrow" );
	    setOkText( uiStrings::sGo() );
	    break;
	default:
	    pErrMsg("Invalid Status");
	    break;
    }
}


bool uiVarWizardDlg::rejectOK( CallBacker* )
{
    leave_ = !cancelpushed_;
    return true;
}
