/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : J.C. Glas
 * DATE     : December 2010
-*/

static const char* rcsID mUsedVar = "$Id$";


#include "visseedpainter.h"

#include "color.h"
#include "mousecursor.h"
#include "mouseevent.h"
#include "pickset.h"
#include "picksetmgr.h"
#include "settings.h"
#include "statrand.h"
#include "survinfo.h"
#include "timefun.h"
#include "trckeyzsampling.h"
#include "visevent.h"
#include "vislocationdisplay.h"
#include "vismaterial.h"
#include "visplanedatadisplay.h"
#include "vispolyline.h"
#include "vissurvscene.h"
#include "vistransform.h"
#include "vistransmgr.h"

namespace visSurvey
{

int SeedPainter::radius_ = 20;
int SeedPainter::density_ = 10;

TypeSet<Geom::PointI> SeedPainter::circlecoords_;

int SeedPainter::density()
{ return density_; }

void SeedPainter::setDensity( int perc )
{ density_ = perc; }

int SeedPainter::radius()
{ return radius_; }

void SeedPainter::setRadius( int nrsamps )
{
    radius_ = nrsamps;
    circlecoords_.erase();
}


void SeedPainter::mkCircle()
{
    int midpt = mNINT32(radius_ * 0.707);
    for ( int xval=-midpt; xval<midpt; xval++ )
    {
	const int yval =
	    mNINT32( Math::Sqrt( (float) radius_*radius_ - xval*xval ) );
	circlecoords_.add( Geom::PointI(xval,yval) );
    }

    for ( int yval=midpt; yval>-midpt; yval-- )
    {
	const int xval =
	    mNINT32( Math::Sqrt( (float) radius_*radius_ - yval*yval ) );
	circlecoords_.add( Geom::PointI(xval,yval) );
    }

    for ( int xval=midpt; xval>-midpt; xval-- )
    {
	const int yval =
	    -mNINT32( Math::Sqrt( (float) radius_*radius_ - xval*xval ) );
	circlecoords_.add( Geom::PointI(xval,yval) );
    }

    for ( int yval=-midpt; yval<midpt; yval++ )
    {
	const int xval =
	    -mNINT32( Math::Sqrt( (float) radius_*radius_ - yval*yval ) );
	circlecoords_.add( Geom::PointI(xval,yval) );
    }
}


SeedPainter::SeedPainter()
    : visBase::VisualObjectImpl(false)
    , eventcatcher_(0)
    , transformation_(0)
    , circle_(visBase::PolyLine::create())
    , prevev_(nullptr)
{
    circle_->ref();
    circle_->setMaterial( new visBase::Material );
    circle_->setLineStyle( OD::LineStyle(OD::LineStyle::Solid,3) );
    circle_->setPickable( false, false );
    addChild( circle_->osgNode() );
}


SeedPainter::~SeedPainter()
{
    removeChild( circle_->osgNode() );
    circle_->unRef();
    deleteAndZeroPtr( prevev_ );
}


void SeedPainter::setSet( Pick::Set* ps )
{
    set_ = ps;
}


void SeedPainter::setSetMgr( Pick::SetMgr* mgr )
{
    picksetmgr_ = mgr;
}


void SeedPainter::setDisplayTransformation( const mVisTrans* transformation )
{
    if ( transformation_ )
	transformation_->unRef();

    transformation_ = transformation;
    if ( transformation_ )
	transformation_->ref();

    circle_->setDisplayTransformation( transformation );
}


void SeedPainter::setEventCatcher( visBase::EventCatcher* eventcatcher )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove(mCB(this,SeedPainter,eventCB));
	eventcatcher_->unRef();
    }

    eventcatcher_ = eventcatcher;

    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.notify(mCB(this,SeedPainter,eventCB));
	eventcatcher_->ref();
    }
}


#define mReturnHandled( yn ) \
{ \
    if ( yn && eventcatcher_ ) eventcatcher_->setHandled(); \
    return yn; \
}


bool SeedPainter::activate()
{
    if ( !set_ || !picksetmgr_ )
	return false;

    active_ = true;

    set_->disp_.markertype_ = MarkerStyle3D::Point;
    picksetmgr_->reportDispChange( this, *set_ );
    circle_->getMaterial()->setColor( set_->disp_.color_ );
    circle_->turnOn( true );
    return true;
}


void SeedPainter::deActivate()
{
    reset();
}


void SeedPainter::eventCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const visBase::EventInfo&, eventinfo, cb );
    accept( eventinfo );
}


bool SeedPainter::accept( const visBase::EventInfo& ei )
{
    PtrMan<visBase::EventInfo> eventinfo = new visBase::EventInfo( ei );

    if ( eventinfo->tabletinfo )
	return acceptTablet( *eventinfo );

    return acceptMouse( *eventinfo );
}


bool SeedPainter::acceptMouse( const visBase::EventInfo& eventinfo )
{
    if ( !active_ )
	mReturnHandled( false );

    if ( eventinfo.type == visBase::Keyboard )
	mReturnHandled( true );

    if ( eventinfo.type==visBase::MouseMovement && !eventinfo.dragging )
	mReturnHandled( false );

    if ( eventinfo.type==visBase::MouseClick )
    {
	circle_->turnOn( eventinfo.pressed );
	if ( eventinfo.pressed )
	{
	    drawLine( eventinfo );
	    prevev_ = new visBase::EventInfo( eventinfo );
	    mReturnHandled( true );
	}

	if ( !prevev_ )
	    mReturnHandled( false );

	if ( OD::ctrlKeyboardButton(eventinfo.buttonstate_) )
	    eraseSeeds( eventinfo, *prevev_ );
	else
	    paintSeeds( eventinfo, *prevev_ );

	picksetmgr_->undo().setUserInteractionEnd(
				picksetmgr_->undo().currentEventID() );
	deleteAndZeroPtr( prevev_ );
	mReturnHandled( true );
    }

    if ( eventinfo.type==visBase::MouseMovement )
    {
	drawLine( eventinfo );
	if ( !prevev_ )
	{
	    prevev_ = new visBase::EventInfo( eventinfo );
	    mReturnHandled( true );
	}

	if ( OD::ctrlKeyboardButton(eventinfo.buttonstate_) )
	    eraseSeeds( eventinfo, *prevev_ );
	else
	    paintSeeds( eventinfo, *prevev_ );

	prevev_ = new visBase::EventInfo( eventinfo );
	mReturnHandled( true );
    }

    mReturnHandled( false );
}


static const PlaneDataDisplay* getSectionDisplay( const visBase::EventInfo& ev )
{
    const PlaneDataDisplay* pdd = nullptr;
    for ( int idx=0; idx<ev.pickedobjids.size(); idx++ )
    {
        mDynamicCast(const PlaneDataDisplay*,pdd,
		     visBase::DM().getObject(ev.pickedobjids[idx]))
        if ( pdd )
            break;
    }

    return pdd;
}


void SeedPainter::paintSeeds( const visBase::EventInfo& curev,
			      const visBase::EventInfo& prevev )
{
    Scene* scene = STM().currentScene();
    const PlaneDataDisplay* cursec = getSectionDisplay( curev );
    const PlaneDataDisplay* prevsec = getSectionDisplay( prevev );
    if ( !scene || !cursec || cursec != prevsec || !cursec->isInlCrl() )
	return;

    const TrcKeyZSampling tkzs = cursec->getTrcKeyZSampling();
    const bool isinl = cursec->getOrientation()==OD::InlineSlice;
    const StepInterval<int> nrrg = isinl ? tkzs.hsamp_.trcRange()
					 : tkzs.hsamp_.lineRange();

    const float zstretch = scene->getFixedZStretch() * scene->getTempZStretch();
    const float fac = (scene->getApparentVelocity(zstretch) * tkzs.zsamp_.step)/
		(2 * (isinl ? SI().crlDistance() : SI().inlDistance()) );
    const Coord3 curpos = curev.worldpickedpos;
    const Coord3 prevpos = prevev.worldpickedpos;
    const bool fillprev = prevev.type == visBase::MouseClick;
    const BinID curbid = SI().transform( curpos );
    const BinID prevbid = SI().transform( prevpos );
    int nrdiff = isinl ? curbid.crl() - prevbid.crl()
			: curbid.inl() - prevbid.inl();
    nrdiff = mNINT32(nrdiff/fac);
    const int cursampidx = SI().zRange().nearestIndex( curpos.z );
    const int prevsampidx = SI().zRange().nearestIndex( prevpos.z );
    const int sampdiff = cursampidx - prevsampidx;

    const int nrpts = mNINT32(radius_ * radius_ * density() / 100) + 1;
    const int dia = radius_ * 2 + 1;
    const Stats::RandGen rgx = Stats::randGen();
    const Stats::RandGen rgy = Stats::randGen();
    TypeSet<Pick::Location> mylocs;
    TypeSet<int> indexes;
    int locidx = set_->size();

#define mAddPos(mynr,sampidx) \
    { \
	const BinID bid( isinl ? curbid.inl() : mynr, \
			   isinl ? mynr : curbid.crl() ); \
	const float z = SI().zRange().atIndex( sampidx ); \
	if ( tkzs.hsamp_.includes(bid,true) && tkzs.zsamp_.includes(z,false) ) \
	{ \
	    const Coord mypos = SI().transform( bid ); \
	    Pick::Location myloc( mypos, z ); \
	    mylocs.add( myloc ); \
	    indexes += locidx++; \
	} \
    }

    for ( int ridx=0; ridx<nrpts; ridx++ )
    {
	const int ptidx1 = rgx.getIndex( dia ) - radius_;
	const int ptidx2 = rgx.getIndex( dia ) - radius_;
	const int ptidy1 = rgy.getIndex( dia ) - radius_;
	const int ptidy2 = rgy.getIndex( dia ) - radius_;
	const bool pt1incircle =
	    (radius_*radius_) >= (ptidx1*ptidx1 + ptidy1*ptidy1);
	const bool pt2incircle =
	    (radius_*radius_) >= (ptidx2*ptidx2 + ptidy2*ptidy2);
	const int nr1 = mNINT32(fac*ptidx1) +
	    		(isinl ? prevbid.crl() : prevbid.inl());
	const int nr2 = mNINT32(fac*ptidx2) +
	    		(isinl ? curbid.crl() : curbid.inl());
	const int sampidx1 = ptidy1 + prevsampidx;
	const int sampidx2 = ptidy2 + cursampidx;
	if ( fillprev && pt1incircle )
	    mAddPos( nr1, sampidx1 )

	if ( !pt2incircle )
	    continue;

	const int xdiff = ptidx2 + nrdiff;
	const int ydiff = ptidy2 + sampdiff;
	const bool pt2incircle1 =
	    (radius_*radius_) >= (xdiff*xdiff + ydiff*ydiff);
	if ( !pt2incircle1 )
	    mAddPos( nr2, sampidx2 );
    }

    set_->bulkAppendWithUndo( mylocs, indexes );
    Pick::SetMgr::BulkChangeData cd( Pick::SetMgr::BulkChangeData::Added,
	    			     set_, indexes );
    picksetmgr_->reportBulkChange( 0, cd );
}


void SeedPainter::eraseSeeds( const visBase::EventInfo& curev,
			      const visBase::EventInfo& prevev )
{
    Scene* scene = STM().currentScene();
    const PlaneDataDisplay* cursec = getSectionDisplay( curev );
    const PlaneDataDisplay* prevsec = getSectionDisplay( prevev );
    if ( !scene || !cursec || cursec != prevsec || !cursec->isInlCrl() )
	return;

    const TrcKeyZSampling tkzs = cursec->getTrcKeyZSampling();
    const bool isinl = cursec->getOrientation()==OD::InlineSlice;
    const StepInterval<int> nrrg = isinl ? tkzs.hsamp_.trcRange()
					 : tkzs.hsamp_.lineRange();

    const float zstretch = scene->getFixedZStretch() * scene->getTempZStretch();
    const float fac = (scene->getApparentVelocity(zstretch) * tkzs.zsamp_.step)/
		(2 * (isinl ? SI().crlDistance() : SI().inlDistance()) );
    const Coord3 curpos = curev.worldpickedpos;
//    const Coord3 prevpos = prevev.worldpickedpos;
//    const bool doprev = prevev.type == visBase::MouseClick;
    const BinID curbid = SI().transform( curpos );
//    const BinID prevbid = SI().transform( prevpos );

    TypeSet<Pick::Location> mylocs;
    TypeSet<int> indexes;

    for ( int idx=0; idx<set_->size(); idx++ )
    {
	const Pick::Location& loc = set_->get( idx );
	const BinID bid = SI().transform( loc.pos() );
	if ( !tkzs.hsamp_.includes(bid) ||
		!tkzs.zsamp_.includes(loc.pos().z,false) )
	    continue;

	int nrdiff = Math::Abs( isinl ? (curbid.crl() - bid.crl())
				      : (curbid.inl() - bid.inl()) ) / fac;
	int sampdiff = Math::Abs( loc.pos().z - curpos.z ) / SI().zStep();
	float distsq = nrdiff*nrdiff + sampdiff*sampdiff;
	if ( distsq > radius_*radius_ )
	    continue;

	mylocs.add( loc );
	indexes.add( idx );
    }

    set_->bulkRemoveWithUndo( mylocs, indexes );
    Pick::SetMgr::BulkChangeData cd( Pick::SetMgr::BulkChangeData::ToBeRemoved,
	    			     set_, indexes );
    picksetmgr_->reportBulkChange( 0, cd );
}


void SeedPainter::drawLine( const visBase::EventInfo& eventinfo )
{
    for ( int idx=circle_->size()-1; idx>=0; idx-- )
	circle_->removePoint( idx );

    circle_->dirtyCoordinates();
    Scene* scene = STM().currentScene();
    const PlaneDataDisplay* pdd = getSectionDisplay( eventinfo );
    if ( !pdd || !scene )
	return;

    Coord3 pickedpos = eventinfo.worldpickedpos;
    BinID pickedbid = SI().transform( pickedpos );
    const TrcKeyZSampling tkzs = pdd->getTrcKeyZSampling();
    const bool isinl = pdd->getOrientation()==OD::InlineSlice;

    if ( circlecoords_.isEmpty() )
	mkCircle();

    const float zstretch = scene->getFixedZStretch() * scene->getTempZStretch();
    const float fac = (scene->getApparentVelocity(zstretch) * tkzs.zsamp_.step)/
		(2 * (isinl ? SI().crlDistance() : SI().inlDistance()) );
    for ( int idx=0; idx<circlecoords_.size(); idx++ )
    {
	Coord pt = SI().transform( BinID(pickedbid.inl(),
		    	pickedbid.crl()+mNINT32(fac*circlecoords_[idx].x)) );
	circle_->addPoint(
		Coord3(pt,pickedpos.z+circlecoords_[idx].y*SI().zStep()) );
    }

    circle_->dirtyCoordinates();
}


void SeedPainter::reset()
{
    circle_->turnOn( false );
    for ( int idx=circle_->size()-1; idx>=0; idx-- )
	circle_->removePoint( idx );

    circle_->dirtyCoordinates();
    active_ = false;
}


bool SeedPainter::acceptTablet( const visBase::EventInfo& eventinfo )
{
    if ( !eventinfo.tabletinfo )
	mReturnHandled( false );

    return acceptMouse( eventinfo );
}


}; //namespace
