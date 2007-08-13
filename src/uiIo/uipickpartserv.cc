/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uipickpartserv.cc,v 1.44 2007-08-13 04:35:04 cvsraman Exp $
________________________________________________________________________

-*/

#include "uipickpartserv.h"

#include "uicursor.h"
#include "uicreatepicks.h"
#include "uiimppickset.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uipicksetman.h"

#include "ioman.h"
#include "pickset.h"
#include "picksettr.h"
#include "surfaceinfo.h"
#include "color.h"
#include "ioobj.h"
#include "survinfo.h"
#include "statrand.h"
#include "ptrman.h"

const int uiPickPartServer::evGetHorInfo = 0;
const int uiPickPartServer::evGetHorDef = 1;
const int uiPickPartServer::evFillPickSet = 2;


uiPickPartServer::uiPickPartServer( uiApplService& a )
	: uiApplPartServer(a)
	, uiPickSetMgr(Pick::Mgr())
    	, gendef_(2,true)
{
}


uiPickPartServer::~uiPickPartServer()
{
    deepErase( selhorids_ );
}


void uiPickPartServer::managePickSets()
{
    uiPickSetMan dlg( appserv().parent() );
    dlg.go();
}


void uiPickPartServer::impexpSet( bool import )
{
    uiImpExpPickSet dlg( this, import );
    dlg.go();
}


void uiPickPartServer::fetchHors()
{
    deepErase( hinfos_ );
    sendEvent( evGetHorInfo );
}

bool uiPickPartServer::loadSets()
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(PickSet);
    ctio->ctxt.forread = true;
    uiIOObjSelDlg dlg( appserv().parent(), *ctio, 0, true );
    if ( !dlg.go() ) return false;

    bool retval = false;
    const int nrsel = dlg.nrSel();
    for ( int idx=0; idx<nrsel; idx++ )
    {
	const MultiID id = dlg.selected(idx);
	PtrMan<IOObj> ioobj = IOM().get( id );
	if ( setmgr_.indexOf(ioobj->key()) >= 0 )
	    continue;

	Pick::Set* newps = new Pick::Set;
	BufferString bs;
	if ( PickSetTranslator::retrieve(*newps,ioobj,bs) )
	{
	    setmgr_.set( ioobj->key(), newps );
	    retval = true;
	}
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
		BufferString msg( ioobj->name() );
		msg += ": "; msg += bs;
		uiMSG().warning( msg );
	    }
	}
    }

    return retval;
}


bool uiPickPartServer::createSet()
{
    fetchHors();
    BufferStringSet hornms;
    for ( int idx=0; idx<hinfos_.size(); idx++ )
	hornms.add( hinfos_[idx]->name );

    uiCreatePicks3D dlg( parent(), hornms );
    if ( !dlg.go() )
    { deepErase( hinfos_ ); return false; }

    Pick::Set* newps = new Pick::Set( dlg.getName() );
    newps->disp_.color_ = dlg.getPickColor();
    if ( dlg.genRand() )
    {
	if ( !mkRandLocs(*newps,dlg.randPars()) )
	    { delete newps; newps = 0; }
    }
    
    if ( newps )
	return storeNewSet( newps );

    return false;
}


bool uiPickPartServer::mkRandLocs( Pick::Set& ps, const RandLocGenPars& rp )
{
    uiCursorChanger cursorlock( uiCursor::Wait );

    Stats::RandGen::init();
    selbr_ = &rp.bidrg;
    const bool do2hors = !rp.iscube && rp.horidx2 >= 0 && 
			 rp.horidx2 != rp.horidx;
    gendef_.empty();
    deepErase( selhorids_ );
    if ( !rp.iscube )
    {
	selhorids_ += new MultiID( hinfos_[rp.horidx]->multiid );
	if ( do2hors )
	    selhorids_ += new MultiID( hinfos_[rp.horidx2]->multiid );
	sendEvent( evGetHorDef );
    }
    else
    {
	const BinID stp = BinID( SI().inlStep(), SI().crlStep() );
	BinID bid;
	for ( bid.inl=selbr_->start.inl; bid.inl<=selbr_->stop.inl;
		bid.inl +=stp.inl )
	{
	    for ( bid.crl=selbr_->start.crl; bid.crl<=selbr_->stop.crl;
		    	bid.crl += stp.crl )
		gendef_.add( bid, rp.zrg.start, rp.zrg.stop );
	}
    }

    const int nrpts = gendef_.totalSize();
    if ( !nrpts ) return true;

    BinID bid; Interval<float> zrg;
    for ( int ipt=0; ipt<rp.nr; ipt++ )
    {
	const int ptidx = Stats::RandGen::getIndex( nrpts );
	BinIDValueSet::Pos pos = gendef_.getPos( ptidx );
	gendef_.get( pos, bid, zrg.start, zrg.stop );
	float val = zrg.start + Stats::RandGen::get() * zrg.width();

	ps += Pick::Location( SI().transform(bid), val );
    }

    gendef_.empty();
    return true;
}


void uiPickPartServer::setMisclassSet( const BinIDValueSet& bivs )
{
    static const char* sKeyMisClass = "Misclassified [NN]";
    int setidx = setmgr_.indexOf( sKeyMisClass );
    const bool isnew = setidx < 0;
    Pick::Set* ps = isnew ? 0 : &setmgr_.get( setidx );
    if ( ps )
	ps->erase();
    else
    {
	ps = new Pick::Set( sKeyMisClass );
	ps->disp_.color_.set( 240, 0, 0 );
    }

    BinIDValueSet::Pos pos; BinIDValue biv;
    while ( bivs.next(pos,false) )
    {
	bivs.get( pos, biv );
	Coord crd = SI().transform( biv.binid );
	*ps += Pick::Location( crd.x, crd.y, biv.value );
    }

    if ( isnew )
	storeNewSet( ps );
    else
    {
	PtrMan<IOObj> ioobj = getSetIOObj( *ps );
	if ( ioobj )
	    doStore( *ps, *ioobj );
    }
}


void uiPickPartServer::fillZValsFrmHor( Pick::Set* ps, int horidx )
{
    ps_ = ps;
    horid_ = hinfos_[horidx]->multiid;
    sendEvent( evFillPickSet );
}
