/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Yuancheng Liu
 * DATE     : May 2008
-*/

static const char* rcsID = "$Id: uipsviewerpreproctab.cc,v 1.8 2009/07/22 16:01:28 cvsbert Exp $";

#include "uipsviewerpreproctab.h"

#include "prestackprocessor.h"
#include "uibutton.h"
#include "uimsg.h"
#include "uiprestackprocessor.h"
#include "uipsviewermanager.h"
#include "visprestackviewer.h"

namespace PreStackView
{


uiViewer3DPreProcTab::uiViewer3DPreProcTab( uiParent* p, 
	PreStackView::Viewer3D& vwr, uiViewer3DMgr& mgr, 
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
    applybut_->activated.notify( mCB(this,uiViewer3DPreProcTab,applyButPushedCB));
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
    if ( !preprocmgr_->nrProcessors() )
	applybut_->setSensitive( false );
    else
	applybut_->setSensitive( true );
}


bool uiViewer3DPreProcTab::acceptOK()
{
    if ( !applybut_->sensitive() )
	return true;

    return applyButPushedCB( 0 );
}


bool uiViewer3DPreProcTab::applyButPushedCB( CallBacker* cb )
{
    applybut_->setSensitive( false );
    if ( !preprocmgr_->nrProcessors() )
	return true;

    for ( int idx=0; idx<mgr_.get3DViewers().size(); idx++ )
    {
	PreStackView::Viewer3D* vwr = mgr_.get3DViewers()[idx];
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


}; //namespace
