/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uipickpartserv.cc,v 1.7 2002-06-24 15:01:39 nanne Exp $
________________________________________________________________________

-*/

#include "uipickpartserv.h"
#include "uifetchpicks.h"
#include "uistorepicks.h"
#include "uimsg.h"
#include "pickset.h"
#include "picksettr.h"
#include "color.h"
#include "uigeninputdlg.h"
#include "datainpspec.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "uiioobjsel.h"

const int uiPickPartServer::evGetAvailableSets = 0;
const int uiPickPartServer::evFetchPicks = 1;


uiPickPartServer::uiPickPartServer( uiApplService& a )
	: uiApplPartServer(a)
    	, psg(*new PickSetGroup)
    	, avsets("Select sets to be stored")
	, pickcolor(Color::DgbColor)
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
    { 
	psg.setName(dlg.getName());
	pickcolor = dlg.getPickColor();
	return true; 
    }

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


bool uiPickPartServer::storeSinglePickSet( PickSet* ps )
{
    CtxtIOObj ctio = PickSetGroupTranslator::ioContext();
    ctio.ctxt.forread = false;
    ctio.ctxt.maychdir = false;
    uiIOObjSelDlg dlg( appserv().parent(), ctio );
    dlg.setInitOutputName( ps->name() );
    if ( !dlg.go() || !dlg.ioObj() )
	return false;

    psg.clear();
    psg.add( ps );
    ctio.ioobj = 0;
    ctio.setObj( dlg.ioObj()->clone() );
    if ( !ctio.ioobj )
	uiMSG().error("Cannot find pickset in data base");
    BufferString bs;
    if ( !PickSetGroupTranslator::store( psg,ctio.ioobj,bs ) )
	{ uiMSG().error(bs); return false; }

    return true;
}


void uiPickPartServer::renamePickset( const char* oldnm, BufferString& newnm )
{
    DataInpSpec* inpspec = new StringInpSpec(oldnm);
    uiGenInputDlg dlg( appserv().parent(), "Rename Pickset", "Pickset name",
			inpspec );
    if ( dlg.go() )
    {
	newnm = dlg.text();
	if ( !strcmp(oldnm,newnm) )
	{
	    newnm = oldnm;
	    return;
	}

	avsets.erase();
	sendEvent( evGetAvailableSets );
	for ( int idx=0; idx<avsets.size(); idx++ )
	{
	    if ( !strcmp(newnm,avsets[idx]->name()) )
	    {
		BufferString msg( "Pickset: "); msg += newnm;
		msg += "\nalready exists.";
		uiMSG().about( msg );
		newnm = oldnm;
		return;
	    }
	}

    }
    else
	newnm = oldnm;
}

