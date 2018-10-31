/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2018
-*/


#include "sublattice.h"
#include "survgeom3d.h"
typedef StepInterval<int> IdxRange;


SubLattice::SubLattice()
    : SubLattice(*Survey::Geometry::default3D().as3D())
{
}


SubLattice::SubLattice( const Geometry3D& g3d )
    : offs_(0,0)
    , step_(1,1)
    , sz_(g3d.inlRange().width()+1,g3d.crlRange().width()+1)
{
}


SubLattice::SubLattice( const SubLattice& oth )
{
    survgeom_ = oth.survgeom_;
    offs_ = oth.offs_;
    step_ = oth.step_;
    sz_ = oth.sz_;
}


SubLattice& SubLattice::operator =( const SubLattice& oth )
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


bool SubLattice::hasOffset() const
{
    return offs_.row() || offs_.col();
}


bool SubLattice::isSubSpaced() const
{
    return step_.row()>1 || step_.col()>1;
}


bool SubLattice::hasFullArea() const
{
    const auto survinlrg = survInlRange();
    const auto survcrlrg = survCrlRange();
    return inlStart() == survinlrg.start && crlStart() == survcrlrg.start
	&& inlStop() == survinlrg.stop   && crlStop() == survcrlrg.stop;
}


bool SubLattice::isFull() const
{
    return inlRange() == survInlRange() && crlRange() == survCrlRange();
}


IdxRange SubLattice::survInlRange() const
{
    return survgeom_->sampling().hsamp_.inlRange();
}


IdxRange SubLattice::survCrlRange() const
{
    return survgeom_->sampling().hsamp_.crlRange();
}


int SubLattice::inlStart() const
{
    return survgeom_->sampling().hsamp_.start_.inl() + step_.row()*offs_.row();
}


int SubLattice::crlStart() const
{
    return survgeom_->sampling().hsamp_.start_.crl() + step_.col()*offs_.col();
}


int SubLattice::inlStep() const
{
    return survgeom_->sampling().hsamp_.step_.inl() * step_.row();
}


int SubLattice::crlStep() const
{
    return survgeom_->sampling().hsamp_.step_.crl() * step_.col();
}


int SubLattice::inlStop() const
{
    return inlStart() + inlStep() * (sz_.row()-1);
}


int SubLattice::crlStop() const
{
    return crlStart() + crlStep() * (sz_.col()-1);
}


IdxRange SubLattice::inlRange() const
{
    return IdxRange( inlStart(), inlStop(), inlStep() );
}


IdxRange SubLattice::crlRange() const
{
    return IdxRange( crlStart(), crlStop(), crlStep() );
}


int SubLattice::getInl( int irow ) const
{
    return inlStart() + inlStep() * irow;
}


int SubLattice::getRow( int inln ) const
{
    return (inln - inlStart()) / inlStep();
}


int SubLattice::getCrl( int icol ) const
{
    return crlStart() + crlStep() * icol;
}


int SubLattice::getCol( int crln ) const
{
    return (crln - crlStart()) / crlStep();
}


BinID SubLattice::origin() const
{
    return BinID( inlStart(), crlStart() );
}


RowCol SubLattice::getIndexes( const BinID& bid ) const
{
    RowCol ret = bid - origin();
    ret.row() /= inlStep();
    ret.col() /= crlStep();
    return ret;
}


BinID SubLattice::getBinID( int irow, int icol ) const
{
    return BinID( inlStart() + inlStep() * irow,
		  crlStart() + crlStep() * icol );
}


BinID SubLattice::getBinID( const RowCol& rc ) const
{
    return getBinID( rc.row(), rc.col() );
}


void SubLattice::setRange( const BinID& start, const BinID& stop,
			   RowCol substeps )
{
    const SubLattice fullsl( *survgeom_ );
    offs_ = fullsl.getIndexes( start );
    step_ = substeps;
    const RowCol stoprc = fullsl.getIndexes( stop );
    sz_ = RowCol( (stoprc.row()-offs_.row())/step_.row() + 1,
		  (stoprc.col()-offs_.col())/step_.col() + 1 );
}
