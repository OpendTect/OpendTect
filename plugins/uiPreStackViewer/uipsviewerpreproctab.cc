/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Yuancheng Liu
 * DATE     : May 2008
-*/

static const char* rcsID = "$Id: uipsviewerpreproctab.cc,v 1.4 2008-12-18 15:21:06 cvsyuancheng Exp $";

#include "uipsviewerpreproctab.h"

#include "prestackprocessor.h"
#include "uibutton.h"
#include "uimsg.h"
#include "uiprestackprocessor.h"
#include "uipsviewermanager.h"
#include "visprestackviewer.h"

namespace PreStackView
{


uiPSViewerPreProcTab::uiPSViewerPreProcTab( uiParent* p, PreStackViewer& vwr,
	uiPSViewerMgr& mgr, PreStack::ProcessManager& preprocmgr )
    : uiDlgGroup( p, "Preprocessing" )
    , vwr_( vwr )
    , preprocmgr_( &preprocmgr )
    , mgr_( mgr )
    , applyall_( false )
{
    uipreprocmgr_ = new PreStack::uiProcessorManager( this, preprocmgr );
    applybut_ = new uiPushButton( this, "Apply", true );
    applybut_->attach( centeredBelow, uipreprocmgr_ );
    applybut_->activated.notify( 
	    mCB(this,uiPSViewerPreProcTab,applyButPushedCB) );
    uipreprocmgr_->change.notify( 
	    mCB(this,uiPSViewerPreProcTab,processorChangeCB) );
    applybut_->setSensitive( false );
}


uiPSViewerPreProcTab::~uiPSViewerPreProcTab()
{
    uipreprocmgr_->change.remove(
	    mCB(this,uiPSViewerPreProcTab,processorChangeCB) );
    applybut_->activated.remove( 
	    mCB(this,uiPSViewerPreProcTab,applyButPushedCB) );
    delete uipreprocmgr_;
}


void uiPSViewerPreProcTab::processorChangeCB( CallBacker* )
{
    if ( !preprocmgr_->nrProcessors() )
	applybut_->setSensitive( false );
    else
	applybut_->setSensitive( true );
}


bool uiPSViewerPreProcTab::acceptOK()
{
    if ( !applybut_->sensitive() )
	return true;

    if ( !preprocmgr_->nrProcessors() )
	return true;

    for ( int idx=0; idx<mgr_.getViewers().size(); idx++ )
    {
	PreStackViewer* vwr = mgr_.getViewers()[idx];
	if ( !applyall_ && vwr != &vwr_ )
	    continue;

	if ( !vwr->setPreProcessor( preprocmgr_ ) )
	{
	    uiMSG().message( "Preprocessing failed!" );
	    return false;
	}
    }

    return true;
}


void uiPSViewerPreProcTab::applyButPushedCB( CallBacker* cb )
{
    applybut_->setSensitive( false );
    if ( !preprocmgr_->nrProcessors() )
	return;

    for ( int idx=0; idx<mgr_.getViewers().size(); idx++ )
    {
	PreStackViewer* vwr = mgr_.getViewers()[idx];
	if ( !applyall_ && vwr != &vwr_ )
	    continue;

	if ( !vwr->setPreProcessor( preprocmgr_ ) )
	{
	    uiMSG().message( "Preprocessing failed!" );
	    return;
	}
    }
}


}; //namespace
