/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2018
-*/


#include "survsubgeom.h"
#include "survgeom3d.h"
#include "survinfo.h"

typedef Survey::SubGeometry3D::pos_idx_type		pos_idx_type;
typedef Survey::SubGeometry3D::idx_type			idx_type;
typedef Survey::SubGeometry3D::pos_idx_range_type	pos_idx_range_type;


Survey::SubGeometry3D::SubGeometry3D()
    : Survey::SubGeometry3D(*Survey::Geometry::default3D().as3D())
{
}


Survey::SubGeometry3D::SubGeometry3D( const Geometry3D& g3d )
    : survgeom_(&g3d)
    , offs_(0,0)
    , step_(1,1)
    , sz_(g3d.inlRange().width()+1,g3d.crlRange().width()+1)
{
}


Survey::SubGeometry3D::SubGeometry3D( const SubGeometry3D& oth )
{
    survgeom_ = oth.survgeom_;
    offs_ = oth.offs_;
    step_ = oth.step_;
    sz_ = oth.sz_;
}


Survey::SubGeometry3D& Survey::SubGeometry3D::operator =(
						const SubGeometry3D& oth )
{
    if ( this != &oth )
    {
	survgeom_ = oth.survgeom_;
	offs_ = oth.offs_;
	step_ = oth.step_;
	sz_ = oth.sz_;
    }
    return *this;
}


const Survey::Geometry* Survey::SubGeometry3D::surveyGeometry() const
{
    return survgeom_;
}


Survey::SubGeometry* Survey::SubGeometry3D::clone() const
{
    return new SubGeometry3D( *this );
}


bool Survey::SubGeometry3D::isFull() const
{
    return inlRange() == survInlRange() && crlRange() == survCrlRange();
}


bool Survey::SubGeometry3D::hasOffset() const
{
    return offs_.row() || offs_.col();
}


bool Survey::SubGeometry3D::isSubSpaced() const
{
    return step_.row()>1 || step_.col()>1;
}


bool Survey::SubGeometry3D::hasFullArea() const
{
    const auto survinlrg = survInlRange();
    const auto survcrlrg = survCrlRange();
    return inlStart() == survinlrg.start && crlStart() == survcrlrg.start
	&& inlStop() == survinlrg.stop   && crlStop() == survcrlrg.stop;
}


BinID Survey::SubGeometry3D::origin() const
{
    return BinID( inlStart(), crlStart() );
}


pos_idx_type Survey::SubGeometry3D::inlStart() const
{
    return survgeom_->inlStart() + inlStep() * offs_.row();
}


pos_idx_type Survey::SubGeometry3D::crlStart() const
{
    return survgeom_->crlStart() + crlStep() * offs_.col();
}


pos_idx_type Survey::SubGeometry3D::inlStep() const
{
    return survgeom_->inlStep() * step_.row();
}


pos_idx_type Survey::SubGeometry3D::crlStep() const
{
    return survgeom_->crlStep() * step_.col();
}


pos_idx_type Survey::SubGeometry3D::inlStop() const
{
    return inlStart() + inlStep() * (sz_.row()-1);
}


pos_idx_type Survey::SubGeometry3D::crlStop() const
{
    return crlStart() + crlStep() * (sz_.col()-1);
}


pos_idx_range_type Survey::SubGeometry3D::inlRange() const
{
    return pos_idx_range_type( inlStart(), inlStop(), inlStep() );
}


pos_idx_range_type Survey::SubGeometry3D::crlRange() const
{
    return pos_idx_range_type( crlStart(), crlStop(), crlStep() );
}


pos_idx_range_type Survey::SubGeometry3D::survInlRange() const
{
    return survgeom_->inlRange();
}


pos_idx_range_type Survey::SubGeometry3D::survCrlRange() const
{
    return survgeom_->crlRange();
}


idx_type Survey::SubGeometry3D::idx4Inl( pos_idx_type inln ) const
{
    return (inln - inlStart()) / inlStep();
}


idx_type Survey::SubGeometry3D::idx4Crl( pos_idx_type crln ) const
{
    return (crln - crlStart()) / crlStep();
}


pos_idx_type Survey::SubGeometry3D::inl4Idx( idx_type irow ) const
{
    return inlStart() + inlStep() * irow;
}


pos_idx_type Survey::SubGeometry3D::crl4Idx( idx_type icol ) const
{
    return crlStart() + crlStep() * icol;
}


RowCol Survey::SubGeometry3D::idxs4BinID( const BinID& bid ) const
{
    RowCol ret = bid - origin();
    ret.row() /= inlStep();
    ret.col() /= crlStep();
    return ret;
}


BinID Survey::SubGeometry3D::binid4Idxs( idx_type irow, idx_type icol ) const
{
    return BinID( inlStart() + inlStep() * irow,
		  crlStart() + crlStep() * icol );
}


BinID Survey::SubGeometry3D::binid4Idxs( const RowCol& rc ) const
{
    return binid4Idxs( rc.row(), rc.col() );
}


void Survey::SubGeometry3D::setRange( const BinID& start, const BinID& stop,
				      RowCol stp )
{
    const Survey::SubGeometry3D fullsl( *survgeom_ );
    offs_ = fullsl.idxs4BinID( start );
    step_ = stp;
    const auto stoprc = idxs4BinID( stop );
    sz_ = RowCol( stoprc.row()+1, stoprc.col()+1 );
}


void Survey::SubGeometry3D::setRange( const BinID& start, const BinID& stop,
				      const BinID& stepbid )
{
    step_.row() = stepbid.inl() / survgeom_->inlStep();
    if ( step_.row() < 1 )
	{ pErrMsg("Bad inl step"); step_.row() = 1; }
    step_.col() = stepbid.crl() / survgeom_->crlStep();
    if ( step_.col() < 1 )
	{ pErrMsg("Bad crl step"); step_.col() = 1; }

    offs_ = RowCol( 0, 0 );
    const auto stoprc = idxs4BinID( stop );
    offs_ = idxs4BinID( start );

    sz_ = RowCol( stoprc.row()-offs_.row()+1, stoprc.col()-offs_.col()+1 );
}
