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

SeisOutlineObject::SeisOutlineObject()
    : BaseMapObject(0)
    , color_(Color::NoColor())
    , seismid_(MultiID::udf())
    , linespacing_(StepInterval<int>::udf())
    , ls_(*new LineStyle)
    , seisarea_(*new TrcKeySampling)
    , fullyrect_(false)
    , nrsegments_(0)
{}


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

void SeisOutlineObject::setFillColor(int,const Color& color)
{
    color_ = color;
    stylechanged.trigger();
}


void SeisOutlineObject::setInsideLines( const StepInterval<int>& linespacing )
{
    linespacing_ = linespacing;
}


void SeisOutlineObject::setLineStyle( int shapeidx, const LineStyle& ls )
{
    ls_ = ls;
    stylechanged.trigger();
}


void SeisOutlineObject::updateGeometry()
{
    changed.trigger();
}


int SeisOutlineObject::nrShapes() const
{ return fullyrect_ ? (1 + nrsegments_) :
			(polygons_.size() + nrsegments_); }


bool SeisOutlineObject::close( int idx ) const
{
    return fullyrect_ ? idx == 0 : idx < polygons_.size();
}


Alignment SeisOutlineObject::getAlignment( int shapeidx ) const
{
    return Alignment( Alignment::HCenter, Alignment::VCenter );
}


const Color SeisOutlineObject::getFillColor(int idx) const
{
    return color_;
}


const char* SeisOutlineObject::getShapeName( int idx ) const
{
    if ( ( fullyrect_ && idx == 1 ) || polygons_.validIdx(idx) )
	return name().buf();

    const int lineidx = fullyrect_ ? (idx - 1) :
				     (idx - polygons_.size());
    mDeclStaticString( str );
    str.set( linespacing_.atIndex(lineidx) );
    return str.buf();
}


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

    extractSegments( cubedata );

    IsoContourTracer outline( area );
    outline.setBendPointsOnly( 0.1 );
    return outline.getContours( polygons_, 0.9f );
}


bool SeisOutlineObject::extractSegments( PosInfo::CubeData& cubedata )
{
    if ( linespacing_.isUdf() ) return false;

    SegmentLine segline;
    for ( int inlidx=linespacing_.start; inlidx<=linespacing_.stop;
						    inlidx+=linespacing_.step )
    {
	PosInfo::LineData* ld = cubedata[cubedata.indexOf(inlidx)];
	for ( int sidx=0; sidx<ld->segments_.size(); sidx++ )
	{
	    const StepInterval<int>& crlrg = ld->segments_[sidx];
	    segline.end1 = SI().transform( BinID(ld->linenr_,crlrg.start) );
	    segline.end2 = SI().transform( BinID(ld->linenr_,crlrg.stop) );
	    seglineset_ += segline;
	}

    }
    nrsegments_ = seglineset_.size();
    return true;
}


void SeisOutlineObject::getPoints( int shapeidx, TypeSet<Coord>& pts ) const
{
    if ( fullyrect_ && shapeidx == 1 )
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

    const int segmentidx = fullyrect_ ? (shapeidx - 1) :
					(shapeidx - polygons_.size());

    if ( segmentidx >= 0 )
    {
	pts.add( seglineset_[segmentidx].end1 );
	pts.add( seglineset_[segmentidx].end2 );
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

} // namespace Basemap
