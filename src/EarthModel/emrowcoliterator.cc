/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emrowcoliterator.h"

#include "trckeyzsampling.h"
#include "emsurface.h"
#include "rowcolsurface.h"
#include "survinfo.h"
#include "typeset.h"

namespace EM
{


RowColIterator::RowColIterator( const Surface& surf,
				const TrcKeyZSampling* tkzs )
    : surf_(surf)
    , csbound_(tkzs)
    , rowcolbounded_(false)
{
    fillPosIDs();
}


RowColIterator::RowColIterator( const Surface& surf,
				const StepInterval<int> rowbnd,
				const StepInterval<int> colbnd )
    : surf_(surf)
    , rowbound_(rowbnd)
    , colbound_(colbnd)
    , rowcolbounded_(true)
{
    fillPosIDs();
}


RowColIterator::~RowColIterator()
{
    deepErase( posids_ );
}


void RowColIterator::fillPosIDs()
{
    if ( !posids_.isEmpty() )
	posids_.setEmpty();

    TypeSet<GeomPosID>* posids = new TypeSet<GeomPosID>;
    posids_ += posids;

    mDynamicCastGet(const Geometry::RowColSurface*,rcs,
		    surf_.geometryElement())
    if ( !rcs ) return;

    rcs->getPosIDs( *posids );
}


PosID RowColIterator::next()
{
    while ( true )
    {
	if ( !cursection_ )
	{
	    if ( !initSection() )
		return PosID::udf();
	}
	else
	{
	    rc_.col() += colrg_.step_;
	    if ( !colrg_.includes(rc_.col(),true) )
	    {
		rc_.row() += rowrg_.step_;
		if ( !rowrg_.includes(rc_.row(),true) )
		{
		    cursection_ = 0;
		    return PosID::udf();
		}

		colrg_ = cursection_->colRange( rc_.row() );
		if ( rowcolbounded_ )
		    colrg_.limitTo( colbound_ );
		rc_.col() = colrg_.start_;
	    }
	}
	if ( !cursection_->isKnotDefined( rc_ ) )
	    continue;

	if ( !csbound_ )
	    break;

	pos_ = surf_.getPos( rc_.toInt64() );
	bid_ = SI().transform( pos_ );

	if ( csbound_->hsamp_.includes(bid_) &&
	     csbound_->zsamp_.includes(pos_.z,false) )
	    break;
    }

    return PosID( surf_.id(), rc_ );
}


PosID RowColIterator::fromIndex( int idx ) const
{
    for ( int ids=0; ids<posids_.size(); ids++ )
    {
	const TypeSet<GeomPosID>& posids = *posids_[ids];
	if ( idx > posids.size() )
	    idx -= posids.size();
	else
	    return PosID( surf_.id(), posids[idx] );
    }

    return PosID::udf();
}


int RowColIterator::maxIndex() const
{
    int sum = 0;
    for ( int idx=0; idx<posids_.size(); idx++ )
    {
	const TypeSet<GeomPosID>& posids = *posids_[idx];
	sum += posids.size();
    }
    return sum;
}


int RowColIterator::maximumSize() const
{
    mDynamicCastGet(const Geometry::RowColSurface*,rcs,
		    surf_.geometryElement())
    if ( !rcs )
	return 0;

    StepInterval<int> rowrg = rcs->rowRange();
    if ( rowcolbounded_ )
	rowrg.limitTo( rowbound_ );

    int res = 0;
    for ( int row=rowrg.start_; row<=rowrg.stop_; row+=rowrg.step_ )
    {
	StepInterval<int> colrg = rcs->colRange( row );
	if ( rowcolbounded_ )
	    colrg.limitTo( colbound_ );
	res += colrg.nrSteps()+1;
    }

    return res;
}


bool RowColIterator::initSection()
{
    mDynamicCastGet(const Geometry::RowColSurface*,rcs,
		    surf_.geometryElement())
    if ( !rcs )
	return false;

    cursection_ = rcs;

    rowrg_ = rcs->rowRange();
    if ( rowrg_.stop_ < rowrg_.start_ )
	return false;
    colrg_ = rcs->colRange( rowrg_.start_ );
    if ( colrg_.stop_ < colrg_.start_ )
	return false;

    if ( rowcolbounded_ )
    {
	rowrg_.limitTo( rowbound_ );
	colrg_.limitTo( colbound_ );
    }

    rc_.row() = rowrg_.start_;
    rc_.col() = colrg_.start_;
    return true;
}

} // namespace EM
