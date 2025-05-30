/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visarrowdisplay.h"

#include "pickset.h"
#include "trigonometry.h"
#include "viscoord.h"
#include "visdrawstyle.h"
#include "visevent.h"
#include "vislines.h"

mCreateFactoryEntry( visSurvey::ArrowDisplay );

namespace visSurvey
{

ArrowDisplay::ArrowDisplay()
{
    ref();
    group_ = visBase::DataObjectGroup::create();
    linestyle_ = visBase::DrawStyle::create();
    addChild( group_->osgNode() );
    setLineWidth( 2 );
    unRefNoDelete();
}


ArrowDisplay::~ArrowDisplay()
{
}


void ArrowDisplay::setDisplayTransformation( const mVisTrans* tf )
{
    displaytransform_ = tf;
    LocationDisplay::setDisplayTransformation( tf );
}


const mVisTrans* ArrowDisplay::getDisplayTransformation() const
{
    return displaytransform_.ptr();
}


void ArrowDisplay::setType( Type typ )
{
    arrowtype_ = typ;

    for ( int idx=group_->size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( visBase::Lines*, lines, group_->getObject(idx));
	if ( !lines ) continue;
	updateLineIndices( lines );
    }
}


void ArrowDisplay::setScene( Scene* ns )
{
    SurveyObject::setScene( ns );
}


ArrowDisplay::Type ArrowDisplay::getType() const
{ return arrowtype_; }


void ArrowDisplay::setLineWidth( int nw )
{
    linestyle_->setLineStyle( OD::LineStyle(OD::LineStyle::Solid,nw) );
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


RefMan<visBase::VisualObject> ArrowDisplay::createLocation() const
{
    RefMan<visBase::Lines> lines = visBase::Lines::create();
    lines->setMaterial( nullptr );
    lines->setSelectable( true );
    lines->setDisplayTransformation( displaytransform_.ptr() );
    lines->addNodeState( linestyle_.getNonConstPtr() );
    RefMan<Geometry::IndexedPrimitiveSet> indices =
				Geometry::IndexedPrimitiveSet::create( false );
    lines->addPrimitiveSet( indices.ptr() );
    updateLineIndices( lines.ptr() );
    return lines;
}


void ArrowDisplay::setPosition( int idx, const Pick::Location& loc, bool add )
{
    mDynamicCastGet( visBase::Lines*, lines, group_->getObject(idx) );

    if ( !lines )
    {
	RefMan<visBase::VisualObject> locations = createLocation();
	lines = static_cast<visBase::Lines*>( locations.ptr() );
	group_->addObject( lines );
    }

    lines->getCoordinates()->setPos( 0, loc.pos() );

    if ( mIsUdf(loc.dir().radius) || mIsUdf(loc.dir().theta) ||
	    mIsUdf(loc.dir().phi) )
	return;

    const Coord3 d0 = world2Display( loc.pos() );
    Coord3 vector = spherical2Cartesian( loc.dir(), true );

    if ( scene_ )
        vector.z_ /= -scene_->getZScale();
    const Coord3 c1 = loc.pos()+vector;
    Coord3 d1 = world2Display( c1 );
    Coord3 displayvector = d1-d0;
    const double len = displayvector.abs();
    if ( mIsZero(len,1e-3) )
	return;

    displayvector /= len;
    displayvector *= set_->disp_.pixsize_;
    //Note: pos.vec points in the direction of the tail, not the arrow.
    d1 = d0+displayvector;
    lines->getCoordinates()->setPos( 1, display2World(d1) );

    const Coord3 planenormal( sin(loc.dir().phi), -cos(loc.dir().phi), 0 );
    const Quaternion plus45rot(planenormal, M_PI/4);
    const Quaternion minus45rot(planenormal, -M_PI/4 );
    Coord3 arrowheadvec = minus45rot.rotate( displayvector*.3 );
    lines->getCoordinates()->setPos( 2, display2World(d0+arrowheadvec) );

    arrowheadvec = plus45rot.rotate( displayvector*.3 );
    lines->getCoordinates()->setPos( 3, display2World(d0+arrowheadvec) );
}


void ArrowDisplay::updateLineIndices( visBase::Lines* lines ) const
{
    if ( !lines || lines->nrPrimitiveSets()<1 )
	return;

    mDynamicCastGet(Geometry::IndexedPrimitiveSet*,
		    indices,lines->getPrimitiveSet(0));
    if ( !indices )
	return;

    TypeSet<int> indexarray;

    indexarray += 0; //0
    indexarray += 1; //1

    if ( arrowtype_==Bottom || arrowtype_==Double )
    {
	 indexarray += 0; //2
	 indexarray += 2; //3
    }
    else
    {
	indexarray += 0; //4
	indexarray += 3; //5
    }
    if ( arrowtype_ == Double )
    {
	indexarray += 0; //4
	indexarray += 3; //5
    }

    indices->setEmpty();
    indices->set( indexarray.arr(), indexarray.size() );
    lines->dirtyCoordinates();
    requestSingleRedraw();
}


void ArrowDisplay::removePosition( int idx )
{
    if ( idx >= group_->size() )
	return;

    group_->removeObject( idx );
}


int ArrowDisplay::clickedMarkerIndex(const visBase::EventInfo& evi)const
{
    for ( int idx=0; idx<group_->size(); idx++ )
    {
	mDynamicCastGet(const visBase::Lines*,lines,group_->getObject(idx));
	if ( lines && evi.pickedobjids.isPresent(lines->id()) )
	    return idx;
    }

    return -1;
}


} // namespace visSurvey
