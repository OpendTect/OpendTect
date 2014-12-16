/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		December 2014
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "basemapoutline.h"

#include "arrayndimpl.h"
#include "draw.h"
#include "ioman.h"
#include "ioobj.h"
#include "isocontourtracer.h"
#include "ptrman.h"
#include "seisioobjinfo.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "survinfo.h"


namespace Basemap
{

OutlineObject::OutlineObject( const MultiID& mid )
    : BaseMapObject(0)
    , seismid_(mid)
    , ls_(*new LineStyle)
    , seisarea_(*new TrcKeySampling)
{
    setMultiID( mid );
}


OutlineObject::~OutlineObject()
{
    delete &ls_;
    delete &seisarea_;
    deepErase( polygons_ );
}


void OutlineObject::setMultiID( const MultiID& mid )
{
    seismid_ = mid;
    PtrMan<IOObj> ioobj = IOM().get( seismid_ );
    if ( !ioobj ) return;

    setName( ioobj->name() );
    extractPolygons();

    updateGeometry();
}


void OutlineObject::updateGeometry()
{
    changed.trigger();
}


int OutlineObject::nrShapes() const
{ return polygons_.size(); }


const char* OutlineObject::getShapeName(int) const
{ return name().buf(); }


bool OutlineObject::extractPolygons()
{
    TrcKeyZSampling tkzs;
    const SeisIOObjInfo info( seismid_ );
    if ( !info.getRanges(tkzs) )
	return false;

    if ( info.isFullyRectAndRegular() )
    {
	ODPolygon<float>* polygon = new ODPolygon<float>();
	polygon->setClosed( false );

	const Coord& startpt = SI().transform(tkzs.hrg.start_);
	const Coord& stoppt = SI().transform(tkzs.hrg.stop_);

	typedef Geom::Point2D<float> Point;

	polygon->insert( 0, Point(mCast(float,startpt.x),
				  mCast(float,startpt.y)) );
	polygon->insert( 1, Point(mCast(float,stoppt.x),
				  mCast(float,startpt.y)) );
	polygon->insert( 2, Point(mCast(float,stoppt.x),
				  mCast(float,stoppt.y)) );
	polygon->insert( 3, Point(mCast(float,startpt.x),
				  mCast(float,stoppt.y)) );

	polygons_.insertAt( polygon, 0 );
	return true;
    }
    // creates matrix of 0.0 and 1.0
    // area[x][y] == 1.0 => valid trace
    // area[x][y] == 0.0 => invalid trace
    seisarea_ = tkzs.hsamp_;
    const int lines = seisarea_.nrLines();
    const int traces = seisarea_.nrTrcs();
    // line below allow data on the edge to be drawn
    Array2DImpl<float> area( lines+2 , traces+2 );
    area.setAll( 0.0f );

    PtrMan<IOObj> seisobj = IOM().get( seismid_ );
    if ( !seisobj ) return false;

    SeisTrcReader seistrcrdr( seisobj );
    seistrcrdr.prepareWork( Seis::Prod );
    seistrcrdr.setSelData( new Seis::RangeSelData(tkzs) );
    SeisTrcInfo si;

    while ( true )
    {
	const int res = seistrcrdr.get( si );
	if ( res == 0 ) break;
	if ( res > 1 ) continue;
	if ( res == 1 )
	{
	    const BinID bid = si.binid;
	    const int lineidx = seisarea_.lineIdx( bid.inl() );
	    const int trcidx = seisarea_.trcIdx( bid.crl() );
	    area.set( lineidx + 1, trcidx + 1, 1.0f );
	}
	if ( res < 0 ) return false;
    }

    IsoContourTracer outline( area );
    outline.setBendPointsOnly( 0.5 );
    return outline.getContours( polygons_, 0.9f );
}


void OutlineObject::getPoints( int shapeidx, TypeSet<Coord>& pts ) const
{
    if ( !polygons_.validIdx(shapeidx) )
	return;

    const ODPolygon<float>& poly = *polygons_[shapeidx];
    for ( int idy=0; idy<poly.size(); idy++ )
    {
	const Geom::Point2D<float>& pt = poly.getVertex( idy );
	const BinID bid = seisarea_.atIndex( mNINT32(pt.x), mNINT32(pt.y) );
	pts.add( SI().transform(bid) );
    }
}


} // namepace Basemap
