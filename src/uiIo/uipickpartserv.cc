/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uipickpartserv.cc,v 1.34 2006-05-16 16:28:22 cvsbert Exp $
________________________________________________________________________

-*/

#include "uipickpartserv.h"
#include "uifetchpicks.h"
#include "uiimppickset.h"
#include "uimsg.h"
#include "uigeninputdlg.h"
#include "uiioobjsel.h"
#include "uicursor.h"
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
    	, ps(*new Pick::Set)
	, pickcolor(Color::DgbColor)
    	, gendef(2,true)
{
}


uiPickPartServer::~uiPickPartServer()
{
    delete &ps;
    deepErase( selhorids );
}


void uiPickPartServer::impexpSet( bool import )
{
    uiImpExpPickSet dlg( appserv().parent(), import );
    dlg.go();
}


bool uiPickPartServer::fetchSets()
{
    deepErase( hinfos );
    sendEvent( evGetHorInfo );
    BufferStringSet hornms;
    for ( int idx=0; idx<hinfos.size(); idx++ )
	hornms.add( hinfos[idx]->name );

    deepErase( pssfetched );
    uiFetchPicks dlg( appserv().parent(), psid, hornms );
    if ( !dlg.go() )
	{ deepErase( hinfos ); return false; }

    deepErase( pssfetched );
    if ( dlg.mkNew() )
    {
	Pick::Set* newps = new Pick::Set( dlg.getName() );
	pickcolor = dlg.getPickColor();
	bool rv = true;
	if ( dlg.genRand() )
	{
	    newps->color_ = pickcolor;
	    if ( !mkRandLocs(*newps,dlg.randPars()) )
		{ delete newps; newps = 0; }
	}

	if ( newps )
	{
	    pssfetched += newps;
	    PtrMan<IOObj> ioobj = dlg.storObj();
	    if ( newps && ioobj )
	    {
		BufferString bs;
		if ( !PickSetTranslator::store(*newps,ioobj,bs) )
		    uiMSG().error( bs );
	    }
	}
    }

    BufferString bs;
    for ( int idx=0; dlg.ioobj(idx); idx++ )
    {
	Pick::Set* newps = new Pick::Set;
	if ( PickSetTranslator::retrieve(*newps,dlg.ioobj(idx),bs) )
	    pssfetched += newps;
	else
	{
	    delete newps;
	    if ( idx == 0 )
	    {
		uiMSG().error( bs );
		return false;
	    }
	    else
	    {
		BufferString msg( dlg.ioobj(idx)->name() );
		msg += ": "; msg += bs;
		uiMSG().warning( msg );
	    }
	}
    }

    return pssfetched.size() > 0;
}


bool uiPickPartServer::mkRandLocs( Pick::Set& ps, const RandLocGenPars& rp )
{
    uiCursorChanger cursorlock( uiCursor::Wait );

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

	ps += Pick::Location( SI().transform(bid), val );
    }

    gendef.empty();
    return true;
}


bool uiPickPartServer::storeSets()
{
    // TODO store all sets that have changed
    uiMSG().error( "Not supported yet. Please save sets one by one" );
    return true;
}


bool uiPickPartServer::storeSetAs()
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(PickSet);
    ctio->ctxt.forread = false;
    ctio->ctxt.maychdir = false;
    ctio->setName( ps.name() );
    uiIOObjSelDlg dlg( appserv().parent(), *ctio );
    if ( !dlg.go() || !dlg.ioObj() )
	return false;

    ctio->ioobj = 0;
    ctio->setObj( dlg.ioObj()->clone() );
    if ( !ctio->ioobj )
	uiMSG().error("Cannot find pickset in data base");
    BufferString bs;
    if ( !PickSetTranslator::store( ps, ctio->ioobj, bs ) )
	{ uiMSG().error(bs); return false; }

    return true;
}


bool uiPickPartServer::storeSet()
{
    //TODO implement, no user intervention
    return storeSetAs();
}


void uiPickPartServer::renameSet( const char* oldnm, BufferString& newnm )
{
    uiGenInputDlg dlg( appserv().parent(), "Rename Pickset", "Pickset name",
			new StringInpSpec(oldnm) );
    if ( !dlg.go() )
	newnm = oldnm;
    else
    {
	newnm = dlg.text();
	if ( newnm == oldnm )
	    return;

	avsets.erase();
	sendEvent( evGetAvailableSets );
	for ( int idx=0; idx<avsets.size(); idx++ )
	{
	    if ( newnm == avsets.get(idx) )
	    {
		BufferString msg( "Pickset: '"); msg += newnm;
		msg += "'\nalready exists.";
		uiMSG().error( msg );
		newnm = oldnm;
		return;
	    }
	}

    }
}


void uiPickPartServer::setMisclassSet( const BinIDValueSet& bivs )
{
    ps.erase();
    ps.setName( "Misclassified [NN]" );
    ps.color_.set( 240, 0, 0 );
    BinIDValueSet::Pos pos; BinIDValue biv;
    while ( bivs.next(pos,false) )
    {
	bivs.get( pos, biv );
	Coord pos = SI().transform( biv.binid );
	ps += Pick::Location( pos.x, pos.y, biv.value );
    }
}
