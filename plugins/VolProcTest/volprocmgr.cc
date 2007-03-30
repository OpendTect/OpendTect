/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Y.C. Liu
 * DATE     : March 2007
-*/

static const char* rcsID = "$Id: volprocmgr.cc,v 1.1 2007-03-30 21:00:56 cvsyuancheng Exp $";


#include "volprocmgr.h"

#include "uitoolbar.h"
#include "uivolumeprocessing.h"
#include "volumeprocessing.h"

using namespace VolProc;

Manager& Manager::get( uiParent* parent )
{
    static Manager man( parent );
    return man;
}


Manager::Manager( uiParent* p )
    : toolbar_( new uiToolBar(p, "Volume Processing" ) )
    , chain_( *new ProcessingChain )
{
    chain_.ref();
    showsetupidx_ = toolbar_->addButton( "edvelmodrendpars.png",
			mCB( this, Manager, buttonClickCB  ),
			"Edit volume processing setup", false );
}


Manager::~Manager()
{
    chain_.unRef();
}


void Manager::buttonClickCB( CallBacker* )
{
    uiProcessingChain dlg( toolbar_, chain_ );
    dlg.go();
}
