/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Yuancheng Liu
 * DATE     : May 2008
-*/

static const char* rcsID = "$Id: uipsviewerpreproctab.cc,v 1.1 2008-05-15 18:48:39 cvsyuancheng Exp $";

#include "uipsviewerpreproctab.h"

#include "visprestackviewer.h"
#include "prestackprocessor.h"
#include "uiprestackprocessor.h"
#include "uipsviewermanager.h"
#include "uimsg.h"

namespace PreStackView
{


uiPSViewerPreProcTab::uiPSViewerPreProcTab( uiParent* p, PreStackViewer& vwr,
       					    uiPSViewerMgr& mgr )
    : uiDlgGroup( p, "Preprocessing" )
    , vwr_( vwr )
    , preprocmgr_( new PreStack::ProcessManager )
    , mgr_( mgr )
    , applyall_( false )
{
    preprocmgr_->usePar( vwr.getPreProcessingPars() );
    uipreprocmgr_ = new PreStack::uiProcessorManager( this, *preprocmgr_ );
}


uiPSViewerPreProcTab::~uiPSViewerPreProcTab()
{
    delete uipreprocmgr_;
    delete preprocmgr_;
}


bool uiPSViewerPreProcTab::acceptOK()
{
    if ( !preprocmgr_->nrProcessors() )
	return true;

    for ( int idx=0; idx<mgr_.getViewers().size(); idx++ )
    {
	PreStackViewer* vwr = mgr_.getViewers()[idx];
	if ( !applyall_ && vwr != &vwr_ )
	    continue;

	if ( !vwr->doPreProcessing( preprocmgr_ ) )
	{
	    uiMSG().message( "Preprocessing failed!" );
	    return false;
	}
    }

    return true;
}


}; //namespace
