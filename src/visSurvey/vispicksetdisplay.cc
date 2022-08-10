/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2002
-*/


#include "vispicksetdisplay.h"

#include "callback.h"
#include "color.h"
#include "commondefs.h"
#include "draw.h"
#include "mousecursor.h"
#include "pickset.h"
#include "zaxistransform.h"

#include "visemobjdisplay.h"
#include "vismarkerset.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "visrandompos2body.h"
#include "visevent.h"
#include "vistransform.h"
#include "visdragger.h"
#include "visplanedatadisplay.h"
#include "visseis2ddisplay.h"
#include "vispolygonselection.h"

static float cDipFactor() { return SI().zIsTime() ? 1e-6f : 1e-3f; }

namespace visSurvey {

const char* PickSetDisplay::sKeyNrPicks()       { return "No Picks"; }
const char* PickSetDisplay::sKeyPickPrefix()    { return "Pick "; }
const char* PickSetDisplay::sKeyDisplayBody()	{ return "Show Body"; }

#define mCheckReadyOnly(set) { if (set->isReadOnly()) return; }

PickSetDisplay::PickSetDisplay()
    : LocationDisplay()
    , markerset_(visBase::MarkerSet::create())
    , needline_(false)
    , bodydisplay_(nullptr)
    , shoulddisplaybody_( false )
    , dragger_(nullptr)
    , draggeridx_(-1)
    , showdragger_(false)
    , color_(OD::Color::White())
    , polylines_(nullptr)
{
    markerset_->ref();
    markerset_->applyRotationToAllMarkers( false );
    addChild( markerset_->osgNode() );
    pickselstatus_.setEmpty();
}


PickSetDisplay::~PickSetDisplay()
{
    if ( dragger_ )
    {
	dragger_->unRef();
	dragger_ = nullptr;
    }

    unRefAndZeroPtr( bodydisplay_ );
    removeChild( markerset_->osgNode() );
    unRefAndZeroPtr( markerset_ );

    if ( polylines_ )
    {
	removeChild( polylines_->osgNode() );
	unRefAndZeroPtr( polylines_ );
    }

    Pick::SetMgr& mgr = Pick::Mgr();
    mgr.undo().removeAll();
}


void PickSetDisplay::setSet( Pick::Set* newset )
{
    if ( !newset )
	return;

    LocationDisplay::setSet( newset );
    if ( set_->isSizeLargerThanThreshold() )
    {
	set_->disp_.markertype_ = MarkerStyle3D::Point;
	set_->disp_.pixsize_ = 2;
    }

    MarkerStyle3D markerstyle;
    markerstyle.size_ = set_->disp_.pixsize_;
    markerstyle.type_ = (MarkerStyle3D::Type) set_->disp_.markertype_;
    markerset_->setMaterial( nullptr );
    markerset_->setMarkerStyle( markerstyle );
    markerset_->setMarkersSingleColor( set_->disp_.color_ );

    OD::LineStyle ls = set_->disp_.linestyle_;
    if ( polylines_ )
	polylines_->setLineStyle( ls );

    if ( !showall_ && scene_ )
	scene_->objectMoved( nullptr );

    dragger_= visBase::Dragger::create();
    dragger_->ref();
    dragger_->setDisplayTransformation( transformation_ );
    dragger_->setDraggerType( visBase::Dragger::Translate2D );
    dragger_->setSize( (float)markerstyle.size_ );
    dragger_->setOwnShape( createOneMarker(), false );
    addChild( dragger_->osgNode() );
    dragger_->turnOn( false );

    displayBody( set_->disp_.dofill_ );
}


void PickSetDisplay::updateDragger()
{
    mCheckReadyOnly( set_ )

    if ( dragger_ )
    dragger_->updateDragger( false );
}


bool PickSetDisplay::draggerNormal() const
{ return dragger_ ? !dragger_->defaultRotation() : false; }


void PickSetDisplay::setDraggerNormal( const Coord3& normal )
{
    if ( normal.isUdf() )
	return;
    const Coord3 defnormal( 0, 0, 1 );
    const float dotproduct = (float)defnormal.dot( normal );
    Coord3 rotationaxis(0,0,1);
    float angle = 0;
    if ( !mIsEqual(dotproduct,1,1e-3) )
    {
	const float zaxisangle =
	    sCast( float, Math::Atan2(normal.y,normal.x) );
	Quaternion rotation( defnormal, zaxisangle );
	rotation *= Quaternion( Coord3(0,1,0), -Math::ACos(dotproduct) );
	rotation.getRotation( rotationaxis, angle );
    }

    dragger_->setRotation( rotationaxis, angle );
}


bool PickSetDisplay::isPolygon() const
{ return set_ ? set_->isPolygon() : false; }


void PickSetDisplay::locChg( CallBacker* cb )
{
    LocationDisplay::locChg( cb );
    setBodyDisplay();
    if ( markerset_ )
	markerset_->forceRedraw( true );
}


void PickSetDisplay::dispChg( CallBacker* cb )
{
    if ( !markerset_ )
	return;

    const int oldpixsz = (int)(markerset_->getScreenSize() + .5);
    if ( oldpixsz != set_->disp_.pixsize_ )
    {
	updateLineStyle();

	markerset_->setScreenSize(  sCast(float,set_->disp_.pixsize_) );
    }

    if ( markerset_->getType() != set_->disp_.markertype_ )
    {
	markerset_->setType( (MarkerStyle3D::Type)set_->disp_.markertype_ );
	if ( set_->disp_.markertype_ == MarkerStyle3D::Arrow
		|| set_->disp_.markertype_ == MarkerStyle3D::Plane )
	    fullRedraw(nullptr);
    }

    if ( bodydisplay_ &&
	    bodydisplay_->getMaterial()->getColor() != set_->disp_.fillcolor_ )
    {
	bodydisplay_->getMaterial()->setColor( set_->disp_.fillcolor_ );
    }

    LocationDisplay::dispChg( cb );
    markerset_->setMarkersSingleColor( set_->disp_.color_  );
    markerset_->forceRedraw( true );

    showLine( needLine() );

    displayBody( set_->disp_.dofill_ );
}


void PickSetDisplay::updateLineStyle()
{
    if ( !polylines_ || !set_ )
	return;

    polylines_->setLineStyle( set_->disp_.linestyle_ );
}


void PickSetDisplay::setPosition( int idx, const Pick::Location& loc )
{
    setPosition( idx, loc, false );
}


void PickSetDisplay::setPosition( int idx, const Pick::Location& loc, bool add )
{

    if ( add )
	markerset_->insertPos( idx, loc.pos(), true );
    else
	markerset_->setPos( idx, loc.pos(), true );
    if ( set_->disp_.markertype_ == MarkerStyle3D::Arrow ||
	 set_->disp_.markertype_ == MarkerStyle3D::Plane )
	markerset_->setSingleMarkerRotation( getDirection(loc), idx );

    if ( needLine() )
	setPolylinePos( idx, loc.pos() );
}


Coord3 PickSetDisplay::getPosition( int loc ) const
{
    const visBase::Coordinates* markercoords = markerset_->getCoordinates();
    if( markercoords->size() )
	 return markercoords->getPos( loc );
    return Coord3::udf();
}


void PickSetDisplay::setPolylinePos( int idx, const Coord3& pos )
{
    if ( !polylines_ )
	createLine();

    if ( set_->nrSets() > 1 )
	return;

     if ( set_->disp_.connect_==Pick::Set::Disp::Close )
     {
	redrawLine();
	return;
     }

    polylines_->setPoint( idx, pos );
    polylines_->dirtyCoordinates();
}


void PickSetDisplay::removePosition( int idx )
{
    mCheckReadyOnly( set_ )

    if ( set_->disp_.connect_ == Pick::Set::Disp::Close )
    {
	redrawAll( idx );
	return;
    }

    if ( !markerset_ || idx>=markerset_->size() )
	return;

    markerset_->removeMarker( idx );
    removePolylinePos( idx );

    dragger_->turnOn( false );
}


void PickSetDisplay::removePolylinePos( int idx )
{
    mCheckReadyOnly( set_ )
    if ( !polylines_ || idx>polylines_->size() )
	return;

    if ( set_->disp_.connect_==Pick::Set::Disp::Close )
    {
	redrawLine();
	return;
    }

    polylines_->removePoint( idx );

    if ( set_->disp_.connect_==Pick::Set::Disp::Close )
	redrawLine();
}


void PickSetDisplay::redrawMultiSets()
{
    if ( !markerset_ )
	return;

    if ( !polylines_ && needLine() )
	createLine();

    markerset_->clearMarkers();
    if ( polylines_ )
    {
	polylines_->removeAllPrimitiveSets();
	polylines_->getCoordinates()->setEmpty();
    }

    if ( set_->nrSets()==0 && set_->disp_.connect_!=Pick::Set::Disp::None )
    {
	// this is for a default new polygon, we should have a default start
	// index. it should be changed in trunk version.
	set_->addStartIdx( 0 );
    }

    for ( int idx=0; idx<set_->nrSets(); idx++ )
    {
	int start=0, stop=0;
	set_->getStartStopIdx( idx, start, stop );
	TypeSet<int> ps;
	bool first = true;
	int firstidx = 0;
	for ( int idy=start; idy<=stop; idy++ )
	{
	    Coord3 pos = set_->getPos( idy );
	    if ( datatransform_ )
		pos.z = datatransform_->transform( pos );
	    if ( mIsUdf(pos.z) )
		continue;

	    markerset_->addPos( pos );
	    if ( !polylines_ )
		continue;

	    ps += polylines_->getCoordinates()->addPos( pos );
	    if ( first )
	    {
		firstidx = ps[ps.size()-1];
		first = false;
	    }
	}

	if ( set_->disp_.connect_==Pick::Set::Disp::Close )
	    ps += firstidx;
	if ( ps.isEmpty() )
	    continue;

	Geometry::IndexedPrimitiveSet* lineprimitiveset =
	    Geometry::IndexedPrimitiveSet::create( false );
	lineprimitiveset->ref();
	lineprimitiveset->set( ps.arr(), ps.size() );
	polylines_->addPrimitiveSet( lineprimitiveset );
    }
}


void PickSetDisplay::redrawAll( int drageridx )
{
    if ( !markerset_ )
	return;

    if ( !polylines_ && needLine() )
	createLine();

    markerset_->clearMarkers();
    for ( int idx=0; idx<set_->size(); idx++ )
    {
	Coord3 pos = set_->getPos( idx );
	if ( datatransform_ )
	    pos.z = datatransform_->transform( pos );
	if ( !mIsUdf(pos.z) )
	    markerset_->addPos( pos );
    }

    markerset_->forceRedraw( true );
    redrawLine();
}


void PickSetDisplay::removeAll()
{
    if ( markerset_ )
    {
	markerset_->clearMarkers();
	markerset_->forceRedraw( true );
    }

    if ( polylines_ )
    {
	polylines_->removeAllPrimitiveSets();
	polylines_->getCoordinates()->setEmpty();
	removeChild( polylines_->osgNode() );
	unRefAndZeroPtr( polylines_ );
    }
}


void PickSetDisplay::createLine()
{
     if ( polylines_ || !set_ )
	return;

    polylines_ = visBase::PolyLine::create();
    polylines_->ref();

    addChild( polylines_->osgNode() );
    polylines_->setDisplayTransformation( transformation_ );
    polylines_->setMaterial( new visBase::Material() );

    updateLineStyle();
}


void PickSetDisplay::redrawLine()
{
    if ( !polylines_ || !set_ )
	return;

    polylines_->setLineStyle( set_->disp_.linestyle_ );

    if ( set_->nrSets() > 1 )
	return;

    polylines_->removeAllPoints();

    int idx=0;
    for ( ; idx<set_->size(); idx++ )
    {
	Coord3 pos = set_->getPos( idx );
	if ( datatransform_ )
	    pos.z = datatransform_->transform( pos );
	if ( !mIsUdf(pos.z) )
	    polylines_->addPoint( pos );
    }

    if ( idx && set_->disp_.connect_==Pick::Set::Disp::Close )
	polylines_->setPoint( idx, polylines_->getPoint(0) );

    polylines_->dirtyCoordinates();
}


bool PickSetDisplay::needLine()
{
    needline_ = set_->disp_.connect_ != Pick::Set::Disp::None;
    return needline_;
}


void PickSetDisplay::showLine( bool yn )
{
    if ( !needLine() && !lineShown() )
	return;

    if ( !polylines_ )
	createLine();

    redrawLine();
    polylines_->turnOn( yn );
}


bool PickSetDisplay::lineShown() const
{
    return polylines_ ? polylines_->isOn() : false;
}


void PickSetDisplay::showDragger( bool yn )
{ showdragger_ = yn; }

bool PickSetDisplay::draggerShown() const
{ return showdragger_; }


static float getSurveyRotation()
{
    const Pos::IdxPair2Coord& b2c = SI().binID2Coord();
    const float xcrd = (float) b2c.getTransform(true).c;
    const float ycrd = (float) b2c.getTransform(false).c;
    const float angle = Math::Atan2( ycrd, xcrd );
    return angle;
}


::Quaternion PickSetDisplay::getDirection( const Pick::Location& loc ) const
{
    const float survngle = getSurveyRotation();
    if ( set_->disp_.markertype_ == MarkerStyle3D::Arrow )
    {
	const float phi = SI().isRightHandSystem() ? survngle + loc.dir().phi
					     : survngle - loc.dir().phi;
	const Quaternion azimuth( Coord3(0,0,1), phi );
	const Quaternion dip( Coord3(0,1,0), loc.dir().theta );
	const Quaternion rot = azimuth * dip;
	return rot;
    }

    const float zscale = !scene_ ? 0 :
		    scene_->getApparentVelocity(scene_->getFixedZStretch());

    const float inldepth = (loc.inlDip()*cDipFactor()) * zscale;
    const float crldepth = (loc.crlDip()*cDipFactor()) * zscale;

    const float inlangle = atan( (SI().isRightHandSystem()
			? -inldepth
			: inldepth) );
    const float crlangle = atan( crldepth );

    const Quaternion inlrot( Coord3(1,0,0), inlangle );
    const Quaternion crlrot( Coord3(0,1,0), crlangle );
    const Quaternion survrot( Coord3(0,0,1),survngle );

    const Quaternion finalrot = survrot * crlrot * inlrot;
    return finalrot;
}


bool PickSetDisplay::isBodyDisplayed() const
{
    return bodydisplay_ && shoulddisplaybody_;
}


void PickSetDisplay::displayBody( bool yn )
{
    shoulddisplaybody_ = yn;
    if ( yn && !bodydisplay_ )
	setBodyDisplay();

    if ( bodydisplay_ )
	bodydisplay_->turnOn( yn );
}


bool PickSetDisplay::setBodyDisplay()
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );

    if ( !shoulddisplaybody_ || !set_ || !set_->size() )
	return false;

    if ( !bodydisplay_ )
    {
	bodydisplay_ = visBase::RandomPos2Body::create();
	bodydisplay_->ref();
	addChild( bodydisplay_->osgNode() );
	bodydisplay_->setPixelDensity( getPixelDensity() );
    }

    if ( !bodydisplay_->getMaterial() )
	bodydisplay_->setMaterial( new visBase::Material );
    bodydisplay_->getMaterial()->setColor( set_->disp_.fillcolor_ );
    bodydisplay_->setDisplayTransformation( transformation_ );

    TypeSet<Coord3> picks;
    for ( int idx=0; idx<set_->size(); idx++ )
    {
	picks += set_->getPos( idx );
	if ( datatransform_ )
	    picks[idx].z = datatransform_->transformBack( picks[idx] );
    }

    setColor( set_->disp_.fillcolor_ );

    return bodydisplay_->setPoints( picks, set_->isPolygon() );
}


visBase::MarkerSet* PickSetDisplay::createOneMarker() const
{
    visBase::MarkerSet* marker = visBase::MarkerSet::create();
    MarkerStyle3D markerstyle;
    markerstyle.size_ = set_->disp_.pixsize_;
    markerstyle.type_ = sCast(MarkerStyle3D::Type,set_->disp_.markertype_);
    marker->setMaterial(nullptr);
    marker->setMarkerStyle(markerstyle);
    marker->setMarkersSingleColor( OD::Color::NoColor() );
    marker->addPos( Coord3(0,0,0) );
    return marker;
}


int PickSetDisplay::clickedMarkerIndex( const visBase::EventInfo& evi ) const
{
    if ( !isMarkerClick( evi ) )
	return -1;

    return markerset_->findClosestMarker( evi.displaypickedpos, true );
}


bool PickSetDisplay::isMarkerClick( const visBase::EventInfo& evi ) const
{
    return evi.pickedobjids.isPresent( markerset_->id() );
}


void PickSetDisplay::otherObjectsMoved(
			const ObjectSet<const SurveyObject>& objs, VisID )
{
    mCheckReadyOnly(set_)

    if ( showall_ && invalidpicks_.isEmpty() )
	return;

    for ( int idx=0; idx<markerset_->getCoordinates()->size(); idx++ )
    {
	bool showmarker = false;
	if ( showall_ )
	    showmarker = true;
	else
	{
	    for ( int idy=0; idy<objs.size(); idy++ )
	    {
		if ( updateMarkerAtSection(objs[idy],idx) )
		{
		    showmarker = true;
		    break;
		}
	    }
	}

	if ( showmarker )
	    invalidpicks_ -= idx;
	else
	    invalidpicks_ += idx;

	markerset_->turnMarkerOn( idx, showmarker );
	markerset_->requestSingleRedraw();
    }

    updateLineAtSection();
}


bool PickSetDisplay::updateMarkerAtSection( const SurveyObject* obj, int idx )
{
    if ( !obj ) return false;

    Coord3 pos = set_->validIdx(idx) ? set_->getPos(idx) : Coord3::udf();
    if ( !pos.isDefined()) return false;

    if ( datatransform_ )
	pos.z = datatransform_->transform( pos );

    if ( scene_ )
	scene_->getUTM2DisplayTransform()->transform( pos );

    bool onsection = false;
    if ( !pos.isDefined() )
	return false;
    else if ( showall_ )
	onsection = true;
    else
    {
	mDynamicCastGet( const EMObjectDisplay*,emobj,obj )
	if ( emobj && emobj->displayedOnlyAtSections() )
	    return false;

	const float dist = obj->calcDist( pos );
	if ( dist<obj->maxDist() )
	    onsection = true;
    }

    return onsection;
}


void PickSetDisplay::updateLineAtSection()
{
   if ( polylines_ )
   {
       TypeSet<Coord3> polycoords;
       for ( int idx = 0; idx<markerset_->getCoordinates()->size(); idx++ )
	   polycoords += markerset_->getCoordinates()->getPos(idx);

	polylines_->removeAllPoints();
	int pidx = 0;
	for ( pidx=0; pidx<polycoords.size(); pidx++ )
	{
	    if ( markerset_->markerOn(pidx) )
		polylines_->addPoint( polycoords[pidx] );
	}

	if ( pidx && set_->disp_.connect_ == Pick::Set::Disp::Close )
	    polylines_->setPoint( pidx, polylines_->getPoint(0) );
    }
}


//
//void PickSetDisplay::setScene( Scene* scn )
//{
//    SurveyObject::setScene( scn );
//    if ( scene_ )
//	scene_->zstretchchange.notify(
//		mCB(this,PickSetDisplay,sceneZChangeCB) );
//}

//
//void PickSetDisplay::sceneZChangeCB( CallBacker* )
//{
//    for ( int idx=0; idx<group_->size(); idx++ )
//    {
//	mDynamicCastGet(visBase::MarkerSet*,markerset,group_->getObject(idx));
//	/*if ( markerset && scene_ )
// markerset->setZStretch( scene_->getZStretch()*scene_->getZScale()/2 );*/
//    }
//}


void PickSetDisplay::getPickingMessage( BufferString& str ) const
{
    float area = set_ ? set_->getXYArea() : mUdf(float);
    const bool hasarea = !mIsUdf(area) && area>0;
    BufferString areastring;

    if ( hasarea )
    {
	areastring.set( "Area: " )
		  .add( getAreaString(area,SI().xyInFeet(),2,false) );
    }

    str = isPainting() ? "Painting " : "Picking ";
    str += set_ ? set_->name() : "";
    str += " (Nr picks="; str += set_ ? set_->size() : 0;
    if ( !areastring.isEmpty() ) { str +=", "; str += areastring; }
    str += ")";
}


void PickSetDisplay::setColor( OD::Color nc )
{
    if ( set_ )
	set_->disp_.fillcolor_ = nc;

    if ( !bodydisplay_ ) return;

    if ( !bodydisplay_->getMaterial() )
	bodydisplay_->setMaterial( new visBase::Material );

    bodydisplay_->getMaterial()->setColor( nc );
}


void PickSetDisplay::setPixelDensity( float dpi )
{
    LocationDisplay::setPixelDensity( dpi );

    if ( markerset_ )
       markerset_->setPixelDensity( dpi );
    if ( bodydisplay_ )
	bodydisplay_->setPixelDensity( dpi );
    if ( polylines_ )
	polylines_->setPixelDensity( dpi );
}


float PickSetDisplay::getPixelDensity() const
{ return markerset_->getPixelDensity(); }


void PickSetDisplay::setDisplayTransformation( const mVisTrans* newtr )
{
    visSurvey::LocationDisplay::setDisplayTransformation( newtr );
    if ( bodydisplay_ )
	bodydisplay_->setDisplayTransformation( newtr );
    if ( markerset_ )
	markerset_->setDisplayTransformation( newtr );
    if ( polylines_ )
	polylines_->setDisplayTransformation( newtr );
    if ( dragger_ )
	dragger_->setDisplayTransformation( newtr );
}


const mVisTrans* PickSetDisplay::getDisplayTransformation() const
{
    return transformation_;
}


void PickSetDisplay::fillPar( IOPar& par ) const
{
    LocationDisplay::fillPar( par );
    par.setYN( sKeyDisplayBody(), shoulddisplaybody_ );
}


bool PickSetDisplay::usePar( const IOPar& par )
{
    if ( !visSurvey::LocationDisplay::usePar( par ) )
	 return false;

    bool showbody = false;
    par.getYN( sKeyDisplayBody(), showbody );
    displayBody( showbody );

    return true;
}


void PickSetDisplay::turnOnSelectionMode( bool yn )
{
    ctrldown_ = false;

    if ( scene_ && scene_->getPolySelection() )
    {
	if ( yn )
	{
	    mAttachCBIfNotAttached(
	    scene_->getPolySelection()->polygonFinished(),
	    PickSetDisplay::polygonFinishedCB );
	    selectionmodel_ = true;
	}
	else
	{
	    mDetachCB(
	    scene_->getPolySelection()->polygonFinished(),
	    PickSetDisplay::polygonFinishedCB );
	    selectionmodel_ = false;
	}
    }
}


void PickSetDisplay::polygonFinishedCB(CallBacker*)
{
    mCheckReadyOnly( set_ )

    if ( !scene_ || ! scene_->getPolySelection() )
	return;

    color_ = set_->disp_.color_;
    const int diff = markerset_->size()-pickselstatus_.size();
    if ( diff !=0 ) // added new pos or removed pos. reset
    {
	pickselstatus_.setSize( markerset_->size() );
	pickselstatus_.setAll( false );
    }

    visBase::PolygonSelection* polysel =  scene_->getPolySelection();
    MouseCursorChanger mousecursorchanger( MouseCursor::Wait );

    if ( (!polysel->hasPolygon() && !polysel->singleSelection()) )
    {
	unSelectAll();
	return;
    }

    if ( !ctrldown_ )
	unSelectAll();

    updateSelections( polysel );
    polysel->clear();

}


void PickSetDisplay::unSelectAll()
{
    markerset_->setMarkersSingleColor( color_ );
    pickselstatus_.setAll( false );
}


void PickSetDisplay::setPickSelect( int idx, bool yn )
{
    OD::Color clr = yn ? color_.complementaryColor() : color_;
    markerset_->getMaterial()->setColor( clr, idx, false );
    pickselstatus_[idx] = yn;
}


void PickSetDisplay::updateSelections(
    const visBase::PolygonSelection* polysel )
{
    if ( !markerset_ || !polysel || !polysel->hasPolygon() )
	return;

    const visBase::Coordinates* coords = markerset_->getCoordinates();
    if ( !coords||coords->size()==0 )
	return;

    for ( int idx=0; idx<markerset_->size(); idx++ )
    {
	const Coord3 pos = coords->getPos(idx);
	if ( !ctrldown_ )
	{
	    if ( !polysel->isInside(pos) )
		setPickSelect( idx, false );
	    else
		setPickSelect( idx, true );
	}
	else
	{
	    if ( polysel->isInside(pos) )
	    {
		if ( pickselstatus_[idx] )
		    setPickSelect( idx, false );
		else
		    setPickSelect( idx, true );
	    }
	}
    }

    markerset_->getMaterial()->change.trigger();
}


bool PickSetDisplay::removeSelections( TaskRunner* )
{
    MouseCursorChanger mousecursorchanger( MouseCursor::Wait );
    bool changed = false;
    for ( int idx=pickselstatus_.size()-1; idx>=0; idx-- )
    {
	if ( pickselstatus_[idx] )
	{
	    Pick::SetMgr::ChangeData cd(
		Pick::SetMgr::ChangeData::ToBeRemoved, set_,idx );
	    set_->removeSingleWithUndo( idx );
	    Pick::Mgr().reportChange( nullptr, cd );
	    changed = true;
	}
    }

    Pick::Mgr().undo().setUserInteractionEnd(
	    Pick::Mgr().undo().currentEventID() );

    unSelectAll();
    return changed;
}

} // namespace visSurvey
