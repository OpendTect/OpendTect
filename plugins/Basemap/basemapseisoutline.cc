/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		December 2014
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "basemapseisoutline.h"

#include "arrayndimpl.h"
#include "draw.h"
#include "ioman.h"
#include "ioobj.h"
#include "isocontourtracer.h"
#include "posinfo.h"
#include "ptrman.h"
#include "seisioobjinfo.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "survinfo.h"

namespace Basemap
{

SeisOutlineObject::SeisOutlineObject( const MultiID& mid )
    : BaseMapObject(0)
    , seismid_(mid)
    , ls_(*new LineStyle)
    , seisarea_(*new TrcKeySampling)
    , fullyrect_(false)
{
    setMultiID( mid );
}


SeisOutlineObject::~SeisOutlineObject()
{
    delete &ls_;
    delete &seisarea_;
    deepErase( polygons_ );
}


void SeisOutlineObject::setMultiID( const MultiID& mid )
{
    seismid_ = mid;
    PtrMan<IOObj> ioobj = IOM().get( seismid_ );
    if ( !ioobj ) return;

    setName( ioobj->name() );
    extractPolygons();
}


void SeisOutlineObject::setLineStyle( const LineStyle& ls )
{
    ls_ = ls;
}


void SeisOutlineObject::setLineStyle( int shapeidx, const LineStyle& ls )
{
    setLineStyle( ls );
}


void SeisOutlineObject::updateGeometry()
{
    changed.trigger();
}


int SeisOutlineObject::nrShapes() const
{ return fullyrect_ ? 1 : polygons_.size(); }


const char* SeisOutlineObject::getShapeName(int) const
{ return name().buf(); }


bool SeisOutlineObject::extractPolygons()
{
    const SeisIOObjInfo ioobjinfo( seismid_ );

    TrcKeyZSampling tkzs;
    if ( !ioobjinfo.getRanges(tkzs) ) return false;
    seisarea_ = tkzs.hsamp_;

    if ( ioobjinfo.isFullyRectAndRegular() ) return fullyrect_ = true;

    const int lines = seisarea_.nrLines();
    const int traces = seisarea_.nrTrcs();
    // creates matrix of 0.0 and 1.0
    // area[x][y] == 1.0 => valid trace
    // area[x][y] == 0.0 => invalid trace
    Array2DImpl<float> area( lines+2 , traces+2 );
    area.setAll( 0.0f );
    // +2 allow data on the edge to be drawn

    SeisTrcReader rdr( ioobjinfo.ioObj() );

    PosInfo::CubeData cubedata;
    if ( !rdr.get3DGeometryInfo(cubedata) ) return false;

    PosInfo::CubeDataPos cubedatapos;
    while ( cubedata.toNext(cubedatapos) )
    {
	const BinID bid = cubedata.binID( cubedatapos );
	const int lineidx = seisarea_.lineIdx( bid.inl() );
	const int trcidx = seisarea_.trcIdx( bid.crl() );
	area.set( lineidx+1, trcidx+1, 1.0f );
    }

    IsoContourTracer outline( area );
    outline.setBendPointsOnly( 0.1 );
    return outline.getContours( polygons_, 0.9f );
}


void SeisOutlineObject::getPoints( int shapeidx, TypeSet<Coord>& pts ) const
{
    if ( fullyrect_ )
    {
	const Coord startpt = SI().transform(seisarea_.start_);
	const Coord stoppt = SI().transform(seisarea_.stop_);

	pts.add( startpt );
	pts.add( Coord(stoppt.x,startpt.y) );
	pts.add( stoppt );
	pts.add( Coord(startpt.x,stoppt.y) );
	pts.add( startpt );

	return;
    }

    if ( !polygons_.validIdx(shapeidx) ) return;

    const ODPolygon<float>& poly = *polygons_[shapeidx];
    for ( int idy=0; idy<poly.size(); idy++ )
    {
	const Geom::Point2D<float>& pt = poly.getVertex( idy );
	const BinID bid = seisarea_.atIndex( mNINT32(pt.x), mNINT32(pt.y) );
	pts.add( SI().transform(bid) );
    }
}

} // namepace Basemap
