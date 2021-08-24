/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Yuancheng Liu
 * DATE     : May 2008
-*/


#include "uipsviewerpreproctab.h"

#include "prestackprocessor.h"
#include "uibutton.h"
#include "uimsg.h"
#include "uiprestackprocessor.h"
#include "uipsviewermanager.h"
#include "visprestackdisplay.h"

namespace PreStackView
{


uiViewer3DPreProcTab::uiViewer3DPreProcTab( uiParent* p,
	visSurvey::PreStackDisplay& vwr, uiViewer3DMgr& mgr )
    : uiDlgGroup( p, tr("Preprocessing") )
    , vwr_( vwr )
    , mgr_( mgr )
    , applyall_( false )
{
    uipreprocmgr_ = new PreStack::uiProcessorManager( this, vwr.procMgr() );
    uipreprocmgr_->change.notify(
	    mCB(this,uiViewer3DPreProcTab,processorChangeCB) );

    applybut_ = uiButton::getStd( this, OD::Apply,
	   mCB(this,uiViewer3DPreProcTab,applyButPushedCB), true );
    applybut_->attach( centeredBelow, uipreprocmgr_ );

    applybut_->setSensitive( false );
}


uiViewer3DPreProcTab::~uiViewer3DPreProcTab()
{
    detachAllNotifiers();
    delete uipreprocmgr_;
}


void uiViewer3DPreProcTab::processorChangeCB( CallBacker* )
{
    applybut_->setSensitive( true );
}


bool uiViewer3DPreProcTab::acceptOK()
{
    return apply();
}


void uiViewer3DPreProcTab::applyButPushedCB( CallBacker* cb )
{
    apply();
}


bool uiViewer3DPreProcTab::apply()
{
    applybut_->setSensitive( false );

    for ( int idx=0; idx<mgr_.get3DViewers().size(); idx++ )
    {
	visSurvey::PreStackDisplay* vwr = mgr_.get3DViewers()[idx];
	const bool isownvwr = vwr == &vwr_;
	if ( !applyall_ && !isownvwr )
	    continue;

	if ( !isownvwr )
	{
	    IOPar curpreprocpar;
	    vwr_.procMgr().fillPar( curpreprocpar );
	    vwr->procMgr().usePar( curpreprocpar );
	}

	if ( !vwr->updateDisplay() )
	{
	    uiMSG().message( tr("Preprocessing failed!") );
	    return false;
	}
    }

    return true;
}


}; //namespace
