/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "rowcolsurface.h"

#include <limits.h>


namespace Geometry
{

class RowColSurfaceIterator : public Iterator
{
public:
    RowColSurfaceIterator( const RowColSurface& ps )
	: curpos_( -1, -1 )
	, rowrg_( ps.rowRange() )
	, colrg_( ps.colRange() )
    {}

    GeomPosID           next()
    {
	if ( curpos_.row==-1 )
	{
	    curpos_.row = rowrg_.start;
	    curpos_.col = colrg_.start;
	}
    else
    {
	curpos_.col += colrg_.step;
	if ( !colrg_.includes( curpos_.col, false ) )
	{
	    curpos_.row += rowrg_.step;
	    if ( !rowrg_.includes( curpos_.row, false ) )
		return -1;

	    curpos_.col = colrg_.start;
	}
    }

    return curpos_.getSerialized();
}

protected:

    RowCol              curpos_;
    StepInterval<int>   rowrg_;
    StepInterval<int>   colrg_;
};


Iterator* RowColSurface::createIterator() const
{ return new RowColSurfaceIterator( *this ); }

void RowColSurface::getPosIDs( TypeSet<GeomPosID>& pids, bool remudf ) const
{
    pids.erase();
    const StepInterval<int> rowrg = rowRange();
    if ( rowrg.start>rowrg.stop )
	return;

    const int nrrows = rowrg.nrSteps()+1;

    RowCol rc;
    for ( int rowidx=0; rowidx<nrrows; rowidx++ )
    {
	rc.row = rowrg.atIndex( rowidx );
	const StepInterval<int> colrg = colRange( rc.row );
	if ( colrg.start>colrg.stop )
	    continue;

	const int nrcols = colrg.nrSteps()+1;

	for ( int colidx=0; colidx<nrcols; colidx++ )
	{
	    rc.col = colrg.atIndex( colidx );

	    if ( remudf && !isKnotDefined(rc) ) continue;

	    pids += rc.toInt64();
	}
    }
}


StepInterval<int> RowColSurface::colRange() const
{
    StepInterval<int> res( INT_MAX, INT_MIN, 1 );

    const StepInterval<int> rowrg = rowRange();
    if ( rowrg.start>rowrg.stop )
	return res;

    const int nrrows = rowrg.nrSteps()+1;

    for ( int rowidx=0; rowidx<nrrows; rowidx++ )
    {
	const int row = rowrg.atIndex( rowidx );
	const StepInterval<int> colrg = colRange( row );
	if ( colrg.start>colrg.stop )
	    continue;

	res.include( colrg.start, false );
	res.include( colrg.stop, false );
	res.step = mMAX( res.step, colrg.step );
    }

    return res;
}


Coord3 RowColSurface::getPosition( GeomPosID pid ) const
{ return getKnot( RowCol::fromInt64(pid) ); }


bool RowColSurface::setPosition( GeomPosID pid, const Coord3& pos )
{ return setKnot( RowCol::fromInt64(pid), pos ); }


bool RowColSurface::isDefined( GeomPosID pid ) const
{ return isKnotDefined( RowCol::fromInt64(pid) ); }

}; //namespace

