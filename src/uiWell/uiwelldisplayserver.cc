/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwelldisplayserver.h"

#include "dbkey.h"
#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "uimsg.h"
#include "uiwelllogdisplay.h"
#include "uiwelllogtoolsgrp.h"


mExternCPP(uiWell) uiWellDisplayServer& GetWellDisplayServer( bool set,
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


uiMainWin* uiODWellDisplayServer::createMultiWellDisplay( uiParent* p,
							 const DBKeySet& wells,
						 const BufferStringSet& loglst )
{
    BufferStringSet lognms;
    lognms.unCat( loglst.get(0), "," );
    const int nrchosen = wells.size()*lognms.size();

    const Well::Log* wl1 = nullptr;
    const Well::Log* wl2 = nullptr;
    BufferStringSet wnms;
    const Well::LoadReqs lreqs( lognms );
    RefMan<Well::Data> wd = Well::MGR().get( wells[0], lreqs );
    if ( wd )
    {
	wnms.add( wd->name() );
	wl1 = wd->logs().getLog( lognms.get(0).buf() );
	if ( lognms.size() >= 2 )
	    wl2 = wd->logs().getLog( lognms.get(1).buf() );
    }

    if ( wells.size() > 1 )
    {
	RefMan<Well::Data> wd2 = Well::MGR().get( wells[1], lreqs );
	if ( wd2 )
	{
	    wnms.add( wd2->name() );
	    wl2 = wd2->logs().getLog( lognms.get(0).buf() );
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


uiWellLogToolWinGrp* uiODWellDisplayServer::createWellLogToolGrp( uiParent* p,
			    const ObjectSet<WellLogToolData>& logs )
{
    return new uiODWellLogToolWinGrp( p, logs );
}


// uiLogDisplayGrp
uiLogDisplayGrp::uiLogDisplayGrp( uiParent* p, const char* nm )
    : uiGroup(p,nm)
{}


uiLogDisplayGrp::~uiLogDisplayGrp()
{}
