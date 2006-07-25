/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          Jan 2005
 RCS:           $Id: visarrow.cc,v 1.2 2006-07-25 22:23:22 cvskris Exp $
________________________________________________________________________

-*/

#include "visarrow.h"

#include "trigonometry.h"
#include "vispolyline.h"
#include "visdrawstyle.h"
#include "pickset.h"
#include "viscoord.h"

mCreateFactoryEntry( visSurvey::ArrowAnnotationDisplay );

namespace visSurvey
{
ArrowAnnotationDisplay::ArrowAnnotationDisplay()
    : arrowtype_( Double )
    , linestyle_( visBase::DrawStyle::create() )
{
    linestyle_->ref();
    insertChild( 0, linestyle_->getInventorNode() );
}


ArrowAnnotationDisplay::~ArrowAnnotationDisplay()
{
    linestyle_->unRef();

    if ( scene_ )
	scene_->zscalechange.notify(mCB(this,ArrowAnnotationDisplay,zScaleCB));
}


void ArrowAnnotationDisplay::setType( Type typ )
{
    arrowtype_ = typ;

    for ( int idx=group_->size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( visBase::IndexedPolyLine*, pl, group_->getObject(idx));
	if ( !pl ) continue;
	updateLineShape( pl );
    }
}


void ArrowAnnotationDisplay::setScene( visSurvey::Scene* ns )
{
    if ( scene_ )
	scene_->zscalechange.remove(mCB(this,ArrowAnnotationDisplay,zScaleCB));
    visSurvey::SurveyObject::setScene( ns );

    if ( scene_ )
	scene_->zscalechange.notify(mCB(this,ArrowAnnotationDisplay,zScaleCB));
}


ArrowAnnotationDisplay::Type ArrowAnnotationDisplay::getType() const
{ return arrowtype_; }


void ArrowAnnotationDisplay::setLineWidth( int nw )
{
    linestyle_->setLineStyle( LineStyle( LineStyle::Solid, nw ) );
}

int ArrowAnnotationDisplay::getLineWidth() const
{
    return linestyle_->lineStyle().width;
}


void ArrowAnnotationDisplay::zScaleCB(CallBacker*)
{
    fullRedraw();
}


void ArrowAnnotationDisplay::dispChg( CallBacker* cb )
{
    fullRedraw();
    LocationDisplay::dispChg( cb );
}


visBase::VisualObject* ArrowAnnotationDisplay::createLocation() const
{
    visBase::IndexedPolyLine* pl = visBase::IndexedPolyLine::create();
    pl->ref();
    pl->setSelectable( true );
    updateLineShape( pl );
    pl->unRefNoDelete();
    return pl;
}
	

void visSurvey::ArrowAnnotationDisplay::setPosition( int idx,
	const Pick::Location& loc )
{
    mDynamicCastGet( visBase::IndexedPolyLine*, line, group_->getObject(idx) );
    line->getCoordinates()->setPos( 0, loc.pos );
    if ( mIsUdf(loc.dir.radius) || mIsUdf(loc.dir.theta) || mIsUdf(loc.dir.phi))
	return;

    const Coord3 d0 = world2Display( loc.pos );
    const Coord3 vector = spherical2Cartesian( loc.dir, true );
    Coord3 c1 = loc.pos+vector;
    Coord3 d1 = world2Display( c1 );
    Coord3 displayvector = d1-d0;
    const float len = displayvector.abs();
    if ( mIsZero(len,1e-3) )
	return;

    displayvector /= len;
    displayvector *= set_->disp_.pixsize_;
    d1 = d0+displayvector;
    line->getCoordinates()->setPos( 1, display2World( d1 ) );

    static const float cos15 = (sqrt(6)-M_SQRT2)/4;
    static const float sin15 = (sqrt(6)+M_SQRT2)/4;

    const Coord3 planenormal( sin(loc.dir.phi)*sin15,
	    		      cos(loc.dir.phi)*sin15, 0 );
    const Quaternion plus30rot( cos15, planenormal );
    const Quaternion minus30rot( -cos15, planenormal );
    Coord3 arrowheadvec;
    minus30rot.rotate( displayvector*.3, arrowheadvec );
    line->getCoordinates()->setPos( 2, display2World(arrowheadvec+d1) );
    
    plus30rot.rotate( displayvector*.3, arrowheadvec );
    line->getCoordinates()->setPos( 3, display2World(arrowheadvec+d1) );
}


void ArrowAnnotationDisplay::updateLineShape(visBase::IndexedPolyLine* pl) const
{
    pl->ref();
    int idx = 0;
    pl->setCoordIndex( idx++, 0 );
    pl->setCoordIndex( idx++, 1 );

    if ( arrowtype_==Top || arrowtype_==Double )
	pl->setCoordIndex( idx++, 2 );
    else
	pl->setCoordIndex( idx++, 3 );

    if ( arrowtype_==Double )
    {
	pl->setCoordIndex( idx++, -1 );
	pl->setCoordIndex( idx++, 1 );
	pl->setCoordIndex( idx++, 3 );
    }

    pl->setCoordIndex( idx, -1 );
    pl->removeCoordIndexAfter( idx );

    pl->unRef();
}


}; // namespace
