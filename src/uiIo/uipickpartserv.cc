/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uipickpartserv.cc,v 1.14 2002-10-25 08:58:35 bert Exp $
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
    	, usehd1(true)
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
    selbr = &rp.bidrg;
    bool do2hors = !rp.iscube && rp.horidx2 >= 0 && rp.horidx2 != rp.horidx;
    if ( !rp.iscube )
    {
	selhorid = hinfos[rp.horidx]->id;
	hordef.erase(); usehd1 = true;
	sendEvent( evGetHorDef );
	if ( do2hors )
	{
	    selhorid = hinfos[rp.horidx2]->id;
	    hordef2.erase(); usehd1 = false;
	    sendEvent( evGetHorDef );
	    do2hors = hordef2.size() > 2;
	}
    }
    else
    {
	BinID bid( selbr->start );
	BinID stp = BinID( SI().inlStep(), SI().crlStep() );
	for ( int inl=selbr->start.inl; inl<=selbr->stop.inl; inl +=stp.inl)
	{
	    for ( int crl=selbr->start.crl; crl<=selbr->stop.crl;
		    	crl += stp.crl )
		hordef += BinIDZValue( inl, crl, mUndefValue );
	}
    }

    const int nrpts = hordef.size();
    if ( !nrpts ) return true;

    const float zwidth = rp.zrg.width();
    for ( int ipt=0; ipt<rp.nr; ipt++ )
    {
	int ptidx = Stat_getIndex( nrpts );
	BinIDZValue bv = hordef[ptidx];

	if ( rp.iscube )

	    bv.z = rp.zrg.start + Stat_getRandom() * zwidth;

	else if ( mIsUndefined(bv.z) )

	    continue;

	else if ( do2hors )
	{
	    // Horizons are likely to have equal geometry, so
	    if ( ptidx >= hordef2.size() ) ptidx = hordef2.size() / 2;
	    BinIDZValue bv2 = hordef2[ptidx];
	    if ( bv.binid != bv2.binid )
	    {
		// But alas. Need extensive search now
		bool found = false;
		for ( int idx=0; idx<hordef2.size(); idx++ )
		{
		    bv2 = hordef2[idx];
		    if ( bv.binid == bv2.binid )
			{ found = true; break; }
		}

		if ( !found ) // Need both horizons defined. Try again.
		    { ipt--; continue; }
	    }
	    bv.z += Stat_getRandom() * (bv2.z - bv.z);
	}

	ps += PickLocation( SI().transform(bv.binid), bv.z );
    }

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
	PtrUserIDObjectSet nms = avsets;
	for ( int idx=0; idx<nms.size(); idx++ )
	{
	    if ( !strcmp(newnm,nms[idx]->name()) )
	    {
		BufferString msg( "Pickset: "); msg += newnm;
		msg += "\nalready exists.";
		uiMSG().error( msg );
		newnm = oldnm;
		return;
	    }
	}

    }
    else
	newnm = oldnm;
}

