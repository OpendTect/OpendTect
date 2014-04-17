/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          May 2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uipickpartserv.h"

#include "uicreatepicks.h"
#include "uiimppickset.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uipicksetman.h"
#include "uipicksetmgr.h"

#include "binidvalset.h"
#include "color.h"
#include "datapointset.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "mousecursor.h"
#include "pickset.h"
#include "picksettr.h"
#include "posinfo2d.h"
#include "ptrman.h"
#include "statrand.h"
#include "surfaceinfo.h"
#include "survgeom2d.h"
#include "survinfo.h"

int uiPickPartServer::evGetHorInfo2D()		{ return 0; }
int uiPickPartServer::evGetHorInfo3D()		{ return 1; }
int uiPickPartServer::evGetHorDef3D()		{ return 2; }
int uiPickPartServer::evGetHorDef2D()		{ return 3; }
int uiPickPartServer::evFillPickSet()		{ return 4; }
int uiPickPartServer::evGet2DLineDef()		{ return 6; }
int uiPickPartServer::evDisplayPickSet()	{ return 7; }


uiPickPartServer::uiPickPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , psmgr_(Pick::Mgr())
    , uipsmgr_(*new uiPickSetMgr(parent(),psmgr_))
    , gendef_(*new BinIDValueSet(2,true))
    , selhs_(true)
    , ps_(0)
    , imppsdlg_(0)
    , exppsdlg_(0)
{
    IOM().surveyChanged.notify( mCB(this,uiPickPartServer,survChangedCB) );
}


uiPickPartServer::~uiPickPartServer()
{
    delete &uipsmgr_;
    delete &gendef_;
    deepErase( selhorids_ );
    deepErase( linegeoms_ );
}


void uiPickPartServer::survChangedCB( CallBacker* )
{
    delete imppsdlg_; imppsdlg_ = 0;
    delete exppsdlg_; exppsdlg_ = 0;
}


void uiPickPartServer::managePickSets()
{
    uiPickSetMan dlg( parent() );
    dlg.go();
}


void uiPickPartServer::importSet()
{
    if ( !imppsdlg_ )
    {
	imppsdlg_ = new uiImpExpPickSet( parent(), this, true );
	imppsdlg_->importReady.notify(
		mCB(this,uiPickPartServer,importReadyCB) );
    }

    imppsdlg_->show();
    imppsdlg_->raise();
}


void uiPickPartServer::importReadyCB( CallBacker* cb )
{
    if ( imppsdlg_ && cb==imppsdlg_ )
    {
	picksetid_ = imppsdlg_->getStoredID();
	sendEvent( evDisplayPickSet() );
    }
}


void uiPickPartServer::exportSet()
{
    if ( !exppsdlg_ )
	exppsdlg_ = new uiImpExpPickSet( parent(), this, false );

    exppsdlg_->show();
    exppsdlg_->raise();
}


bool uiPickPartServer::storePickSets()
{ return uipsmgr_.storeSets(); }

bool uiPickPartServer::storePickSet( const Pick::Set& ps )
{ return uipsmgr_.storeSet( ps ); }

bool uiPickPartServer::storePickSetAs( const Pick::Set& ps )
{ return uipsmgr_.storeSetAs( ps ); }

bool uiPickPartServer::pickSetsStored() const
{ return uipsmgr_.pickSetsStored(); }

void uiPickPartServer::mergePickSets( MultiID& mid )
{ uipsmgr_.mergeSets( mid ); }


Pick::Set* uiPickPartServer::loadSet( const MultiID& mid )
{
    PtrMan<IOObj> ioobj = IOM().get( mid );
    const int setidx = psmgr_.indexOf( mid );
    if ( setidx<0 )
    {
	Pick::Set* ps = new Pick::Set;
	BufferString bs;
	if ( PickSetTranslator::retrieve(*ps,ioobj,true,bs) )
	{
	    psmgr_.set( mid, ps );
	    return ps;
	}

	delete ps;
	return 0;
    }

    return &(psmgr_.get(setidx));
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

    uiIOObjSelDlg dlg( parent(), *ctio, 0, true );
    if ( !dlg.go() ) return false;

    bool retval = false;
    const int nrsel = dlg.nrSel();
    for ( int idx=0; idx<nrsel; idx++ )
    {
	const MultiID id = dlg.selected(idx);
	psids += id;
	PtrMan<IOObj> ioobj = IOM().get( id );
	const int setidx = psmgr_.indexOf( ioobj->key() );
	Pick::Set* ps = setidx < 0 ? new Pick::Set : &(psmgr_.get(setidx));
	BufferString bs;
	if ( PickSetTranslator::retrieve(*ps,ioobj,true, bs) )
	{
	    if ( setidx < 0 )
		psmgr_.set( ioobj->key(), ps );

	    psids.addIfNew( ioobj->key() );
	    retval = true;
	}
	else
	{
	    if ( setidx < 0 )
		delete ps;
	    else
		psmgr_.set( ioobj->key(), 0 ); //Remove from Mgr if present.

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
    return newps ? uipsmgr_.storeNewSet( newps ) : false;
}


bool uiPickPartServer::create3DGenSet()
{
    uiGenPosPicks dlg( parent() );
    mHandleDlg();
    return newps ? uipsmgr_.storeNewSet( newps ) : false;
}


bool uiPickPartServer::createRandom2DSet()
{
    fetchHors( true );
    BufferStringSet hornms;
    for ( int idx=0; idx<hinfos_.size(); idx++ )
	hornms.add( hinfos_[idx]->name );

    BufferStringSet linenames;
    TypeSet<Pos::GeomID> geomids;
    Survey::GM().getList( linenames, geomids, true );
    uiGenRandPicks2D dlg( parent(), hornms, linenames );
    if ( linenames.isEmpty() )
    {
	uiMSG().warning( "No 2D lines are available in this survey" );
	return false;
    }

    mHandleDlg();
    if ( !mkRandLocs2D(*newps,dlg.randPars()) )
    { delete newps; newps = 0; }
    if ( newps )
	return uipsmgr_.storeNewSet( newps );
   
    return false;
}


bool uiPickPartServer::mkRandLocs2D(Pick::Set& ps,const RandLocGenPars& rp)
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );

    Stats::randGen().init();
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
	for ( int iln=0; iln<selectlines_.size(); iln++ )
	{
	    const Survey::Geometry* geom = 
		Survey::GM().getGeometry( selectlines_.get(iln) );
	    mDynamicCastGet(const Survey::Geometry2D*,geom2d,geom);
	    if ( !geom2d )
		continue;

	    const TypeSet<PosInfo::Line2DPos>& posns = 
				    geom2d->data().positions();
	    for ( int ipos=0; ipos<posns.size(); ipos++ )
		coords2d_ += posns[ipos].coord_;
	}

	deepErase( linegeoms_ );
    }

    const int nrpos = coords2d_.size();
    if ( !nrpos ) return false;

    for ( int ipt=0; ipt<rp.nr_; ipt++ )
    {
	const int posidx = Stats::randGen().getIndex( nrpos );
	Interval<float> zrg = rp.needhor_ ? hor2dzrgs_[posidx] : rp.zrg_;
	float val = (float) ( zrg.start +
				  Stats::randGen().get() * zrg.width(false) );
	ps += Pick::Location( coords2d_[posidx], val );
    }

    return true;
}


void uiPickPartServer::setPickSet( const Pick::Set& pickset )
{
    const int setidx = psmgr_.indexOf( pickset.name() );
    const bool isnew = setidx < 0;
    Pick::Set* ps = isnew ? 0 : &psmgr_.get( setidx );
    if ( ps )
	ps->erase();
    else
    {
	ps = new Pick::Set( pickset.name() );
	ps->disp_.color_.set( 240, 0, 0 );
    }

    *ps = pickset;

    if ( isnew )
	uipsmgr_.storeNewSet( ps );
    else
    {
	uipsmgr_.storeSet( *ps );
	psmgr_.reportChange( 0, *ps );
    }
}


void uiPickPartServer::setMisclassSet( const DataPointSet& dps )
{
    const char* sKeyMisClass = "Misclassified [NN]";
    int setidx = psmgr_.indexOf( sKeyMisClass );
    const bool isnew = setidx < 0;
    Pick::Set* ps = isnew ? 0 : &psmgr_.get( setidx );
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
	uipsmgr_.storeNewSet( ps );
    else
	uipsmgr_.storeSet( *ps );
}


void uiPickPartServer::fillZValsFrmHor( Pick::Set* ps, int horidx )
{
    ps_ = ps;
    horid_ = hinfos_[horidx]->multiid;
    sendEvent( evFillPickSet() );
}
