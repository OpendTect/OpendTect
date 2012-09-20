/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          Jan 2005
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "visarrow.h"

#include "trigonometry.h"
#include "vispolyline.h"
#include "visdrawstyle.h"
#include "pickset.h"
#include "survinfo.h"
#include "viscoord.h"

mCreateFactoryEntry( Annotations::ArrowDisplay );

namespace Annotations
{
ArrowDisplay::ArrowDisplay()
    : arrowtype_( Double )
    , linestyle_( visBase::DrawStyle::create() )
{
    linestyle_->ref();
    insertChild( 0, linestyle_->getInventorNode() );
    setLineWidth( 2 );
}


ArrowDisplay::~ArrowDisplay()
{
    linestyle_->unRef();

    if ( scene_ )
	scene_->zstretchchange.remove( mCB(this,ArrowDisplay,zScaleCB) );
}


void ArrowDisplay::setType( Type typ )
{
    arrowtype_ = typ;

    for ( int idx=group_->size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( visBase::IndexedPolyLine*, pl, group_->getObject(idx));
	if ( !pl ) continue;
	updateLineShape( pl );
    }
}


void ArrowDisplay::setScene( visSurvey::Scene* ns )
{
    if ( scene_ )
	scene_->zstretchchange.remove( mCB(this,ArrowDisplay,zScaleCB) );
    visSurvey::SurveyObject::setScene( ns );

    if ( scene_ )
	scene_->zstretchchange.notify( mCB(this,ArrowDisplay,zScaleCB) );
}


ArrowDisplay::Type ArrowDisplay::getType() const
{ return arrowtype_; }


void ArrowDisplay::setLineWidth( int nw )
{
    linestyle_->setLineStyle( LineStyle(LineStyle::Solid,nw) );
}


int ArrowDisplay::getLineWidth() const
{
    return linestyle_->lineStyle().width_;
}


void ArrowDisplay::zScaleCB( CallBacker* )
{
    fullRedraw();
}


void ArrowDisplay::dispChg( CallBacker* cb )
{
    fullRedraw();
    LocationDisplay::dispChg( cb );
}


visBase::VisualObject* ArrowDisplay::createLocation() const
{
    visBase::IndexedPolyLine* pl = visBase::IndexedPolyLine::create();
    pl->setMaterial( 0 );
    pl->ref();
    pl->setSelectable( true );
    updateLineShape( pl );
    pl->unRefNoDelete();
    return pl;
}
	

void ArrowDisplay::setPosition( int idx, const Pick::Location& loc )
{
    mDynamicCastGet( visBase::IndexedPolyLine*, line, group_->getObject(idx) );
    line->getCoordinates()->setPos( 0, loc.pos );
    if ( mIsUdf(loc.dir.radius) || mIsUdf(loc.dir.theta) || mIsUdf(loc.dir.phi))
	return;

    const Coord3 d0 = world2Display( loc.pos );
    Coord3 vector = spherical2Cartesian( loc.dir, true );

    if ( scene_ )
	vector.z /= -scene_->getZScale();
    const Coord3 c1 = loc.pos+vector;
    Coord3 d1 = world2Display( c1 );
    Coord3 displayvector = d1-d0;
    const double len = displayvector.abs();
    if ( mIsZero(len,1e-3) )
	return;

    displayvector /= len;
    displayvector *= set_->disp_.pixsize_;
    //Note: pos.vec points in the direction of the tail, not the arrow.
    d1 = d0+displayvector;
    line->getCoordinates()->setPos( 1, display2World( d1 ) );

    const Coord3 planenormal( sin(loc.dir.phi), -cos(loc.dir.phi), 0 );
    const Quaternion plus45rot(planenormal, M_PI/4);
    const Quaternion minus45rot(planenormal, -M_PI/4 );
    Coord3 arrowheadvec = minus45rot.rotate( displayvector*.3 );
    line->getCoordinates()->setPos( 2, display2World(d0+arrowheadvec) );
    
    arrowheadvec = plus45rot.rotate( displayvector*.3 );
    line->getCoordinates()->setPos( 3, display2World(d0+arrowheadvec) );
}


void ArrowDisplay::updateLineShape( visBase::IndexedPolyLine* pl ) const
{
    pl->ref();
    int idx = 0;
    pl->setCoordIndex( idx++, 1 );
    pl->setCoordIndex( idx++, 0 );

    if ( arrowtype_==Bottom || arrowtype_==Double )
	pl->setCoordIndex( idx++, 2 );
    else
	pl->setCoordIndex( idx++, 3 );

    if ( arrowtype_==Double )
    {
	pl->setCoordIndex( idx++, -1 );
	pl->setCoordIndex( idx++, 0 );
	pl->setCoordIndex( idx++, 3 );
    }

    pl->setCoordIndex( idx, -1 );
    pl->removeCoordIndexAfter( idx );

    pl->unRef();
}


int ArrowDisplay::isMarkerClick( const TypeSet<int>& path ) const
{
    for ( int idx=group_->size()-1; idx>=0; idx-- )
    {
	if ( path.indexOf(group_->getObject(idx)->id())!=-1 )
	    return idx;
    }

    return -1;

}


}; // namespace
