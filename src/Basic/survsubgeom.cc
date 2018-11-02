/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2018
-*/


#include "survsubgeom.h"
#include "survgeom3d.h"
#include "survinfo.h"

typedef Survey::SubGeometryHDimData::pos_type	pos_type;
typedef Survey::SubGeometryHDimData::idx_type		idx_type;
typedef Survey::SubGeometryHDimData::size_type		size_type;
typedef Survey::SubGeometryHDimData::pos_range_type	pos_range_type;


Survey::SubGeometryHDimData::SubGeometryHDimData( const pos_range_type& rg )
    : fullposrg_(0)
{
    setFullPosRange( rg );
}


void Survey::SubGeometryHDimData::setFullPosRange( const pos_range_type& rg )
{
    fullposrg_ = &rg;
    sz_ = fullposrg_->nrSteps() + 1;
}


bool Survey::SubGeometryHDimData::operator ==(
					const SubGeometryHDimData& oth ) const
{
    return *fullposrg_ == *oth.fullposrg_
	&& offs_ == oth.offs_
	&& step_ == oth.step_
	&& sz_ == oth.sz_;
}


bool Survey::SubGeometryHDimData::hasFullRange() const
{
    return posStart() == fullposrg_->start && posStop() == fullposrg_->stop;
}



pos_type Survey::SubGeometryHDimData::posStart() const
{
    return fullposrg_->start + posStep() * offs_;
}


pos_type Survey::SubGeometryHDimData::posStep() const
{
    return fullposrg_->step * step_;
}


pos_type Survey::SubGeometryHDimData::posStop() const
{
    return posStart() + posStep() * (sz_-1);
}


pos_range_type Survey::SubGeometryHDimData::posRange() const
{
    return pos_range_type( posStart(), posStop(), posStep() );
}


idx_type Survey::SubGeometryHDimData::idx4Pos( pos_type pos ) const
{
    return (pos - posStart()) / posStep();
}


pos_type Survey::SubGeometryHDimData::pos4Idx( idx_type idx ) const
{
    return posStart() + posStep() * idx;
}


void Survey::SubGeometryHDimData::setPosRange( pos_type start,
				pos_type stop, pos_type stp )
{
    step_ = stp / fullposrg_->step;
    if ( step_ < 1 )
	{ pErrMsg("Bad pos step"); step_ = 1; }

    offs_ = 0;
    const idx_type stopidx = idx4Pos( stop );
    offs_ = idx4Pos( start );

    sz_ = stopidx - offs_ + 1;
}



Survey::SubGeometry::SubGeometry( const Geometry& geom )
    : trcdd_(geom.trcNrRange())
{
}


bool Survey::SubGeometry::operator ==( const SubGeometry& oth ) const
{
    return trcdd_ == oth.trcdd_;
}



Survey::SubGeometry3D::SubGeometry3D( const Geometry3D& g3d )
    : SubGeometry(g3d)
    , survgeom_(&g3d)
    , inldd_(g3d.inlRange())
{
}


Survey::SubGeometry3D::SubGeometry3D()
    : SubGeometry3D(*Survey::Geometry::default3D().as3D())
{
}


bool Survey::SubGeometry3D::operator ==( const SubGeometry3D& oth ) const
{
    return SubGeometry::operator ==(oth)
	&& survgeom_ == oth.survgeom_
	&& inldd_ == oth.inldd_;
}


const Survey::Geometry* Survey::SubGeometry3D::surveyGeometry() const
{
    return survgeom_;
}


Survey::SubGeometry* Survey::SubGeometry3D::clone() const
{
    return new SubGeometry3D( *this );
}


bool Survey::SubGeometry3D::isEmpty() const
{
    return trcdd_.isEmpty() || inldd_.isEmpty();
}


bool Survey::SubGeometry3D::isFull() const
{
    return trcdd_.isFull() && inldd_.isFull();
}


bool Survey::SubGeometry3D::hasOffset() const
{
    return trcdd_.hasOffset() || inldd_.hasOffset();
}


bool Survey::SubGeometry3D::isSubSpaced() const
{
    return trcdd_.hasOffset() || inldd_.isSubSpaced();
}


bool Survey::SubGeometry3D::hasFullRange() const
{
    return trcdd_.hasFullRange() && inldd_.hasFullRange();
}


RowCol Survey::SubGeometry3D::offset() const
{
    return RowCol( inldd_.offset(), trcdd_.offset() );
}


RowCol Survey::SubGeometry3D::step() const
{
    return RowCol( inldd_.step(), trcdd_.step() );
}


RowCol Survey::SubGeometry3D::size() const
{
    return RowCol( inldd_.size(), trcdd_.size() );
}


BinID Survey::SubGeometry3D::origin() const
{
    return BinID( inlStart(), crlStart() );
}


size_type Survey::SubGeometry3D::nrRows() const
{
    return inldd_.size();
}


size_type Survey::SubGeometry3D::nrCols() const
{
    return trcdd_.size();
}


idx_type Survey::SubGeometry3D::idx4Inl( pos_type pos ) const
{
    return inldd_.idx4Pos( pos );
}


idx_type Survey::SubGeometry3D::idx4Crl( pos_type pos ) const
{
    return trcdd_.idx4Pos( pos );
}


pos_type Survey::SubGeometry3D::inl4Idx( idx_type idx ) const
{
    return inldd_.pos4Idx( idx );
}


pos_type Survey::SubGeometry3D::crl4Idx( idx_type idx ) const
{
    return trcdd_.pos4Idx( idx );
}


RowCol Survey::SubGeometry3D::idxs4BinID( const BinID& bid ) const
{
    return RowCol( idx4Inl(bid.inl()), idx4Crl(bid.crl()) );
}


BinID Survey::SubGeometry3D::binid4Idxs( const RowCol& rc ) const
{
    return BinID( inl4Idx(rc.row()), crl4Idx(rc.col()) );
}


Coord Survey::SubGeometry3D::coord4Idxs( const RowCol& rc ) const
{
    return coord4BinID( binid4Idxs(rc) );
}


Coord Survey::SubGeometry3D::coord4BinID( const BinID& bid ) const
{
    return survgeom_->toCoord( bid.inl(), bid.crl() );
}


RowCol Survey::SubGeometry3D::nearestIdxs( const Coord& crd ) const
{
    return idxs4BinID( nearestBinID(crd) );
}


BinID Survey::SubGeometry3D::nearestBinID( const Coord& crd ) const
{
    return survgeom_->nearestTracePosition( crd, 0 );
}


pos_range_type Survey::SubGeometry3D::survInlRange() const
{
    return survgeom_->inlRange();
}


pos_range_type Survey::SubGeometry3D::survCrlRange() const
{
    return survgeom_->crlRange();
}


void Survey::SubGeometry3D::setSurvGeom( const Geometry& geom )
{
    mDynamicCastGet( const Geometry3D*, g3d, &geom )
    if ( !g3d )
	{ pErrMsg("Bad geom"); }
    else
    {
	survgeom_ = g3d;
	inldd_.setFullPosRange( g3d->inlRange() );
	trcdd_.setFullPosRange( g3d->crlRange() );
    }
}


void Survey::SubGeometry3D::setRange( const BinID& start, const BinID& stop,
				      RowCol steps )
{
    setRange( start, stop, BinID(inldd_.posStep()*steps.row(),
				 trcdd_.posStep()*steps.col()) );
}


void Survey::SubGeometry3D::setRange( const BinID& start, const BinID& stop,
				      const BinID& stepbid )
{
    inldd_.setPosRange( start.inl(), stop.inl(), stepbid.inl() );
    trcdd_.setPosRange( start.crl(), stop.crl(), stepbid.crl() );
}
