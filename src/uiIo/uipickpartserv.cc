/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uipickpartserv.cc,v 1.27 2004-07-16 15:35:26 bert Exp $
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
#include "iopar.h"
#include "survinfo.h"
#include "stats.h"
#include "ptrman.h"

const int uiPickPartServer::evGetAvailableSets = 0;
const int uiPickPartServer::evFetchPicks = 1;
const int uiPickPartServer::evGetHorInfo = 2;
const int uiPickPartServer::evGetHorDef = 3;


uiPickPartServer::uiPickPartServer( uiApplService& a )
	: uiApplPartServer(a)
    	, psg(*new PickSetGroup)
	, pickcolor(Color::DgbColor)
    	, gendef(2,true)
{
}


uiPickPartServer::~uiPickPartServer()
{
    delete &psg;
    deepErase( selhorids );
}


void uiPickPartServer::importPickSet()
{
    uiImpExpPickSet dlg( appserv().parent(), true );
    dlg.go();
}


void uiPickPartServer::exportPickSet()
{
    uiImpExpPickSet dlg( appserv().parent(), false );
    dlg.go();
}


bool uiPickPartServer::fetchPickSets()
{
    psg.clear();
    deepErase( hinfos );
    sendEvent( evGetHorInfo );
    BufferStringSet hornms;
    for ( int idx=0; idx<hinfos.size(); idx++ )
	hornms += new BufferString( hinfos[idx]->name );

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
    const bool do2hors = !rp.iscube && rp.horidx2 >= 0 && 
			 rp.horidx2 != rp.horidx;
    gendef.empty();
    deepErase( selhorids );
    if ( !rp.iscube )
    {
	selhorids += new MultiID( hinfos[rp.horidx]->multiid );
	if ( do2hors )
	    selhorids += new MultiID( hinfos[rp.horidx2]->multiid );
	sendEvent( evGetHorDef );
    }
    else
    {
	const BinID stp = BinID( SI().inlStep(), SI().crlStep() );
	BinID bid;
	for ( bid.inl=selbr->start.inl; bid.inl<=selbr->stop.inl;
		bid.inl +=stp.inl )
	{
	    for ( bid.crl=selbr->start.crl; bid.crl<=selbr->stop.crl;
		    	bid.crl += stp.crl )
		gendef.add( bid, rp.zrg.start, rp.zrg.stop );
	}
    }

    const int nrpts = gendef.totalSize();
    if ( !nrpts ) return true;

    BinID bid; Interval<float> zrg;
    for ( int ipt=0; ipt<rp.nr; ipt++ )
    {
	const int ptidx = Stat_getIndex( nrpts );
	BinIDValueSet::Pos pos = gendef.getPos( ptidx );
	gendef.get( pos, bid, zrg.start, zrg.stop );
	float val = zrg.start + Stat_getRandom() * zrg.width();

	ps += PickLocation( SI().transform(bid), val );
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
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(PickSetGroup);
    ctio->ctxt.forread = false;
    ctio->ctxt.maychdir = false;
    ctio->setName( ps->name() );
    uiIOObjSelDlg dlg( appserv().parent(), *ctio );
    if ( !dlg.go() || !dlg.ioObj() )
	return false;

    psg.clear();
    psg.add( ps );
    ctio->ioobj = 0;
    ctio->setObj( dlg.ioObj()->clone() );
    if ( !ctio->ioobj )
	uiMSG().error("Cannot find pickset in data base");
    BufferString bs;
    if ( !PickSetGroupTranslator::store( psg, ctio->ioobj, bs ) )
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
	    if ( avsets.get(idx) == newnm )
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


void uiPickPartServer::setMisclassSet( const BinIDValueSet& bivs )
{
    static const BufferString sMisClassStr = "Misclassified [NN]";

    PickSet* pickset = new PickSet( sMisClassStr );
    pickset->color.set( 255, 0, 0 );
    BinIDValueSet::Pos pos; BinIDValue biv;
    while ( bivs.next(pos,false) )
    {
	bivs.get( pos, biv );
	Coord pos = SI().transform( biv.binid );
	*pickset += PickLocation( pos.x, pos.y, biv.value );
    }

    psg.clear();
    psg.setName( sMisClassStr );
    psg.add( pickset );
}
