/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          May 2001
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uipickpartserv.cc,v 1.73 2012-08-01 11:55:56 cvsmahant Exp $";

#include "uipickpartserv.h"

#include "mousecursor.h"
#include "uicreatepicks.h"
#include "uiimppickset.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uipicksetman.h"

#include "ioman.h"
#include "keystrs.h"
#include "pickset.h"
#include "picksettr.h"
#include "surfaceinfo.h"
#include "color.h"
#include "ioobj.h"
#include "survinfo.h"
#include "posinfo2d.h"
#include "statrand.h"
#include "ptrman.h"

int uiPickPartServer::evGetHorInfo2D()	{ return 0; }
int uiPickPartServer::evGetHorInfo3D()	{ return 1; } 
int uiPickPartServer::evGetHorDef3D()	{ return 2; }
int uiPickPartServer::evGetHorDef2D()	{ return 3; }
int uiPickPartServer::evFillPickSet()	{ return 4; }
int uiPickPartServer::evGet2DLineInfo()	{ return 5; }
int uiPickPartServer::evGet2DLineDef()	{ return 6; }


uiPickPartServer::uiPickPartServer( uiApplService& a )
	: uiApplPartServer(a)
	, uiPickSetMgr(Pick::Mgr())
    	, gendef_(2,true)
    	, selhs_(true)
	, ps_(0)
{
}


uiPickPartServer::~uiPickPartServer()
{
    deepErase( selhorids_ );
    deepErase( linegeoms_ );
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


void uiPickPartServer::fetchHors( bool is2d )
{
    deepErase( hinfos_ );
    sendEvent( is2d ? evGetHorInfo2D() : evGetHorInfo3D() );
}


bool uiPickPartServer::loadSets( TypeSet<MultiID>& psids, bool poly )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(PickSet);
    ctio->ctxt.forread = true;
    if ( poly )
	ctio->ctxt.toselect.require_.set( sKey::Type(), sKey::Polygon() );

    uiIOObjSelDlg dlg( appserv().parent(), *ctio, 0, true );
    if ( !dlg.go() ) return false;

    bool retval = false;
    const int nrsel = dlg.nrSel();
    for ( int idx=0; idx<nrsel; idx++ )
    {
	const MultiID id = dlg.selected(idx);
	psids += id;
	PtrMan<IOObj> ioobj = IOM().get( id );
	if ( setmgr_.indexOf(ioobj->key()) >= 0 )
	    continue;

	Pick::Set* newps = new Pick::Set;
	BufferString bs;
	if ( PickSetTranslator::retrieve(*newps,ioobj,true, bs) )
	{
	    setmgr_.set( ioobj->key(), newps );
	    psids.addIfNew( ioobj->key() );
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


#define mHandleDlg() \
    if ( !dlg.go() ) \
        return false; \
    Pick::Set* newps = dlg.getPickSet();

bool uiPickPartServer::createEmptySet( bool aspolygon )
{
    uiCreatePicks dlg( parent(), aspolygon );
    mHandleDlg();
    return newps ? storeNewSet( newps ) : false;
}


bool uiPickPartServer::create3DGenSet()
{
    uiGenPosPicks dlg( parent() );
    mHandleDlg();
    return newps ? storeNewSet( newps ) : false;
}


bool uiPickPartServer::createRandom2DSet()
{
    fetchHors( true );
    BufferStringSet hornms;
    for ( int idx=0; idx<hinfos_.size(); idx++ )
	hornms.add( hinfos_[idx]->name );

    deepErase( linesets_ );
    linenms_.erase();
    sendEvent( evGet2DLineInfo() );
    uiGenRandPicks2D dlg( parent(), hornms, linesets_, linenms_ );
    mHandleDlg();
    if ( !mkRandLocs2D(*newps,dlg.randPars()) )
    { delete newps; newps = 0; }
    if ( newps )
	return storeNewSet( newps );

    return false;
}


bool uiPickPartServer::mkRandLocs2D(Pick::Set& ps,const RandLocGenPars& rp)
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );

    Stats::RandGen::init();
    setid_ = setids_[rp.lsetidx_];
    selectlines_ = rp.linenms_;
    deepErase( linegeoms_ );
    deepErase( selhorids_ );
    sendEvent( evGet2DLineDef() );
    if ( !linegeoms_.size() ) return false;

    coords2d_.erase();
    if ( rp.needhor_ )
    {
	selhorids_ += new MultiID( hinfos_[rp.horidx_]->multiid );
	if ( rp.horidx2_ >= 0 )
	    selhorids_ += new MultiID( hinfos_[rp.horidx2_]->multiid );

	hor2dzrgs_.erase();
	sendEvent( evGetHorDef2D() );
    }
    else
    {
	for ( int iln=0; iln<linegeoms_.size(); iln++ )
	{
	    const TypeSet<PosInfo::Line2DPos>& posns
					= linegeoms_[iln]->positions();
	    for ( int ipos=0; ipos<posns.size(); ipos++ )
		coords2d_ += posns[ipos].coord_;
	}
	
	deepErase( linegeoms_ );
    }

    const int nrpos = coords2d_.size();
    if ( !nrpos ) return false;

    for ( int ipt=0; ipt<rp.nr_; ipt++ )
    {
	const int posidx = Stats::RandGen::getIndex( nrpos );
	Interval<float> zrg = rp.needhor_ ? hor2dzrgs_[posidx] : rp.zrg_;
	float val = zrg.start + Stats::RandGen::get() * zrg.width(false);
	ps += Pick::Location( coords2d_[posidx], val );
    }

    return true;
}


void uiPickPartServer::setPickSet( const Pick::Set& pickset )
{
    const int setidx = setmgr_.indexOf( pickset.name() );
    const bool isnew = setidx < 0;
    Pick::Set* ps = isnew ? 0 : &setmgr_.get( setidx );
    if ( ps )
	ps->erase();
    else
    {
	ps = new Pick::Set( pickset.name() );
	ps->disp_.color_.set( 240, 0, 0 );
    }

    *ps = pickset;

    if ( isnew )
	storeNewSet( ps );
    else
    {
	PtrMan<IOObj> ioobj = getSetIOObj( *ps );
	if ( ioobj )
	    doStore( *ps, *ioobj );
	setmgr_.reportChange( 0, *ps );
    }
}


void uiPickPartServer::setMisclassSet( const DataPointSet& dps )
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

    for ( int idx=0; idx<dps.size(); idx++ )
    {
	Coord crd = dps.coord( idx );
	*ps += Pick::Location( crd.x, crd.y, dps.z(idx) );
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
    sendEvent( evFillPickSet() );
}
