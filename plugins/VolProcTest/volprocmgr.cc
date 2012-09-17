/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : March 2007
-*/

static const char* rcsID = "$Id: volprocmgr.cc,v 1.2 2009/07/22 16:01:27 cvsbert Exp $";


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
