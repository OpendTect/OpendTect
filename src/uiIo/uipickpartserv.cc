/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uipickpartserv.cc,v 1.4 2002-03-29 17:26:42 nanne Exp $
________________________________________________________________________

-*/

#include "uipickpartserv.h"
#include "uifetchpicks.h"
#include "uistorepicks.h"
#include "uimsg.h"
#include "pickset.h"
#include "picksettr.h"

const int uiPickPartServer::evGetAvailableSets = 0;
const int uiPickPartServer::evFetchPicks = 1;


uiPickPartServer::uiPickPartServer( uiApplService& a )
	: uiApplPartServer(a)
    	, psg(*new PickSetGroup)
    	, avsets("Select sets to be stored")
{
}


uiPickPartServer::~uiPickPartServer()
{
    delete &psg;
    avsets.deepErase();
}


bool uiPickPartServer::fetchPickSets()
{
    psg.clear();
    uiFetchPicks dlg( appserv().parent(), psgid );
    if ( !dlg.go() ) return false;

    if ( !dlg.nrSets() )
	{ psg.setName(dlg.getName()); return true; }

    BufferString bs;
    if ( !PickSetGroupTranslator::retrieve(psg,dlg.ioobj(),bs,
					   dlg.selectedSets()) )
	{ uiMSG().error( bs ); return false; }

    return true;
}


bool uiPickPartServer::storePickSets()
{
    avsets.erase();
    sendEvent( evGetAvailableSets );
    if ( !avsets.size() )
	{ uiMSG().error( "No pick sets defined yet" ); return false; }

    uiStorePicks dlg( appserv().parent(), psgid, avsets );
    if ( !dlg.go() ) return false;

    selsets.erase();
    for ( int idx=0; idx<avsets.size(); idx++ )
	selsets += dlg.selectedSets()[idx] ? char(1) : char(0);

    psg.clear();
    sendEvent( evFetchPicks );
    if ( !psg.nrSets() ) return false;

    BufferString bs;
    if ( !PickSetGroupTranslator::store(psg,dlg.ioobj(),bs,0,dlg.merge()) )
	{ uiMSG().error( bs ); return false; }

    return true;
}
