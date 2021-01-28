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
#include "uiioobjseldlg.h"
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
    , manpicksetsdlg_(0)
{
    mAttachCB( IOM().surveyChanged, uiPickPartServer::survChangedCB );
}


uiPickPartServer::~uiPickPartServer()
{
    detachAllNotifiers();
    delete &uipsmgr_;
    delete &gendef_;
    deepErase( selhorids_ );
    delete manpicksetsdlg_;
}


void uiPickPartServer::survChangedCB( CallBacker* )
{
    delete imppsdlg_; imppsdlg_ = 0;
    delete exppsdlg_; exppsdlg_ = 0;
    delete manpicksetsdlg_; manpicksetsdlg_ = 0;
}


void uiPickPartServer::managePickSets()
{
    delete manpicksetsdlg_;
    manpicksetsdlg_ = new uiPickSetMan( parent(), nullptr );
    manpicksetsdlg_->go();
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

bool uiPickPartServer::storeNewPickSet( Pick::Set*& ps )
{ return uipsmgr_.storeNewSet( ps ); }

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


bool uiPickPartServer::reLoadSet( const MultiID& mid )
{
    PtrMan<IOObj> ioobj = IOM().get( mid );

    Pick::Set& ps = Pick::Mgr().get( mid );
    ps.setEmpty();
    BufferString bs;

    return PickSetTranslator::retrieve( ps, ioobj, true, bs );
}



void uiPickPartServer::fetchHors( bool is2d )
{
    deepErase( hinfos_ );
    sendEvent( is2d ? evGetHorInfo2D() : evGetHorInfo3D() );
}


bool uiPickPartServer::loadSets( TypeSet<MultiID>& psids, bool poly )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(PickSet);
    ctio->ctxt_.forread_ = true;
    PickSetTranslator::fillConstraints( ctio->ctxt_, poly );

    uiIOObjSelDlg::Setup sdsu; sdsu.multisel( true );
    uiString titletxt = uiStrings::phrSelect(uiStrings::sInput());
    uiString disptyp = poly ? uiStrings::sPolygon(mPlural) :
						uiStrings::sPointSet(mPlural);
    sdsu.titletext_ = toUiString("%1 %2").arg(titletxt).arg(disptyp);
    if ( poly )
	ctio->ctxt_.toselect_.dontallow_.set( sKey::Type(), sKey::PickSet() );
    else
	ctio->ctxt_.toselect_.dontallow_.set( sKey::Type(), sKey::Polygon() );

    uiIOObjSelDlg dlg( parent(), sdsu, *ctio );
    bool isforread = dlg.isForRead();
    uiString caption = !isforread ? tr("Save %1 as") :
				uiStrings::phrLoad(toUiString("%1"));
    dlg.setCaption(caption.arg(disptyp));
    dlg.showAlwaysOnTop();
    if ( !dlg.go() )
	return false;

    uiStringSet errmsgs;
    bool retval = false;
    const int nrsel = dlg.nrChosen();
    for ( int idx=0; idx<nrsel; idx++ )
    {
	const MultiID id = dlg.chosenID(idx);
	PtrMan<IOObj> ioobj = IOM().get( id );
	if ( !ioobj ) continue;

	psids += id;
	if ( Pick::Mgr().indexOf(id) >= 0 )
	{
	    retval = true;
	    continue; // No need to read again
	}

	Pick::Set* ps = new Pick::Set;
	BufferString errmsg;
	if ( PickSetTranslator::retrieve(*ps,ioobj,true,errmsg) )
	{
	    psmgr_.set( ioobj->key(), ps );
	    psids.addIfNew( id );
	    retval = true;
	}
	else
	{
	    delete ps;
	    psmgr_.set( id, 0 ); //Remove from Mgr if present.

	    uiString msg = uiStrings::phrJoinStrings(
			   ioobj->uiName(), toUiString(errmsg) );
	    errmsgs.add( msg );
	}
    }

    if ( !errmsgs.isEmpty() )
    {
	uiString msg = tr("Some problems occurred while loading PickSets");
	uiMSG().errorWithDetails( errmsgs, msg );
    }

    return retval;
}


#define mHandleDlg() \
    if ( !dlg.go() ) \
	return 0; \
    Pick::Set* newps = dlg.getPickSet();

const Pick::Set* uiPickPartServer::createEmptySet( bool aspolygon )
{
    uiCreatePicks dlg( parent(), aspolygon );
    mHandleDlg();
    if ( newps )
	uipsmgr_.storeNewSet( newps );
    return newps;
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
	uiMSG().warning( tr("No 2D lines are available in this survey") );
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

    selectlines_ = rp.linenms_;
    deepErase( selhorids_ );
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
