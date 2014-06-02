/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Yuancheng Liu
 * DATE     : May 2008
-*/

static const char* rcsID mUsedVar = "$Id$";

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
    : uiDlgGroup( p, "Preprocessing" )
    , vwr_( vwr )
    , mgr_( mgr )
    , applyall_( false )
{
    uipreprocmgr_ = new PreStack::uiProcessorManager( this, vwr.procMgr() );
    applybut_ = new uiPushButton( this, "Apply", true );
    applybut_->attach( centeredBelow, uipreprocmgr_ );
    applybut_->activated.notify(
	    mCB(this,uiViewer3DPreProcTab,applyButPushedCB));
    uipreprocmgr_->change.notify( 
	    mCB(this,uiViewer3DPreProcTab,processorChangeCB) );
    applybut_->setSensitive( false );
}


uiViewer3DPreProcTab::~uiViewer3DPreProcTab()
{
    uipreprocmgr_->change.remove(
	    mCB(this,uiViewer3DPreProcTab,processorChangeCB) );
    applybut_->activated.remove( 
	    mCB(this,uiViewer3DPreProcTab,applyButPushedCB) );
    delete uipreprocmgr_;
}


void uiViewer3DPreProcTab::processorChangeCB( CallBacker* )
{
    applybut_->setSensitive( true );
}


bool uiViewer3DPreProcTab::acceptOK()
{
    return applyButPushedCB( 0 );
}


bool uiViewer3DPreProcTab::applyButPushedCB( CallBacker* cb )
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
	    uiMSG().message( "Preprocessing failed!" );
	    return false;
	}
    }

    return true;
}


}; //namespace
