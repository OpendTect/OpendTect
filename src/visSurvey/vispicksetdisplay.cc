/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2002
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "vispicksetdisplay.h"


#include "mousecursor.h"
#include "pickset.h"
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
#include "zaxistransform.h"
#include "callback.h"

static float cDipFactor() { return SI().zIsTime() ? 1e-6f : 1e-3f; }

namespace visSurvey {

const char* PickSetDisplay::sKeyNrPicks()       { return "No Picks"; }
const char* PickSetDisplay::sKeyPickPrefix()    { return "Pick "; }
const char* PickSetDisplay::sKeyDisplayBody()	{ return "Show Body"; }


PickSetDisplay::PickSetDisplay()
    : bodydisplay_(0)
    , markerset_(visBase::MarkerSet::create())
    , polyline_(0)
    , shoulddisplaybody_( false )
    , needline_(0)
    , dragger_(0)
    , draggeridx_(-1)
    , showdragger_(false)
    , color_(Color::White())
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
	dragger_ = 0;
    }

    unRefAndZeroPtr( bodydisplay_ );
    removeChild( markerset_->osgNode() );
    unRefAndZeroPtr( markerset_ );

    if ( polyline_ )
    {
	removeChild( polyline_->osgNode() );
	unRefAndZeroPtr( polyline_ );
    }

    Pick::SetMgr& mgr = Pick::Mgr();
    mgr.undo().removeAll();
}


void PickSetDisplay::setSet( Pick::Set* newset )
{
    if ( !newset )
	return;

    LocationDisplay::setSet( newset );

    MarkerStyle3D markerstyle;
    markerstyle.size_ = set_->disp_.pixsize_;
    markerstyle.type_ = (MarkerStyle3D::Type) set_->disp_.markertype_;
    markerset_->setMaterial( 0 );
    markerset_->setMarkerStyle( markerstyle );
    markerset_->setMarkersSingleColor( set_->disp_.color_ );

    if ( !showall_ && scene_ )
	scene_->objectMoved( 0 );

    dragger_= visBase::Dragger::create();
    dragger_->ref();
    dragger_->setDisplayTransformation( transformation_ );
    dragger_->setDraggerType( visBase::Dragger::Translate2D );
    dragger_->setSize( (float)markerstyle.size_ );
    dragger_->setOwnShape( createOneMarker(), false );
    addChild( dragger_->osgNode() );
    dragger_->turnOn( false );
}


void PickSetDisplay::updateDragger()
{
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
	    mCast( float, Math::Atan2(normal.y,normal.x) );
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
	if ( needLine() && polyline_ )
	{
	    LineStyle ls; ls.width_ = set_->disp_.pixsize_;
	    polyline_->setLineStyle( ls );
	}

	markerset_->setScreenSize(  mCast(float,set_->disp_.pixsize_) );
    }

    if ( markerset_->getType() != set_->disp_.markertype_ )
    {
	markerset_->setType( (MarkerStyle3D::Type)set_->disp_.markertype_ );
	if ( set_->disp_.markertype_ == MarkerStyle3D::Arrow
		|| set_->disp_.markertype_ == MarkerStyle3D::Plane )
	    fullRedraw(0);
    }

    LocationDisplay::dispChg( cb );
    markerset_->setMarkersSingleColor( set_->disp_.color_  );
    markerset_->forceRedraw( true );
    showLine( needLine() );
}


void PickSetDisplay::setPosition( int idx, const Pick::Location& loc )
{
    setPosition( idx, loc, false );
}


void PickSetDisplay::setPosition( int idx, const Pick::Location& loc, bool add )
{
    if ( set_->disp_.connect_ == Pick::Set::Disp::Close )
    {
	redrawAll( idx );
	return;
    }
    if ( add )
	markerset_->insertPos( idx, loc.pos_, true );
    else
	markerset_->setPos( idx, loc.pos_, true );
    if ( set_->disp_.markertype_ == MarkerStyle3D::Arrow ||
	 set_->disp_.markertype_ == MarkerStyle3D::Plane )
	markerset_->setSingleMarkerRotation( getDirection(loc), idx );

    if ( needLine() )
	setPolylinePos( idx, loc.pos_ );

    if ( loc.pos_.isDefined() && dragger_ )
    {
	dragger_->setPos( loc.pos_ );
	dragger_->turnOn( showdragger_ );
    }

    updateDragger();

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
    if ( !polyline_ )
	createLine();

    polyline_->setPoint( idx, pos );
    polyline_->dirtyCoordinates();
}


void PickSetDisplay::removePosition( int idx )
{
    if ( set_->disp_.connect_ == Pick::Set::Disp::Close )
    {
	redrawAll( idx );
	return;
    }

    if ( !markerset_ || idx>markerset_->size() )
	return;

    markerset_->removeMarker( idx );
    removePolylinePos( idx );

    dragger_->turnOn( false );
}


void PickSetDisplay::removePolylinePos( int idx )
{
    if ( !polyline_ || idx>polyline_->size() )
	return;

    polyline_->removePoint( idx );
    redrawLine();
}


void PickSetDisplay::redrawAll( int drageridx )
{
    if ( !markerset_ )
	return;

    if ( !polyline_ && needLine() )
	createLine();

    markerset_->clearMarkers();
    if ( drageridx==set_->size() )
	drageridx = set_->size()-1;

    for ( int idx=0; idx<set_->size(); idx++ )
    {
	Coord3 pos = (*set_)[idx].pos_;
	if ( datatransform_ )
	    pos.z = datatransform_->transform( pos );
	if ( !mIsUdf(pos.z) )
	    markerset_->addPos( pos );

	if ( idx==drageridx && dragger_ )
	{
	    dragger_->setPos( pos );
	    dragger_->updateDragger( false );
	    dragger_->turnOn( showdragger_ );
	}
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

    if ( polyline_ )
	polyline_->removeAllPoints();
}


void PickSetDisplay::createLine()
{
    if ( polyline_ || !set_ )
	return;

    polyline_ = visBase::PolyLine::create();
    polyline_->ref();
    addChild( polyline_->osgNode() );
    polyline_->setDisplayTransformation( transformation_ );
    polyline_->setMaterial( 0 );

    int pixsize = set_->disp_.pixsize_;
    LineStyle ls;
    ls.width_ = pixsize;
    polyline_->setLineStyle( ls );
}


void PickSetDisplay::redrawLine()
{
    if ( !polyline_ )
	return;

    int pixsize = set_->disp_.pixsize_;
    LineStyle ls;
    ls.width_ = pixsize;
    polyline_->setLineStyle( ls );

    polyline_->removeAllPoints();
    int idx=0;
    for ( ; idx<set_->size(); idx++ )
    {
	Coord3 pos = (*set_)[idx].pos_;
	if ( datatransform_ )
	    pos.z = datatransform_->transform( pos );
	if ( !mIsUdf(pos.z) )
	    polyline_->addPoint( pos );
    }

    if ( idx && set_->disp_.connect_==Pick::Set::Disp::Close )
	polyline_->setPoint( idx, polyline_->getPoint(0) );

    polyline_->dirtyCoordinates();
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

    if ( !polyline_ )
	createLine();

    redrawLine();
    polyline_->turnOn( yn );
}


bool PickSetDisplay::lineShown() const
{
    return polyline_ ? polyline_->isOn() : false;
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
	const float phi = SI().isRightHandSystem() ? survngle + loc.dir_.phi
					     : survngle - loc.dir_.phi;
	const Quaternion azimuth( Coord3(0,0,1), phi );
	const Quaternion dip( Coord3(0,1,0), loc.dir_.theta );
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
    bodydisplay_->getMaterial()->setColor( set_->disp_.color_ );
    bodydisplay_->setDisplayTransformation( transformation_ );

    TypeSet<Coord3> picks;
    for ( int idx=0; idx<set_->size(); idx++ )
    {
	picks += (*set_)[idx].pos_;
	if ( datatransform_ )
	    picks[idx].z = datatransform_->transformBack( picks[idx] );
    }

    return  bodydisplay_->setPoints( picks );
}


visBase::MarkerSet* PickSetDisplay::createOneMarker() const
{
    visBase::MarkerSet* marker = visBase::MarkerSet::create();
    MarkerStyle3D markerstyle;
    markerstyle.size_ = set_->disp_.pixsize_;
    markerstyle.type_ = ( MarkerStyle3D::Type ) set_->disp_.markertype_;
    marker->setMaterial(0);
    marker->setMarkerStyle(markerstyle);
    marker->setMarkersSingleColor( Color::NoColor() );
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
			const ObjectSet<const SurveyObject>& objs, int )
{
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

    Coord3 pos = set_->validIdx(idx) ? (*set_)[idx].pos_ : Coord3::udf();
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
   if ( polyline_ )
   {
       TypeSet<Coord3> polycoords;
       for ( int idx = 0; idx<markerset_->getCoordinates()->size(); idx++ )
	   polycoords += markerset_->getCoordinates()->getPos(idx);

	polyline_->removeAllPoints();
	int pidx = 0;
	for ( pidx=0; pidx<polycoords.size(); pidx++ )
	{
	    if ( markerset_->markerOn(pidx) )
		polyline_->addPoint( polycoords[pidx] );
	}

	if ( pidx && set_->disp_.connect_ == Pick::Set::Disp::Close )
	    polyline_->setPoint( pidx, polyline_->getPoint(0) );
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
	areastring = "Area=";
	areastring += getAreaString( area, false, 0 );
    }

    str = "Picking (Nr picks="; str += set_ ? set_->size() : 0;
    if ( !areastring.isEmpty() ) { str +=", "; str += areastring; }
    str += ")";
}


void PickSetDisplay::setColor( Color nc )
{
    if ( set_ )
	set_->disp_.color_ = nc;

    if ( !bodydisplay_ ) return;

    if ( !bodydisplay_->getMaterial() )
	bodydisplay_->setMaterial( new visBase::Material );

    bodydisplay_->getMaterial()->setColor( nc );
    markerset_->setMarkersSingleColor( nc );
}


void PickSetDisplay::setPixelDensity( float dpi )
{
    LocationDisplay::setPixelDensity( dpi );

    if ( markerset_ )
       markerset_->setPixelDensity( dpi );
    if ( bodydisplay_ )
	bodydisplay_->setPixelDensity( dpi );
    if ( polyline_ )
	polyline_->setPixelDensity( dpi );

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
    if ( polyline_ )
	polyline_->setDisplayTransformation( newtr );
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
    deepErase( selectors_ );
    pickselstatus_.setAll( false );
}


void PickSetDisplay::setPickSelect( int idx, bool yn )
{
    Color clr = yn ? color_.complementaryColor() : color_;
    markerset_->getMaterial()->setColor( clr, idx );
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

}


bool PickSetDisplay::removeSelections( TaskRunner* taskr )
{
    bool changed = false;
    for ( int idx=pickselstatus_.size()-1; idx>=0; idx-- )
    {
	if ( pickselstatus_[idx] )
	{
	    Pick::SetMgr::ChangeData cd(
		Pick::SetMgr::ChangeData::ToBeRemoved, set_,idx );
	    set_->removeSingleWithUndo( idx );
	    Pick::Mgr().reportChange( 0, cd );
	    changed = true;
	}
    }

    Pick::Mgr().undo().setUserInteractionEnd(
	    Pick::Mgr().undo().currentEventID() );

    unSelectAll();
    return changed;
}

} // namespace visSurvey
