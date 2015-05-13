/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "basemappolygon.h"

#include "ioman.h"
#include "ioobj.h"
#include "pickset.h"
#include "picksettr.h"
#include "ptrman.h"

namespace Basemap
{

PolygonObject::PolygonObject( const MultiID& mid )
    : BaseMapObject(0)
    , polygonmid_(mid)
    , ps_(*new Pick::Set)
{
    setMultiID( mid );
}


PolygonObject::~PolygonObject()
{
    delete &ps_;
}


void PolygonObject::setMultiID( const MultiID& mid )
{
    polygonmid_ = mid;
    PtrMan<IOObj> ioobj = IOM().get( polygonmid_ );
    if ( !ioobj ) return;

    setName( ioobj->name() );
    ps_.setEmpty();
    BufferString errmsg;
    PickSetTranslator::retrieve( ps_, ioobj, true, errmsg );
    ms_.color_ = ps_.disp_.color_;
    ms_.type_ = MarkerStyle2D::Circle;
    ls_.color_ = ms_.color_;
}


void PolygonObject::updateGeometry()
{ changed.trigger(); }


int PolygonObject::nrShapes() const	{ return 1; }

const char* PolygonObject::getShapeName( int ) const
{ return name().buf(); }


void PolygonObject::getPoints( int, TypeSet<Coord>& pts ) const
{
    for ( int idx=0; idx<ps_.size(); idx++ )
	pts += ps_[idx].pos_;

    if ( ps_.disp_.connect_==Pick::Set::Disp::Close && !pts.isEmpty() )
	pts += pts[0];
}


const MarkerStyle2D* PolygonObject::getMarkerStyle( int ) const
{ return &ms_; }

void PolygonObject::setMarkerStyle( int, const MarkerStyle2D& ms )
{ ms_ = ms; }

const LineStyle* PolygonObject::getLineStyle( int ) const
{ return &ls_; }

void PolygonObject::setLineStyle( int, const LineStyle& ls )
{ ls_ = ls; }

} // namespace Basemap
