/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uipickpartserv.cc,v 1.11 2002-09-30 15:39:49 bert Exp $
________________________________________________________________________

-*/

#include "uipickpartserv.h"
#include "uifetchpicks.h"
#include "uistorepicks.h"
#include "uiimppickset.h"
#include "uimsg.h"
#include "uigeninputdlg.h"
#include "uiioobjsel.h"
#include "pickset.h"
#include "picksettr.h"
#include "surfaceinfo.h"
#include "datainpspec.h"
#include "ctxtioobj.h"
#include "color.h"
#include "ioobj.h"
#include "survinfo.h"
#include "stats.h"

const int uiPickPartServer::evGetAvailableSets = 0;
const int uiPickPartServer::evFetchPicks = 1;
const int uiPickPartServer::evGetHorInfo = 2;
const int uiPickPartServer::evGetHorDef = 3;


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


void uiPickPartServer::importPickSet()
{
    uiImportPickSet dlg( appserv().parent() );
    dlg.go();
}


bool uiPickPartServer::fetchPickSets()
{
    psg.clear();
    deepErase( hinfos );
    sendEvent( evGetHorInfo );
    ObjectSet<BufferString> hornms;
    for ( int idx=0; idx<hinfos.size(); idx++ )
    {
	hornms += new BufferString( hinfos[idx]->name );
    }
    uiFetchPicks dlg( appserv().parent(), psgid, hornms );
    if ( !dlg.go() ) { deepErase( hinfos ); return false; }

    if ( !dlg.nrSets() )
    { 
	psg.setName(dlg.getName());
	pickcolor = dlg.getPickColor();
	bool rv = true;
	if ( dlg.genRand() )
	{
	    IOPar iopar;
	    PickSet* ps = new PickSet( dlg.getName() );
	    ps->color = pickcolor;
	    rv = mkRandLocs( *ps, dlg.randPars() );
	    if ( rv )	psg.add(ps);
	    else	delete ps;
	}
	return rv;
    }

    BufferString bs;
    if ( !PickSetGroupTranslator::retrieve(psg,dlg.ioobj(),bs,
					   dlg.selectedSets()) )
	{ uiMSG().error( bs ); return false; }

    return true;
}


bool uiPickPartServer::mkRandLocs( PickSet& ps, const RandLocGenPars& rp )
{
    Stat_initRandom(0);
    if ( !rp.isvol )
    {
	selhorid = hinfos[rp.horidx]->id;
	selbr = &rp.bidrg;
	hordef.erase();
	sendEvent( evGetHorDef );
    }
    else
    {
	BinID bid( rp.bidrg.start );
	BinID stp = BinID( SI().inlStep(), SI().crlStep() );
	for ( int inl=rp.bidrg.start.inl; inl<=rp.bidrg.stop.inl; inl +=stp.inl)
	{
	    for ( int crl=rp.bidrg.start.crl; crl<=rp.bidrg.stop.crl;
		    	crl += stp.crl )
		hordef += BinIDZValue( inl, crl, mUndefValue );
	}
    }

    const int nrpts = hordef.size();
    if ( !nrpts ) return true;

    const float zwidth = rp.zrg.width();
    Coord c;
    for ( int ipt=0; ipt<rp.nr; ipt++ )
    {
	int idx = Stat_getIndex( nrpts );
	BinIDZValue bv = hordef[idx];
	if ( rp.isvol )
	    bv.z = rp.zrg.start + Stat_getRandom() * zwidth;
	c = SI().transform( bv.binid );

	ps += PickLocation( c, bv.z );
    }
    hordef.erase();
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

