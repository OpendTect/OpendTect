/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2002
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "vispicksetdisplay.h"


#include "mousecursor.h"
#include "pickset.h"
#include "vismarkerset.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "visrandompos2body.h"
#include "visevent.h"
#include "vistransform.h"
#include "zaxistransform.h"


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
{
    markerset_->ref();
    addChild( markerset_->osgNode() );
}


PickSetDisplay::~PickSetDisplay()
{
    unRefAndZeroPtr( bodydisplay_ );
    removeChild( markerset_->osgNode() );
    unRefAndZeroPtr( markerset_ );

    if ( polyline_ )
    {
	removeChild( polyline_->osgNode() );
	unRefAndZeroPtr( polyline_ );
    }
}


void PickSetDisplay::setSet( Pick::Set* newset )
{
    LocationDisplay::setSet( newset );
    
    if ( !newset )
	return;

    MarkerStyle3D markerstyle;
    markerstyle.size_ = set_->disp_.pixsize_;
    markerstyle.type_ = (MarkerStyle3D::Type) set_->disp_.markertype_;
    markerset_->setMaterial( 0 );
    markerset_->setMarkerStyle( markerstyle );
    markerset_->setMarkersSingleColor( set_->disp_.color_ );

    //   if ( scene_ ) // should do later
    //markerset->setZStretch( scene_->getZStretch()*scene_->getZScale()/2 );

    //fullRedraw();

    if ( !showall_ && scene_ )
	scene_->objectMoved( 0 );
}


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

    if( markerset_->getType() != set_->disp_.markertype_)
	markerset_->setType( (MarkerStyle3D::Type)set_->disp_.markertype_ );

    LocationDisplay::dispChg( cb );
    markerset_->setMarkersSingleColor( set_->disp_.color_  );
    markerset_->forceRedraw( true );
    showLine( needLine() );
}


void PickSetDisplay::setPosition( int idx, const Pick::Location& loc )
{
    if ( set_->disp_.connect_ == Pick::Set::Disp::Close )
    {
	redrawAll();
	return;
    }

    markerset_->setPos( idx, loc.pos_, false );

    if ( needLine() )
	setPolylinePos( idx, loc.pos_ );
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
	redrawAll();
	return;
    }

    if ( !markerset_ || idx>markerset_->size() )
	return;

    markerset_->removeMarker( idx );
    removePolylinePos( idx );
}


void PickSetDisplay::removePolylinePos( int idx )
{
    if ( !polyline_ || idx>polyline_->size() )
	return;

    polyline_->removePoint( idx );
    polyline_->dirtyCoordinates();
}


void PickSetDisplay::redrawAll()
{
    if ( !markerset_ )
	return;

    if ( !polyline_ )
	createLine();

    markerset_->clearMarkers();
    for ( int idx=0; idx<set_->size(); idx++ )
    {
	Coord3 pos = (*set_)[idx].pos_;
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
    if ( !polyline_ )
	createLine();
    
    redrawLine();
    polyline_->turnOn( yn );
}


bool PickSetDisplay::lineShown() const
{
    return polyline_ ? polyline_->isOn() : false;
}


::Sphere PickSetDisplay::getDirection( int loc ) const 
{
   // return markerset->getDirection();
    return 0;  // to be implement later
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


int PickSetDisplay::clickedMarkerIndex( const visBase::EventInfo& evi ) const
{
    if ( !isMarkerClick( evi ) )
	return -1;

    return  markerset_->findClosestMarker( evi.displaypickedpos, true );
}


bool PickSetDisplay::isMarkerClick( const visBase::EventInfo& evi ) const
{ 
    return evi.pickedobjids.isPresent( markerset_->id() );
}


void PickSetDisplay::otherObjectsMoved(
			const ObjectSet<const SurveyObject>& objs, int )
{
    //TypeSet<Coord3> polycoords;
    const bool showline = objs.size() ? true : false;
    if( polyline_ && showline && markerset_->size() )
	    polyline_->removeAllPoints();

    for ( int idx=0; idx<markerset_->getCoordinates()->size(); idx++ )
    {
	Coord3 pos = (*set_)[idx].pos_;
	if ( datatransform_ ) pos.z = datatransform_->transform( pos );

	if ( scene_ )
	    scene_->getUTM2DisplayTransform()->transform( pos );

	bool newstatus;
	if ( !pos.isDefined() )
	    newstatus = false;
	else if ( showall_ )
	    newstatus = true;
	else
	{
	    newstatus = false;

	    for ( int idy=0; idy<objs.size(); idy++ )
	    {
		const float dist = objs[idy]->calcDist(pos);
		if ( dist<objs[idy]->maxDist() )
		{
		    newstatus = true;
		    break;
		}
	    }
	}

	if ( newstatus && !invalidpicks_.isEmpty()
		       && invalidpicks_.indexOf(idx)!=-1 )
	{
	    Pick::Location loc = (*set_)[idx];
	    if ( transformPos(loc) )
	    {
		invalidpicks_ -= idx;
		setPosition( idx, loc );
	    }
	    else
	    {
		newstatus = false;
	    }
	}

	markerset_->turnMarkerOn( idx, newstatus );
	if ( showline && newstatus )
	{
	    const Coord3& pos = markerset_->getCoordinates()->getPos( idx );
	    setPolylinePos( idx, pos );
	}

	//if ( newstatus )
	    //polycoords += markerset_->getCoordinates()->getPos( idx );
    }

    //polyline_->turnOn( showall_ );
 //   if ( polyline_ )
 //   {
	//polyline_->removeAllPoints();
	//int pidx = 0;
	//for ( pidx=0; pidx<polycoords.size(); pidx++ )
	//    polyline_->addPoint( polycoords[pidx] );

	//if ( pidx && set_->disp_.connect_ == Pick::Set::Disp::Close )
	//    polyline_->setPoint( pidx, polyline_->getPoint(0) );
 //   }
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
}


const mVisTrans* PickSetDisplay::getDisplayTransformation() const
{
    return transformation_;
}


void PickSetDisplay::fillPar( IOPar& par ) const
{
    LocationDisplay::fillPar( par );
    par.set( sKeyDisplayBody(), shoulddisplaybody_ );
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

}; // namespace visSurvey
