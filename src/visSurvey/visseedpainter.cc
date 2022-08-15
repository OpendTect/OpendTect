/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : J.C. Glas
 * DATE     : December 2010
-*/



#include "visseedpainter.h"

#include "color.h"
#include "mousecursor.h"
#include "mouseevent.h"
#include "pickset.h"
#include "picksetmgr.h"
#include "posinfo2d.h"
#include "settings.h"
#include "statrand.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "timefun.h"
#include "trckeyzsampling.h"
#include "visevent.h"
#include "vislocationdisplay.h"
#include "vismaterial.h"
#include "visplanedatadisplay.h"
#include "vispolyline.h"
#include "visrandomtrackdisplay.h"
#include "visseis2ddisplay.h"
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

    drawLine( eventinfo );
    if ( eventinfo.type==visBase::MouseClick )
    {
	if ( eventinfo.pressed )
	{
	    prevev_ = new visBase::EventInfo( eventinfo );
	    isleftbutpressed_ = eventinfo.buttonstate_ & OD::LeftButton;
	    mReturnHandled( true );
	}

	isleftbutpressed_ = false;
	if ( !prevev_ || !(eventinfo.buttonstate_ & OD::LeftButton) )
	    mReturnHandled( false );

	if ( OD::ctrlKeyboardButton(eventinfo.buttonstate_) )
	    eraseSeeds( eventinfo );
	else
	    paintSeeds( eventinfo, *prevev_ );

	picksetmgr_->undo().setUserInteractionEnd(
				picksetmgr_->undo().currentEventID() );

	deleteAndZeroPtr( prevev_ );
	mReturnHandled( true );
    }

    if ( eventinfo.type==visBase::MouseMovement )
    {
	if ( !eventinfo.dragging || !isleftbutpressed_ )
	    mReturnHandled( false );

	if ( !prevev_ )
	{
	    prevev_ = new visBase::EventInfo( eventinfo );
	    mReturnHandled( true );
	}

	if ( OD::ctrlKeyboardButton(eventinfo.buttonstate_) )
	    eraseSeeds( eventinfo );
	else
	    paintSeeds( eventinfo, *prevev_ );

	prevev_ = new visBase::EventInfo( eventinfo );
	mReturnHandled( true );
    }

    mReturnHandled( false );
}


static const MultiTextureSurveyObject* getSectionDisplay(
						const visBase::EventInfo& ev )
{
    const MultiTextureSurveyObject* so = nullptr;
    for ( int idx=0; idx<ev.pickedobjids.size(); idx++ )
    {
        mDynamicCast(const MultiTextureSurveyObject*,so,
		     visBase::DM().getObject(ev.pickedobjids[idx]))
        if ( so )
            break;
    }

    return so;
}


static float getTrcNrStretchPerZSample( const Scene& scene, float trcdist )
{
    float zstretch = scene.getFixedZStretch() * scene.getTempZStretch();
    if ( SI().zIsTime() )
	zstretch /= 2.f; // Account for TWT

    return (scene.getApparentVelocity(zstretch) * SI().zStep()) / trcdist;
}


void SeedPainter::paintSeeds( const visBase::EventInfo& curev,
			      const visBase::EventInfo& prevev )
{
    const MultiTextureSurveyObject* cursec = getSectionDisplay( curev );
    const MultiTextureSurveyObject* prevsec = getSectionDisplay( prevev );
    if ( !cursec || cursec != prevsec )
	return;

    mDynamicCastGet(const Seis2DDisplay*,s2d,cursec)
    if ( s2d )
	paintSeedsOn2DLine( s2d, curev, prevev );

    mDynamicCastGet(const RandomTrackDisplay*,rtd,cursec)
    if ( rtd )
	paintSeedsOnRandLine( rtd, curev, prevev );

    mDynamicCastGet(const PlaneDataDisplay*,pdd,cursec)
    if ( !pdd )
	return;

    const bool iszslice = pdd->getOrientation()==OD::ZSlice;
    const bool isinl = pdd->getOrientation()==OD::InlineSlice;
    const TrcKeyZSampling tkzs = pdd->getTrcKeyZSampling();

    if ( iszslice )
	paintSeedsOnZSlice( curev, prevev, tkzs );
    else
	paintSeedsOnInlCrl( curev, prevev, tkzs, isinl );
}


void SeedPainter::paintSeedsOnInlCrl( const visBase::EventInfo& curev,
				      const visBase::EventInfo& prevev,
				      const TrcKeyZSampling& tkzs, bool isinl )
{
    Scene* scene = STM().currentScene();
    if ( !scene )
	return;

    const float fac = getTrcNrStretchPerZSample( *scene, SI().crlDistance() );
    const StepInterval<int> nrrg = isinl ? tkzs.hsamp_.trcRange()
					 : tkzs.hsamp_.lineRange();
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
    const Stats::RandGen rgx, rgy;
    TypeSet<Pick::Location> mylocs;
    TypeSet<int> indexes;
    int locidx = set_->size();

#define mAddPosOnInlCrl(mynr,sampidx) \
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
	    mAddPosOnInlCrl( nr1, sampidx1 )

	if ( !pt2incircle )
	    continue;

	const int xdiff = ptidx2 + nrdiff;
	const int ydiff = ptidy2 + sampdiff;
	const bool pt2incircle1 =
	    (radius_*radius_) >= (xdiff*xdiff + ydiff*ydiff);
	if ( !pt2incircle1 )
	    mAddPosOnInlCrl( nr2, sampidx2 );
    }

    if ( mylocs.isEmpty() )
	return;

    set_->bulkAppendWithUndo( mylocs, indexes );
    Pick::SetMgr::BulkChangeData cd( Pick::SetMgr::BulkChangeData::Added,
				     set_, indexes );
    picksetmgr_->reportBulkChange( 0, cd );
}


void SeedPainter::paintSeedsOnZSlice( const visBase::EventInfo& curev,
				      const visBase::EventInfo& prevev,
				      const TrcKeyZSampling& tkzs )
{
    Scene* scene = STM().currentScene();
    if ( !scene )
	return;

    const float inlfac = getTrcNrStretchPerZSample( *scene, SI().inlDistance());
    const float crlfac = getTrcNrStretchPerZSample( *scene, SI().crlDistance());
    const StepInterval<int> inlrg = tkzs.hsamp_.lineRange();
    const StepInterval<int> crlrg = tkzs.hsamp_.trcRange();
    const Coord3 curpos = curev.worldpickedpos;
    const Coord3 prevpos = prevev.worldpickedpos;
    const bool fillprev = prevev.type == visBase::MouseClick;
    const BinID curbid = SI().transform( curpos );
    const BinID prevbid = SI().transform( prevpos );
    int inldiff = curbid.inl() - prevbid.inl();
    int crldiff = curbid.crl() - prevbid.crl();
    inldiff = mNINT32(inldiff/inlfac);
    crldiff = mNINT32(crldiff/crlfac);

    const int nrpts = mNINT32(radius_ * radius_ * density() / 100) + 1;
    const int dia = radius_ * 2 + 1;
    const Stats::RandGen rgx, rgy;
    TypeSet<Pick::Location> mylocs;
    TypeSet<int> indexes;
    int locidx = set_->size();

#define mAddPosOnZSlice(inl,crl) \
    { \
	const BinID bid( inl, crl ); \
	const float z = tkzs.zsamp_.start; \
	if ( tkzs.hsamp_.includes(bid,true) ) \
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
	const int inl1 = mNINT32(inlfac*ptidx1) + prevbid.inl();
	const int inl2 = mNINT32(inlfac*ptidx2) + curbid.inl();
	const int crl1 = mNINT32(crlfac*ptidy1) + prevbid.crl();
	const int crl2 = mNINT32(crlfac*ptidy2) + curbid.crl();
	if ( fillprev && pt1incircle )
	    mAddPosOnZSlice( inl1, crl1 )

	if ( !pt2incircle )
	    continue;

	const int xdiff = ptidx2 + inldiff;
	const int ydiff = ptidy2 + crldiff;
	const bool pt2incircle1 =
	    (radius_*radius_) >= (xdiff*xdiff + ydiff*ydiff);
	if ( !pt2incircle1 )
	    mAddPosOnZSlice( inl2, crl2 );
    }

    if ( mylocs.isEmpty() )
	return;

    set_->bulkAppendWithUndo( mylocs, indexes );
    Pick::SetMgr::BulkChangeData cd( Pick::SetMgr::BulkChangeData::Added,
				     set_, indexes );
    picksetmgr_->reportBulkChange( 0, cd );
}


void SeedPainter::paintSeedsOnRandLine( const RandomTrackDisplay* rtd,
					const visBase::EventInfo& curev,
					const visBase::EventInfo& prevev )
{
    Scene* scene = STM().currentScene();
    if ( !scene )
	return;

    const Coord3 curpos = curev.worldpickedpos;
    const BinID curbid = SI().transform( curpos );
    const TypeSet<BinID>* path = rtd->getPath();
    const int bididx = path->indexOf( curbid );
    if ( bididx < 1 || bididx > path->size()-2 )
	return;

    const Coord posm = SI().transform( path->get(bididx-1) );
    const Coord posp = SI().transform( path->get(bididx+1) );
    const float trcdist = 0.5 * ( curpos.coord().distTo(posm) +
				  curpos.coord().distTo(posp) );
    const float fac = getTrcNrStretchPerZSample( *scene, trcdist );

    const Coord3 prevpos = prevev.worldpickedpos;
    const BinID prevbid = SI().transform( prevpos );
    const bool fillprev = prevev.type == visBase::MouseClick;
    const int prevbididx = path->indexOf( prevbid );
    if ( prevbididx < 1 || prevbididx > path->size()-2 )
	return;

    const int nrdiff = (bididx - prevbididx) / fac;
    const int cursampidx = SI().zRange().nearestIndex( curpos.z );
    const int prevsampidx = SI().zRange().nearestIndex( prevpos.z );
    const int sampdiff = cursampidx - prevsampidx;

    const int nrpts = mNINT32(radius_ * radius_ * density() / 100) + 1;
    const int dia = radius_ * 2 + 1;
    const Stats::RandGen rgx, rgy;
    TypeSet<Pick::Location> mylocs;
    TypeSet<int> indexes;
    int locidx = set_->size();

#define mAddPosOnRandLine(bidx,sampidx) \
    { \
	const BinID bid = path->get( bidx ); \
	const float z = SI().zRange().atIndex( sampidx ); \
	if ( zrg.includes(z,false) ) \
	{ \
	    const Coord mypos = SI().transform( bid ); \
	    Pick::Location myloc( mypos, z ); \
	    mylocs.add( myloc ); \
	    indexes += locidx++; \
	} \
    }

    const Interval<float> zrg = rtd->getDepthInterval();
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
	const int bididx1 = mNINT32(fac*ptidx1) + prevbididx;
	const int bididx2 = mNINT32(fac*ptidx2) + bididx;
	const int sampidx1 = ptidy1 + prevsampidx;
	const int sampidx2 = ptidy2 + cursampidx;
	if ( fillprev && pt1incircle && path->validIdx(bididx1) )
	    mAddPosOnRandLine( bididx1, sampidx1 )

	if ( !pt2incircle )
	    continue;

	const int xdiff = ptidx2 + nrdiff;
	const int ydiff = ptidy2 + sampdiff;
	const bool pt2incircle1 =
	    (radius_*radius_) >= (xdiff*xdiff + ydiff*ydiff);
	if ( !pt2incircle1 && path->validIdx(bididx2) )
	    mAddPosOnRandLine( bididx2, sampidx2 );
    }

    if ( mylocs.isEmpty() )
	return;

    set_->bulkAppendWithUndo( mylocs, indexes );
    Pick::SetMgr::BulkChangeData cd( Pick::SetMgr::BulkChangeData::Added,
				     set_, indexes );
    picksetmgr_->reportBulkChange( 0, cd );

}


void SeedPainter::paintSeedsOn2DLine( const Seis2DDisplay* s2d,
					const visBase::EventInfo& curev,
					const visBase::EventInfo& prevev )
{
    Scene* scene = STM().currentScene();
    if ( !scene )
	return;

    const Pos::GeomID geomid = s2d->getGeomID();
    const Survey::Geometry* geom = Survey::GM().getGeometry( geomid );
    const Survey::Geometry2D* geom2d = geom ? geom->as2D() : nullptr;
    if ( !geom2d )
	return;

    const Coord3 curpos = curev.worldpickedpos;
    const int curtrcnr = s2d->getNearestTraceNr( curpos );
    const StepInterval<float> zrg = s2d->getZRange( false );
    StepInterval<int> trcrg = s2d->getMaxTraceNrRange();
    trcrg.limitTo( s2d->getTraceNrRange() );
    const int nrtrcs = trcrg.nrSteps() + 1;
    const int trcidx = trcrg.getIndex( curtrcnr );
    if ( trcidx < 1 )
	return;

    const float fac =
		getTrcNrStretchPerZSample( *scene, geom2d->averageTrcDist() );
    const Coord3 prevpos = prevev.worldpickedpos;
    const int prevtrcnr = s2d->getNearestTraceNr( prevpos );
    const bool fillprev = prevev.type == visBase::MouseClick;
    const int prevtrcidx = trcrg.getIndex( prevtrcnr );
    if ( prevtrcidx < 1 )
	return;

    const int nrdiff = (trcidx - prevtrcidx) / fac;
    const int cursampidx = SI().zRange().nearestIndex( curpos.z );
    const int prevsampidx = SI().zRange().nearestIndex( prevpos.z );
    const int sampdiff = cursampidx - prevsampidx;

    const int nrpts = mNINT32(radius_ * radius_ * density() / 100) + 1;
    const int dia = radius_ * 2 + 1;
    const Stats::RandGen rgx, rgy;
    TypeSet<Pick::Location> mylocs;
    TypeSet<int> indexes;
    int locidx = set_->size();
    const PosInfo::Line2DData& l2d = geom2d->data();
    PosInfo::Line2DPos l2dpos;

#define mAddPosOn2DLine(trcidx,sampidx) \
    { \
	const TrcKey tk( geomid, trcrg.atIndex(trcidx) ); \
	const float z = SI().zRange().atIndex( sampidx ); \
	if ( zrg.includes(z,false) && l2d.getPos(tk.trcNr(),l2dpos) ) \
	{ \
	    Pick::Location myloc( l2dpos.coord_, z ); \
	    myloc.setTrcKey( tk ); \
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
	const int trcidx1 = mNINT32(fac*ptidx1) + prevtrcidx;
	const int trcidx2 = mNINT32(fac*ptidx2) + trcidx;
	const int sampidx1 = ptidy1 + prevsampidx;
	const int sampidx2 = ptidy2 + cursampidx;
	if ( fillprev && pt1incircle && trcidx1>=0 && trcidx1<nrtrcs )
	    mAddPosOn2DLine( trcidx1, sampidx1 )

	if ( !pt2incircle )
	    continue;

	const int xdiff = ptidx2 + nrdiff;
	const int ydiff = ptidy2 + sampdiff;
	const bool pt2incircle1 =
	    (radius_*radius_) >= (xdiff*xdiff + ydiff*ydiff);
	if ( !pt2incircle1 && trcidx1>=0 && trcidx1<nrtrcs )
	    mAddPosOn2DLine( trcidx2, sampidx2 );
    }

    if ( mylocs.isEmpty() )
	return;

    set_->bulkAppendWithUndo( mylocs, indexes );
    Pick::SetMgr::BulkChangeData cd( Pick::SetMgr::BulkChangeData::Added,
				     set_, indexes );
    picksetmgr_->reportBulkChange( 0, cd );

}


void SeedPainter::eraseSeeds( const visBase::EventInfo& curev )
{
    Scene* scene = STM().currentScene();
    const MultiTextureSurveyObject* cursec = getSectionDisplay( curev );
    if ( !scene || !cursec )
	return;

    mDynamicCastGet(const Seis2DDisplay*,s2d,cursec)
    if ( s2d )
	eraseSeedsOn2DLine( s2d, curev );

    mDynamicCastGet(const RandomTrackDisplay*,rtd,cursec)
    if ( rtd )
	eraseSeedsOnRandLine( rtd, curev );

    mDynamicCastGet(const PlaneDataDisplay*,pdd,cursec)
    if ( !pdd )
	return;

    const TrcKeyZSampling tkzs = pdd->getTrcKeyZSampling();
    const bool isinl = pdd->getOrientation()==OD::InlineSlice;
    const bool isz = pdd->getOrientation()==OD::ZSlice;

    const float inlfac = getTrcNrStretchPerZSample( *scene, SI().inlDistance());
    const float crlfac = getTrcNrStretchPerZSample( *scene, SI().crlDistance());
    const Coord3 curpos = curev.worldpickedpos;
    const BinID curbid = SI().transform( curpos );

    TypeSet<Pick::Location> mylocs;
    TypeSet<int> indexes;

    for ( int idx=0; idx<set_->size(); idx++ )
    {
	const Pick::Location& loc = set_->get( idx );
	const BinID bid = SI().transform( loc.pos() );
	if ( !tkzs.hsamp_.includes(bid) ||
		!tkzs.zsamp_.includes(loc.pos().z,false) )
	    continue;

	const int xdiff = Math::Abs( isinl ? (curbid.crl() - bid.crl()) / crlfac
				      : (curbid.inl() - bid.inl()) / inlfac );
	const int ydiff =
	    isz ? Math::Abs( curbid.crl() - bid.crl() ) / crlfac
		: Math::Abs( loc.pos().z - curpos.z ) / SI().zStep();
	float distsq = xdiff*xdiff + ydiff*ydiff;
	if ( distsq > radius_*radius_ )
	    continue;

	mylocs.add( loc );
	indexes.add( idx );
    }

    if ( mylocs.isEmpty() )
	return;

    set_->bulkRemoveWithUndo( mylocs, indexes );
    Pick::SetMgr::BulkChangeData cd( Pick::SetMgr::BulkChangeData::ToBeRemoved,
				     set_, indexes );
    picksetmgr_->reportBulkChange( 0, cd );
}


void SeedPainter::eraseSeedsOnRandLine( const RandomTrackDisplay* rtd,
					const visBase::EventInfo& curev )
{
    Scene* scene = STM().currentScene();

    const Coord3 curpos = curev.worldpickedpos;
    const BinID curbid = SI().transform( curpos );
    const TypeSet<BinID>* path = rtd->getPath();
    const Interval<float> zrg = rtd->getDepthInterval();
    const int bididx = path->indexOf( curbid );
    if ( bididx < 1 || bididx > path->size()-2 )
	return;

    const Coord posm = SI().transform( path->get(bididx-1) );
    const Coord posp = SI().transform( path->get(bididx+1) );
    const float trcdist = 0.5 * ( curpos.coord().distTo(posm) +
				  curpos.coord().distTo(posp) );
    const float fac = getTrcNrStretchPerZSample( *scene, trcdist );

    TypeSet<Pick::Location> mylocs;
    TypeSet<int> indexes;

    for ( int idx=0; idx<set_->size(); idx++ )
    {
	const Pick::Location& loc = set_->get( idx );
	const BinID bid = SI().transform( loc.pos() );
	const int bidx = path->indexOf( bid );
	if ( bidx < 0 || !zrg.includes(loc.pos().z,false) )
	    continue;

	const int xdiff = Math::Abs( (bidx - bididx) / fac );
	const int ydiff = Math::Abs( loc.pos().z - curpos.z ) / SI().zStep();
	float distsq = xdiff*xdiff + ydiff*ydiff;
	if ( distsq > radius_*radius_ )
	    continue;

	mylocs.add( loc );
	indexes.add( idx );
    }

    if ( mylocs.isEmpty() )
	return;

    set_->bulkRemoveWithUndo( mylocs, indexes );
    Pick::SetMgr::BulkChangeData cd( Pick::SetMgr::BulkChangeData::ToBeRemoved,
				     set_, indexes );
    picksetmgr_->reportBulkChange( 0, cd );
}


void SeedPainter::eraseSeedsOn2DLine( const Seis2DDisplay* s2d,
				      const visBase::EventInfo& curev )
{
    Scene* scene = STM().currentScene();
    const Pos::GeomID geomid = s2d->getGeomID();
    const Survey::Geometry* geom = Survey::GM().getGeometry( geomid );
    const Survey::Geometry2D* geom2d = geom ? geom->as2D() : nullptr;
    if ( !geom2d )
	return;

    const Coord3 curpos = curev.worldpickedpos;
    const int curtrcnr = s2d->getNearestTraceNr( curpos );
    const StepInterval<float> zrg = s2d->getZRange( false );
    StepInterval<int> trcrg = s2d->getMaxTraceNrRange();
    trcrg.limitTo( s2d->getTraceNrRange() );
    const int trcidx = trcrg.getIndex( curtrcnr );
    if ( trcidx < 1 )
	return;

    const float fac =
		getTrcNrStretchPerZSample( *scene, geom2d->averageTrcDist() );
    TypeSet<Pick::Location> mylocs;
    TypeSet<int> indexes;
    for ( int idx=0; idx<set_->size(); idx++ )
    {
	const Pick::Location& loc = set_->get( idx );
	const TrcKey tk = loc.trcKey();
	if ( !tk.is2D() || tk.geomID() != geomid )
	    continue;

	const int loctrcidx = trcrg.getIndex( tk.trcNr() );
	if ( loctrcidx < 0 || !zrg.includes(loc.pos().z,false) )
	    continue;

	const int xdiff = (loctrcidx - trcidx) / fac;
	const int ydiff = (loc.pos().z - curpos.z) / SI().zStep();
	float distsq = xdiff*xdiff + ydiff*ydiff;
	if ( distsq > radius_*radius_ )
	    continue;

	mylocs.add( loc );
	indexes.add( idx );
    }

    if ( mylocs.isEmpty() )
	return;

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
    const MultiTextureSurveyObject* so = getSectionDisplay( eventinfo );
    if ( !so || !scene )
	return;

    mDynamicCastGet(const Seis2DDisplay*,s2d,so)
    if ( s2d )
	drawLineOn2DLine( s2d, eventinfo );

    mDynamicCastGet(const RandomTrackDisplay*,rtd,so)
    if ( rtd )
	drawLineOnRandLine( rtd, eventinfo );

    mDynamicCastGet(const PlaneDataDisplay*,pdd,so)
    if ( !pdd )
	return;

    Coord3 pickedpos = eventinfo.worldpickedpos;
    BinID pickedbid = SI().transform( pickedpos );
    const TrcKeyZSampling tkzs = pdd->getTrcKeyZSampling();
    const bool isinl = pdd->getOrientation()==OD::InlineSlice;
    const bool isz = pdd->getOrientation()==OD::ZSlice;

    if ( circlecoords_.isEmpty() )
	mkCircle();

    const float inlfac = isinl ? 0
		: getTrcNrStretchPerZSample( *scene, SI().inlDistance() );
    const float crlfac = (isinl || isz) ?
		getTrcNrStretchPerZSample( *scene, SI().crlDistance() ) : 0;
    for ( int idx=0; idx<circlecoords_.size(); idx++ )
    {
	const BinID bid( pickedbid.inl() + mNINT32(inlfac*circlecoords_[idx].x),
			 pickedbid.crl() + mNINT32(crlfac*
			 (isz? circlecoords_[idx].y : circlecoords_[idx].x)) );
	const Coord pt = SI().transform( bid );
	circle_->addPoint( Coord3(pt,isz ? pickedpos.z
			: (pickedpos.z+circlecoords_[idx].y*SI().zStep())) );
    }

    circle_->dirtyCoordinates();
}


void SeedPainter::drawLineOnRandLine( const RandomTrackDisplay* rtd,
				      const visBase::EventInfo& eventinfo )
{
    Scene* scene = STM().currentScene();
    const Coord3 pickedpos = eventinfo.worldpickedpos;
    const BinID pickedbid = SI().transform( pickedpos );
    const TypeSet<BinID>* path = rtd->getPath();
    const int bididx = path->indexOf( pickedbid );
    if ( bididx < 1 || bididx > path->size()-2 )
	return;

    if ( circlecoords_.isEmpty() )
	mkCircle();

    const Coord posm = SI().transform( path->get(bididx-1) );
    const Coord posp = SI().transform( path->get(bididx+1) );
    const float trcdist = 0.5 * ( pickedpos.coord().distTo(posm) +
				  pickedpos.coord().distTo(posp) );
    const float fac = getTrcNrStretchPerZSample( *scene, trcdist );
    for ( int idx=0; idx<circlecoords_.size(); idx++ )
    {
	const int posidx = bididx + mNINT32( fac * circlecoords_[idx].x );
	if ( !path->validIdx(posidx) )
	    continue;

	const BinID bid = path->get( posidx );
	const Coord pt = SI().transform( bid );
	circle_->addPoint( Coord3(pt,
			pickedpos.z+circlecoords_[idx].y*SI().zStep()) );
    }

    circle_->dirtyCoordinates();
}


void SeedPainter::drawLineOn2DLine( const Seis2DDisplay* s2d,
				    const visBase::EventInfo& eventinfo )
{
    Scene* scene = STM().currentScene();
    const Pos::GeomID geomid = s2d->getGeomID();
    const Survey::Geometry* geom = Survey::GM().getGeometry( geomid );
    const Survey::Geometry2D* geom2d = geom ? geom->as2D() : nullptr;
    if ( !geom2d )
	return;

    const Coord3 pickedpos = eventinfo.worldpickedpos;
    const int pickedtrcnr = s2d->getNearestTraceNr( pickedpos );
    StepInterval<int> trcrg = s2d->getMaxTraceNrRange();
    trcrg.limitTo( s2d->getTraceNrRange() );
    const int nrtrcs = trcrg.nrSteps() + 1;
    const int trcidx = trcrg.getIndex( pickedtrcnr );
    if ( trcidx < 1 )
	return;

    if ( circlecoords_.isEmpty() )
	mkCircle();

    const float fac =
		getTrcNrStretchPerZSample( *scene, geom2d->averageTrcDist() );
    const PosInfo::Line2DData& l2d = geom2d->data();
    PosInfo::Line2DPos l2dpos;
    for ( int idx=0; idx<circlecoords_.size(); idx++ )
    {
	const int posidx = trcidx + mNINT32( fac * circlecoords_[idx].x );
	if ( posidx<0 || posidx>=nrtrcs )
	    continue;

	const int trcnr = trcrg.atIndex( posidx );
	if ( !l2d.getPos(trcnr,l2dpos) )
	    continue;

	circle_->addPoint( Coord3(l2dpos.coord_,
			pickedpos.z+circlecoords_[idx].y*SI().zStep()) );
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

} // namespace visSurvey
