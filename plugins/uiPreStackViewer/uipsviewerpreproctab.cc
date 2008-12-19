/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Yuancheng Liu
 * DATE     : May 2008
-*/

static const char* rcsID = "$Id: uipsviewerpreproctab.cc,v 1.5 2008-12-19 21:58:00 cvsyuancheng Exp $";

#include "uipsviewerpreproctab.h"

#include "prestackprocessor.h"
#include "uibutton.h"
#include "uimsg.h"
#include "uiprestackprocessor.h"
#include "uipsviewermanager.h"
#include "visprestackviewer.h"

namespace PreStackView
{


uiViewerPreProcTab::uiViewerPreProcTab( uiParent* p, 
	PreStackView::Viewer& vwr, uiViewerMgr& mgr, 
	PreStack::ProcessManager& preprocmgr )
    : uiDlgGroup( p, "Preprocessing" )
    , vwr_( vwr )
    , preprocmgr_( &preprocmgr )
    , mgr_( mgr )
    , applyall_( false )
{
    uipreprocmgr_ = new PreStack::uiProcessorManager( this, preprocmgr );
    applybut_ = new uiPushButton( this, "Apply", true );
    applybut_->attach( centeredBelow, uipreprocmgr_ );
    applybut_->activated.notify( mCB(this,uiViewerPreProcTab,applyButPushedCB));
    uipreprocmgr_->change.notify( 
	    mCB(this,uiViewerPreProcTab,processorChangeCB) );
    applybut_->setSensitive( false );
}


uiViewerPreProcTab::~uiViewerPreProcTab()
{
    uipreprocmgr_->change.remove(
	    mCB(this,uiViewerPreProcTab,processorChangeCB) );
    applybut_->activated.remove( 
	    mCB(this,uiViewerPreProcTab,applyButPushedCB) );
    delete uipreprocmgr_;
}


void uiViewerPreProcTab::processorChangeCB( CallBacker* )
{
    if ( !preprocmgr_->nrProcessors() )
	applybut_->setSensitive( false );
    else
	applybut_->setSensitive( true );
}


bool uiViewerPreProcTab::acceptOK()
{
    if ( !applybut_->sensitive() )
	return true;

    if ( !preprocmgr_->nrProcessors() )
	return true;

    for ( int idx=0; idx<mgr_.getViewers().size(); idx++ )
    {
	PreStackView::Viewer* vwr = mgr_.getViewers()[idx];
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


void uiViewerPreProcTab::applyButPushedCB( CallBacker* cb )
{
    applybut_->setSensitive( false );
    if ( !preprocmgr_->nrProcessors() )
	return;

    for ( int idx=0; idx<mgr_.getViewers().size(); idx++ )
    {
	PreStackView::Viewer* vwr = mgr_.getViewers()[idx];
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
