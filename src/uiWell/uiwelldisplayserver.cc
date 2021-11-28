/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		Nov 2021
________________________________________________________________________

-*/

#include "uiwelldisplayserver.h"

#include "dbkey.h"
#include "welllog.h"
#include "wellman.h"
#include "uimsg.h"
#include "uiwelllogdisplay.h"


mExternC(uiWell) uiWellDisplayServer& GetWellDisplayServer( bool set,
						    uiWellDisplayServer* wds )
{
    static PtrMan<uiWellDisplayServer> welldispsvr_ = new uiODWellDisplayServer;
    if ( set && wds )
	welldispsvr_ = wds;
    else if ( set && !wds && !welldispsvr_->isBasic() )
	welldispsvr_ = new uiODWellDisplayServer;

    return *welldispsvr_.ptr();
}


uiWellDisplayServer::uiWellDisplayServer()
{}


uiWellDisplayServer::~uiWellDisplayServer()
{}


uiODWellDisplayServer::uiODWellDisplayServer()
    : uiWellDisplayServer()
{}


uiODWellDisplayServer::~uiODWellDisplayServer()
{}


uiDialog* uiODWellDisplayServer::createMultiWellDisplay( uiParent* p,
							 const DBKeySet& wells,
						 const BufferStringSet& loglst )
{
    BufferStringSet lognms;
    lognms.unCat( loglst.get(0), "," );
    const int nrchosen = wells.size()*lognms.size();

    const Well::Log* wl1 = nullptr;
    const Well::Log* wl2 = nullptr;
    BufferStringSet wnms;
    RefMan<Well::Data> wd = Well::MGR().get( wells[0],
					     Well::LoadReqs(Well::LogInfos) );
    if ( wd )
    {
	wnms.add( wd->name() );
	wl1 = wd->getLog( lognms.get(0) );

	if ( lognms.size() >= 2 )
	{
	    wl2 = wd->getLog( lognms.get(1) );
	}
    }

    if ( wells.size() > 1 )
    {
	RefMan<Well::Data> wd2 = Well::MGR().get( wells[1],
					     Well::LoadReqs(Well::LogInfos) );
	if ( wd2 )
	{
	    wnms.add( wd2->name() );
	    wl2 = wd2->getLog( lognms.get(0) );
	}
    }

    if ( !wl1 || (nrchosen>=2 && !wl2) )
    {
	 uiMSG().error( uiStrings::phrCannotRead(uiStrings::sWellLog()) );
	 return nullptr;
    }

    return uiWellLogDispDlg::popupNonModal( p, wl1, wl2, wnms.get(0),
				     wnms.size() > 1 ? wnms.get(1) : nullptr );
}
