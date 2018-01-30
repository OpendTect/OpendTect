/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/

#include "emrowcoliterator.h"

#include "trckeyzsampling.h"
#include "emsurface.h"
#include "rowcolsurface.h"
#include "survinfo.h"

namespace EM
{

RowColIterator::RowColIterator( const Surface& surf,
				const TrcKeyZSampling* cs )
    : surf_( surf )
    , surfgeom_( 0 )
    , csbound_( cs )
    , rowcolbounded_( false )
{
}


RowColIterator::RowColIterator( const Surface& surf,
				const StepInterval<int> rowbnd,
				const StepInterval<int> colbnd )
    : surf_( surf )
    , surfgeom_( 0 )
    , csbound_( 0 )
    , rowcolbounded_( true )
    , rowbound_( rowbnd )
    , colbound_( colbnd )
{
}


PosID RowColIterator::next()
{
    while ( true )
    {
	if ( !surfgeom_ )
	{
	    if ( !init() )
		return PosID::getInvalid();
	}
	else
	{
	    rc_.col() += colrg_.step;
	    if ( !colrg_.includes(rc_.col(),true) )
	    {
		rc_.row() += rowrg_.step;
		if ( !rowrg_.includes(rc_.row(),true) )
		{
		    surfgeom_ = 0;
		    return PosID::getInvalid();
		}

		colrg_ = surfgeom_->colRange( rc_.row() );
		if ( rowcolbounded_ )
		    colrg_.limitTo( colbound_ );
		rc_.col() = colrg_.start;
	    }
	}
	if ( !surfgeom_->isKnotDefined( rc_ ) )
	    continue;

	if ( !csbound_ )
	    break;

	pos_ = surf_.getPos( PosID::getFromRowCol(rc_) );
	bid_ = SI().transform( pos_.getXY() );

	if ( csbound_->hsamp_.includes(bid_) &&
	     csbound_->zsamp_.includes(pos_.z_,false) )
	    break;
    }

    return PosID::getFromRowCol( rc_ );
}


int RowColIterator::maximumSize() const
{
    mDynamicCastGet( const Geometry::RowColSurface*, rcs,
		     surf_.geometryElement() );
    if ( !rcs ) return 0;

    StepInterval<int> rowrg = rcs->rowRange();
    if ( rowcolbounded_ )
	rowrg.limitTo( rowbound_ );

    int res = 0;
    for ( int row=rowrg.start; row<=rowrg.stop; row+=rowrg.step )
    {
	StepInterval<int> colrg = rcs->colRange( row );
	if ( rowcolbounded_ )
	    colrg.limitTo( colbound_ );
	res += colrg.nrSteps()+1;
    }

    return res;
}


bool RowColIterator::init()
{
    mDynamicCastGet( const Geometry::RowColSurface*, rcs,
		     surf_.geometryElement() );
    if ( !rcs )
	return false;

    surfgeom_ = rcs;

    rowrg_ = rcs->rowRange();
    if ( rowrg_.stop < rowrg_.start )
	return false;
    colrg_ = rcs->colRange( rowrg_.start );
    if ( colrg_.stop < colrg_.start )
	return false;

    if ( rowcolbounded_ )
    {
	rowrg_.limitTo( rowbound_ );
	colrg_.limitTo( colbound_ );
    }

    rc_.row() = rowrg_.start;
    rc_.col() = colrg_.start;
    return true;
}

} // namespace EM
