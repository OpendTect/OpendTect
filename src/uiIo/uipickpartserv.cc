/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uipickpartserv.cc,v 1.1 2002-02-08 22:01:28 bert Exp $
________________________________________________________________________

-*/

#include "uipickpartserv.h"
#include "uifetchpicks.h"
#include "uistorepicks.h"
#include "uimsg.h"
#include "pickset.h"
#include "picksettr.h"

const int uiPickPartServ::evGetAvailableSets = 0;
const int uiPickPartServ::evFetchPicks = 1;


uiPickPartServ::uiPickPartServ( uiApplService& a )
	: uiApplPartServer(a)
    	, psg(*new PickSetGroup)
    	, avsets("Select sets to be stored")
{
}


uiPickPartServ::~uiPickPartServ()
{
    delete &psg;
    avsets.deepErase();
}


bool uiPickPartServ::fetchPickSets()
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


bool uiPickPartServ::storePickSets()
{
    sendEvent( evGetAvailableSets );
    if ( !avsets.size() )
	{ uiMSG().error( "No pick sets defined yet" ); return false; }

    uiStorePicks dlg( appserv().parent(), psgid, avsets );
    if ( !dlg.go() ) return false;

    selsets.erase();
    for ( int idx=0; idx<avsets.size(); idx++ )
	if ( dlg.selectedSets()[idx] ) selsets += idx;

    psg.clear();
    sendEvent( evFetchPicks );
    if ( !psg.nrSets() ) return false;

    BufferString bs;
    if ( !PickSetGroupTranslator::store(psg,dlg.ioobj(),bs,0,dlg.merge()) )
	{ uiMSG().error( bs ); return false; }

    return true;
}
