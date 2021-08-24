/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2010
________________________________________________________________________

-*/

#include "uivarwizard.h"
#include "uivarwizarddlg.h"
#include "objdisposer.h"


#define mSetState(st) { state_ = st; nextAction(); return; }

uiVarWizard::uiVarWizard( uiParent* p )
    : state_(-1)
    , afterfinishedstate_(-1)
    , parent_(p)
    , processEnded(this)
{
}

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
	    { closeDown(); return; }
	state_ = afterfinishedstate_;
    }

    if ( state_ != cFinished() && state_ != cWait4Dialog() )
	doPart();
}


bool uiVarWizard::mustLeave( uiVarWizardDlg* dlg )
{
    return dlg && dlg->leave_;
}


uiVarWizardDlg::uiVarWizardDlg( uiParent* p, const uiDialog::Setup& su,
			IOPar& pars, uiVarWizardDlg::Position pos,
			bool revbuttons )
    : uiDialog(p,uiDialog::Setup(su).modal(false).okcancelrev(revbuttons))
    , pars_(pars)
    , pos_(pos)
    , leave_(false)
{
    if ( pos_ == End )
	setOkText( tr("Finish") );
    else if ( pos_ == DoWork )
	setOkText( uiStrings::sGo() );
    else
	setOkText( uiStrings::sNext() );

    if ( pos_ == Start )
	setCancelText( uiStrings::sCancel() );
    else
	setCancelText( tr("< Back") );
}


uiVarWizardDlg::~uiVarWizardDlg()
{
}

bool uiVarWizardDlg::rejectOK( CallBacker* )
{
    leave_ = !cancelpushed_;
    return true;
}
