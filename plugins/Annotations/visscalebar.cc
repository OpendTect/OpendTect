/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          Jan 2005
________________________________________________________________________

-*/

#include "visscalebar.h"

#include "pickset.h"
#include "survinfo.h"
#include "viscoord.h"
#include "visdrawstyle.h"
#include "vismarker.h"
#include "vispolyline.h"
#include "vistransform.h"

mCreateFactoryEntry( Annotations::ScaleBar );
mCreateFactoryEntry( Annotations::ScaleBarDisplay );

namespace Annotations
{

ScaleBar::ScaleBar()
    : visBase::VisualObjectImpl(true)
    , displaytrans_(0)
    , length_(1000)
    , firstloc_(*new Pick::Location)
    , oninlcrl_(true)
    , orientation_(0)
{
    firstloc_.pos_ = Coord3::udf();
    setMaterial( 0 );

    linestyle_ = visBase::DrawStyle::create();
    linestyle_->ref();
    insertChild( 0, linestyle_->getInventorNode() );
    setLineWidth( 2 );

    marker1_ = visBase::Marker::create();
    marker1_->setMaterial( 0 );
    marker1_->setMarkerStyle( MarkerStyle3D::Sphere );
    marker1_->ref();
    addChild( marker1_->getInventorNode() );

    marker2_ = visBase::Marker::create();
    marker2_->setMaterial( 0 );
    marker2_->setMarkerStyle( MarkerStyle3D::Sphere );
    marker2_->ref();
    addChild( marker2_->getInventorNode() );

    polyline_ = visBase::IndexedPolyLine::create();
    polyline_->setMaterial( 0 );
    polyline_->ref();
    addChild( polyline_->getInventorNode() );
}


ScaleBar::~ScaleBar()
{
    delete &firstloc_;
    linestyle_->unRef();
    marker1_->unRef();
    marker2_->unRef();
    polyline_->unRef();
    if ( displaytrans_ ) displaytrans_->unRef();
}


void ScaleBar::setLength( double l )
{
    if ( length_ == l )
	return;

    length_ = l;
    updateVis( firstloc_ );
}


void ScaleBar::setOnInlCrl( bool yn )
{
    if ( oninlcrl_ = yn )
	return;

    oninlcrl_ = yn;
    updateVis( firstloc_ );
}


void ScaleBar::setOrientation( int orient )
{
    if ( orientation_ == orient )
	return;

    orientation_ = orient;
    updateVis( firstloc_ );
}


void ScaleBar::setPick( const Pick::Location& loc )
{
    firstloc_ = loc;
    updateVis( loc );
}


void ScaleBar::updateVis( const Pick::Location& loc )
{
    if ( !loc.pos_.isDefined() )
	return;

    const Coord3 pos1 = loc.pos_;
    const Coord3 pos2 = getSecondPos( loc );

    marker1_->setCenterPos( pos1 );
    marker2_->setCenterPos( pos2 );

    polyline_->setCoordIndex( 0, 0 );
    polyline_->setCoordIndex( 1, 1 );
    polyline_->getCoordinates()->setPos( 0, pos1 );
    polyline_->getCoordinates()->setPos( 1, pos2 );
}


Coord3 ScaleBar::getSecondPos( const Pick::Location& loc ) const
{
    Coord3 pos = Coord3::udf();
    if ( oninlcrl_ )
    {
	if ( orientation_ == 1 )
	{
	    pos = loc.pos_;
	    pos.z += length_;
	}
	else
	{
	    const Coord3 vector = spherical2Cartesian( loc.dir_, true );
	    const double signx = vector.x > 0 ? 1 : -1;
	    const double signy = vector.y > 0 ? 1 : -1;

	    const double l2 = length_*length_;
	    const double vx2 = vector.x*vector.x;
	    const double vy2 = vector.y*vector.y;
	    const double v2 = vx2 + vy2;
	    const double factor = l2 / v2;
	    const double dx2 = vx2 * factor;
	    const double dy2 = vy2 * factor;

	    pos = loc.pos_ +
		Coord3( signx*Math::Sqrt(dx2), signy*Math::Sqrt(dy2), 0 );
	}
    }
    else
    {
	pos = loc.pos_;
	if ( orientation_ == 0 )
	    pos.x += length_;
	else
	    pos.y += length_;
    }

    return pos;
}


void ScaleBar::setLineWidth( int width )
{ linestyle_->setLineStyle( LineStyle(LineStyle::Solid,width) ); }

void ScaleBar::setDisplayTransformation( const mVisTrans* nt )
{
    if ( displaytrans_ )
	return;

    marker1_->setDisplayTransformation( nt );
    marker2_->setDisplayTransformation( nt );
    polyline_->setDisplayTransformation( nt );

    displaytrans_ = nt;
    if ( displaytrans_ )
	displaytrans_->ref();
}


static const char* sKeyOnInlCrl()	{ return "On Inl/Crl"; }
static const char* sKeyOrientation()	{ return "Orientation"; }
static const char* sKeyLineWidth()	{ return "Line width"; }
static const char* sKeyLength()		{ return "Length"; }

// ScaleBarDisplay
ScaleBarDisplay::ScaleBarDisplay()
    : orientation_(0)
    , oninlcrl_(true)
    , length_(1000)
    , linewidth_(2)
{
}


ScaleBarDisplay::~ScaleBarDisplay()
{
    if ( scene_ )
	scene_->zstretchchange.remove( mCB(this,ScaleBarDisplay,zScaleCB) );
}


void ScaleBarDisplay::setScene( visSurvey::Scene* ns )
{
    if ( scene_ )
	scene_->zstretchchange.remove( mCB(this,ScaleBarDisplay,zScaleCB) );
    visSurvey::SurveyObject::setScene( ns );

    if ( scene_ )
	scene_->zstretchchange.notify( mCB(this,ScaleBarDisplay,zScaleCB) );
}


#define mToGroup( fn, val ) \
    for ( int idx=0; idx<group_->size(); idx++ ) \
    { \
	mDynamicCastGet(ScaleBar*,sb,group_->getObject(idx)); \
	if ( sb ) sb->fn( val ); \
    }

void ScaleBarDisplay::setLength( double l )
{
    length_ = l;
    mToGroup( setLength, l )
}

double ScaleBarDisplay::getLength() const
{ return length_; }


void ScaleBarDisplay::setLineWidth( int width )
{
    linewidth_ = width;
    mToGroup( setLineWidth, width )
}


int ScaleBarDisplay::getLineWidth() const
{ return linewidth_; }


void ScaleBarDisplay::setOrientation( int ortn )
{
    orientation_ = ortn;
    mToGroup( setOrientation, ortn )
}


int ScaleBarDisplay::getOrientation() const
{ return orientation_; }


void ScaleBarDisplay::setOnInlCrl( bool yn )
{
    oninlcrl_ = yn;
    mToGroup( setOnInlCrl, yn );
}


bool ScaleBarDisplay::isOnInlCrl() const
{ return oninlcrl_; }


void ScaleBarDisplay::zScaleCB( CallBacker* )
{ fullRedraw(); }


void ScaleBarDisplay::dispChg( CallBacker* cb )
{
    fullRedraw();
    LocationDisplay::dispChg( cb );
}


visBase::VisualObject* ScaleBarDisplay::createLocation() const
{
    ScaleBar* sb = ScaleBar::create();
    sb->setOnInlCrl( oninlcrl_ );
    sb->setLineWidth( linewidth_ );
    sb->setLength( length_ );
    sb->setOrientation( orientation_ );
    return sb;
}
	

void ScaleBarDisplay::setPosition( int idx, const Pick::Location& loc )
{
    const Coord3 normal = spherical2Cartesian( loc.dir_, true );
    const bool pickedonz = mIsEqual(normal.z,1,mDefEps);
    if ( idx==0 )
	setOnInlCrl( !pickedonz );
    else if ( pickedonz == oninlcrl_ )
	return;

    mDynamicCastGet(ScaleBar*,sb,group_->getObject(idx));
    if ( sb ) sb->setPick( loc );
}


int ScaleBarDisplay::isMarkerClick( const TypeSet<int>& path ) const
{
    for ( int idx=0; idx<group_->size(); idx++ )
    {
	mDynamicCastGet(ScaleBar*,sb,group_->getObject(idx));
	if ( sb && path.isPresent(sb->id()) )
	    return idx;
    }

    return -1;
}


void ScaleBarDisplay::fromPar( const IOPar& par )
{
    bool oninlcrl = true;
    par.getYN( sKeyOnInlCrl(), oninlcrl );
    setOnInlCrl( oninlcrl );

    int orientation = 0;
    par.get( sKeyOrientation(), orientation );
    setOrientation( orientation );

    int linewidth = 2;
    par.get( sKeyLineWidth(), linewidth );
    setLineWidth( linewidth );

    double length = 1000;
    par.get( sKeyLength(), length );
    setLength( length );
}


void ScaleBarDisplay::toPar( IOPar& par ) const
{
    par.setYN( sKeyOnInlCrl(), isOnInlCrl() );
    par.set( sKeyOrientation(), getOrientation() );
    par.set( sKeyLineWidth(), getLineWidth() );
    par.set( sKeyLength(), getLength() );
}

} // namespace Annotation
