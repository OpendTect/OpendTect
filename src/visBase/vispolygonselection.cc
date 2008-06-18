/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          June 2008
 RCS:           $Id: vispolygonselection.cc,v 1.1 2008-06-18 21:53:08 cvskris Exp $
________________________________________________________________________

-*/

#include "vispolygonselection.h"

#include "polygon.h"
#include "vistransform.h"
#include "visdrawstyle.h"

#include "SoPolygonSelect.h"

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMaterial.h>

#include <math.h>

mCreateFactoryEntry( visBase::PolygonSelection );

namespace visBase
{

PolygonSelection::PolygonSelection()
    : VisualObjectImpl( false )
    , transformation_( 0 )
    , selector_( new SoPolygonSelect )
    , drawstyle_( DrawStyle::create() )
    , polygon_( 0 )
{
    removeSwitch();

    drawstyle_->ref();
    addChild( drawstyle_->getInventorNode() );
    addChild( selector_ );
    selector_->polygonChange.addCallback(
	    (SoCallbackListCB*) polygonChangeCB, this );
}


PolygonSelection::~PolygonSelection()
{
    selector_->polygonChange.removeCallback(
	    (SoCallbackListCB*) polygonChangeCB, this );
    if ( transformation_ ) transformation_->unRef();
    drawstyle_->unRef();
    delete polygon_;
}


void PolygonSelection::setSelectionType( PolygonSelection::SelectionType st )
{
    if ( st==Off )
	selector_->mode = SoPolygonSelect::OFF;
    else if ( st==Rectangle )
	selector_->mode = SoPolygonSelect::RECTANGLE;
    else if ( st==Polygon )
	selector_->mode = SoPolygonSelect::POLYGON;
}


PolygonSelection::SelectionType PolygonSelection::getSelectionType() const
{
    if ( selector_->mode.getValue() == SoPolygonSelect::OFF )
	return Off;

    if ( selector_->mode.getValue() == SoPolygonSelect::RECTANGLE )
	return Rectangle;

    return Polygon;
}


void PolygonSelection::setLineStyle( const LineStyle& lst )
{
    drawstyle_->setLineStyle( lst );
}


const LineStyle& PolygonSelection::getLineStyle() const
{ return drawstyle_->lineStyle(); }


bool PolygonSelection::isInside( const Coord3& crd, bool displayspace ) const
{
    if ( selector_->mode.getValue() == SoPolygonSelect::OFF )
	return false;

    if ( !selector_->getPolygon().getLength() )
	return false;

    Coord3 checkcoord3d = crd;
    if ( !displayspace && transformation_ )
	checkcoord3d = transformation_->transform( checkcoord3d );

    const SbVec2f coord2d = selector_->projectPoint(
	    SbVec3f(checkcoord3d.x,checkcoord3d.y,checkcoord3d.z ) );

    const Coord checkcoord2d( coord2d[0], coord2d[1] );

    polygonlock_.readLock();
    if ( !polygon_ )
    {
	polygonlock_.convToWriteLock();
	if ( !polygon_ )
	{
	    polygon_ = new ODPolygon<double>;
	    const SbList<SbVec2f> sopolygon = selector_->getPolygon();
	    for ( int idx=0; idx<sopolygon.getLength(); idx++ )
		polygon_->add( Coord( sopolygon[idx][0], sopolygon[idx][1] ) );
	}

	polygonlock_.convToReadLock();
    }

    const bool res = polygon_->isInside( checkcoord2d, true, 1e-3 );
    polygonlock_.readUnLock();

    return res;
}


void PolygonSelection::polygonChangeCB( void* data, SoPolygonSelect* )
{
    PolygonSelection* myptr = (PolygonSelection*) data;
    myptr->polygonlock_.writeLock();
    delete myptr->polygon_;
    myptr->polygon_ = 0;

    myptr->polygonlock_.writeUnLock();
}


void PolygonSelection::setDisplayTransformation( Transformation* nt )
{
    if ( transformation_ ) transformation_->unRef();
    transformation_ = nt;
    if ( transformation_ ) transformation_->ref();
}


Transformation* PolygonSelection::getDisplayTransformation()
{ return transformation_; }


}; // namespace visBase
