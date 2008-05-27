/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Yuancheng Liu
 * DATE     : May 2008
-*/

static const char* rcsID = "$Id: uipsviewerpreproctab.cc,v 1.2 2008-05-27 22:53:41 cvsyuancheng Exp $";

#include "uipsviewerpreproctab.h"

#include "visprestackviewer.h"
#include "prestackprocessor.h"
#include "uiprestackprocessor.h"
#include "uipsviewermanager.h"
#include "uimsg.h"

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
}


uiPSViewerPreProcTab::~uiPSViewerPreProcTab()
{
    delete uipreprocmgr_;
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

	if ( !vwr->setPreProcessor( preprocmgr_ ) )
	{
	    uiMSG().message( "Preprocessing failed!" );
	    return false;
	}
    }

    return true;
}


}; //namespace
