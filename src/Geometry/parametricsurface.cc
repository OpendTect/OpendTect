/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
-*/

static const char* rcsID = "$Id$";

#include "parametricsurface.h"

#include "arrayndimpl.h"
#include "errh.h"
#include "rowcol.h"
#include "survinfo.h"

namespace Geometry
{


ParametricSurface::ParametricSurface( const RowCol& origin, const RowCol& step )
    : origin_( origin )
    , step_( step )
    , checksupport_( true )
    , checkselfintersection_( true )
{ }


ParametricSurface::~ParametricSurface()
{ }



Coord3 ParametricSurface::computePosition( const Coord& param ) const
{
    pErrMsg( "Not impl" );
    return Coord3::udf();
}


Coord3 ParametricSurface::computeNormal( const Coord& ) const
{
    pErrMsg( "Not impl" );
    return Coord3::udf();
}


StepInterval<int> ParametricSurface::rowRange() const
{
    return StepInterval<int>( origin_.row, origin_.row+(nrRows()-1)*step_.row,
	   		      step_.row );
}


StepInterval<int> ParametricSurface::colRange() const
{
    return StepInterval<int>( origin_.col, origin_.col+(nrCols()-1)*step_.col,
	    		      step_.col );
}


StepInterval<int> ParametricSurface::colRange(int) const
{ return colRange(); }


int ParametricSurface::nrKnots() const { return nrCols()*nrRows(); }


RowCol ParametricSurface::getKnotRowCol( int idx ) const
{
    return RowCol( origin_.row+idx/nrCols()*step_.row,
	    	   origin_.col+idx%nrCols()*step_.col);
}


int ParametricSurface::getKnotIndex( const RowCol& rc ) const
{
    RowCol relbid = -(origin_-rc);
    if ( relbid.row<0 || relbid.col<0 )
	return -1;

    if ( relbid.row%step_.row || relbid.col%step_.col )
	return -1;

    relbid /= step_;
    
    if ( relbid.row>=nrRows() || relbid.col>=nrCols() )
	return -1;

    return relbid.row*nrCols()+relbid.col;
}



ParametricCurve* ParametricSurface::createRowCurve( float col,
				 const Interval<int>* rw ) const
{
    pErrMsg("Not impl");
    return 0;
}


ParametricCurve* ParametricSurface::createColCurve( float col,
				 const Interval<int>* rw ) const
{
    pErrMsg("Not impl");
    return 0;
}


bool ParametricSurface::setKnot( const RowCol& rc, const Coord3& np )
{
    const Coord3 oldpos = getKnot(rc);
    bool wasundef = !oldpos.isDefined();

    int index;
    if ( nrKnots() )
    {
	if ( wasundef && checksupport_ && !hasSupport(rc) )
	{
	    errmsg() = "New rc does not have any support";
	    return false;
	}

	int rowindex = rowIndex( rc.row );
	if ( rowindex < -1 )
	{
	    const int nrtoinsert = rc.row - origin_.row;
	    bool res = insertRow( origin_.row-step_.row, nrtoinsert );
	    rowindex = rowIndex( rc.row );
	}

	while ( rowindex<0 )
	{
	    if ( !insertRow(origin_.row-step_.row) )
		return false;
	    rowindex = rowIndex( rc.row );
	}

	while ( rowindex>=nrRows() )
	{
	    if ( !insertRow(origin_.row+step_.row*nrRows()) )
		return false;
	    rowindex = rowIndex( rc.row );
	}
 
	int colindex = colIndex( rc.col );
	while ( colindex<0 )
	{
	    if ( !insertCol(origin_.col-step_.col) )
		return false;
	    colindex = colIndex( rc.col );
	}

	while ( colindex>=nrCols() )
	{
	    if ( !insertCol(origin_.col+step_.col*nrCols()) )
		return false;
	    colindex = colIndex( rc.col );
	}

    	index = getKnotIndex( rc );
    }
    else
    {
	origin_ = rc;
	index = 0;
    }

    if ( !np.isDefined() || index<0 )
	return unsetKnot( rc );

    _setKnot( index, np );

    if ( checkSelfIntersection(rc) )
    {
	_setKnot(index,oldpos);
	return false;
    }

    const GeomPosID gpos = rc.toInt64();
    if ( wasundef ) triggerNrPosCh(gpos);
    else triggerMovement(gpos);

    return true;
}


bool ParametricSurface::unsetKnot( const RowCol& rc )
{
    if ( !getKnot(rc).isDefined() )
	return true;

    const int index = getKnotIndex(rc);
    if ( index==-1 )
    {
	errmsg() = "Cannot unset non-existing knot";
	return false;
    }

    // TODO: prevent endless loop in isAtSameEdge. Then remove these two lines
    _setKnot( index, Coord3::udf() );
    triggerNrPosCh( rc.toInt64() );
    return true;

    const TypeSet<RowCol>& dirs = RowCol::clockWiseSequence();

    TypeSet<RowCol> edgepair0, edgepair1;
    if ( isAtEdge( rc ) )
    {
	TypeSet<RowCol> neighborsonedge;
	for ( int idx=0; idx<dirs.size(); idx++ )
	{
	    const RowCol neighbor = dirs[idx]*step_+rc;
	    if ( !isKnotDefined(neighbor) )
		continue;

	    if ( isAtEdge(neighbor) )
		neighborsonedge += neighbor;
	}

	for ( int idx=0; idx<neighborsonedge.size()-1; idx++ )
	{
	    for ( int idy=idx+1; idy<neighborsonedge.size(); idy++ )
	    {
		if ( isAtSameEdge(neighborsonedge[idx],neighborsonedge[idy]) )
		{
		    edgepair0 += neighborsonedge[idx];
		    edgepair1 += neighborsonedge[idy];
		}
	    }
	}
    }

    const Coord3 oldpos = getKnot(rc);
    _setKnot(index, Coord3::udf() );

    for ( int idx=0; idx<dirs.size(); idx++ )
    {
	const RowCol neighbor = dirs[idx]*step_+rc;
	if ( !isKnotDefined(neighbor) )
	    continue;

	if ( checksupport_ && !hasSupport(neighbor) )
	{
	    _setKnot(index, oldpos );
	    errmsg() = "Cannot remove position since neighbor would not be"
		       " supported";
	    return false;
	}
    }

    for ( int idx=0; idx<edgepair0.size(); idx++ )
    {
	if ( !isAtSameEdge(edgepair0[idx],edgepair0[idx]) )
	{
	    _setKnot(index, oldpos );
	    errmsg() = "Cannot remove position since that would divide the"
		       " surface in two parts";
	    return false;
	}
    }

    triggerNrPosCh(rc.toInt64());
    return true;
}


Coord3 ParametricSurface::getKnot( const RowCol& rc ) const
{ return getKnot( rc, false ); }


bool ParametricSurface::isKnotDefined( const RowCol& rc ) const
{
    const int index = getKnotIndex(rc);
    if ( index==-1 ) return false;

    return getKnot(rc).isDefined();
}


Coord3 ParametricSurface::getPosition( od_int64 pid ) const
{ return getKnot( RowCol(pid) ); }


bool ParametricSurface::setPosition( od_int64 pid, const Coord3& pos )
{ return setKnot( RowCol(pid), pos ); }


bool ParametricSurface::isDefined( od_int64 pid ) const
{ return isKnotDefined( RowCol(pid) ); }


bool ParametricSurface::checkSupport(bool yn)
{
    const bool oldstatus = checksupport_;
    checksupport_ = yn;
    return oldstatus;
}


bool ParametricSurface::checksSupport() const { return checksupport_; }


bool ParametricSurface::checkSelfIntersection( bool yn )
{
    const bool oldstatus = checkselfintersection_;
    checkselfintersection_ = yn;
    return oldstatus;
}


bool ParametricSurface::checksSelfIntersection() const
{ return checkselfintersection_; }


bool ParametricSurface::checkSelfIntersection(const RowCol& ) const
{ return false; }


bool ParametricSurface::hasSupport( const RowCol& rc ) const
{
    const TypeSet<RowCol>& dirs = RowCol::clockWiseSequence();
    const int nrdirs = dirs.size();

    bool prevdirdefined = false;
    for ( int idx=0; idx<=nrdirs; idx++ )
    {
	const RowCol neighbor = dirs[idx%nrdirs]*step_+rc;
	if ( isKnotDefined(neighbor) )
	{
	    if ( prevdirdefined )
		return true;

	    prevdirdefined = true;
	}
	else
	    prevdirdefined = false;
    }

    return false;
}


bool ParametricSurface::isAtEdge( const RowCol& rc ) const
{
    return !isKnotDefined(RowCol(rc.row+step_.row, rc.col+step_.col) ) ||
	   !isKnotDefined(RowCol(rc.row+step_.row, rc.col-step_.col) ) ||
	   !isKnotDefined(RowCol(rc.row-step_.row, rc.col+step_.col) ) ||
	   !isKnotDefined(RowCol(rc.row-step_.row, rc.col-step_.col) );
}


bool ParametricSurface::isAtSameEdge( const RowCol& rc1, const RowCol& rc2, 
       TypeSet<RowCol>* path ) const
{
    if ( !isAtEdge(rc1)  || !isAtEdge(rc2) )
	return false;

    if ( rc1.isNeighborTo(rc2, step_, true ) )
	return true;

    const TypeSet<RowCol>& dirs = RowCol::clockWiseSequence();

    int udefidx = -1;
    if ( !isKnotDefined(RowCol(rc1.row+step_.row, rc1.col+step_.col)) )
	udefidx = dirs.indexOf(RowCol(1,1));
    else if ( !isKnotDefined(RowCol(rc1.row+step_.row, rc1.col-step_.col)) )
	udefidx = dirs.indexOf(RowCol(1,-1));
    else if ( !isKnotDefined(RowCol(rc1.row-step_.row, rc1.col+step_.col)) )
	udefidx = dirs.indexOf(RowCol(-1,1));
    else if ( !isKnotDefined(RowCol(rc1.row-step_.row, rc1.col-step_.col)) )
	udefidx = dirs.indexOf(RowCol(-1,-1));

    if ( udefidx==-1 )
    {
	pErrMsg( "hugh?" );
	return false;
    }

    for ( int idx=0; idx<dirs.size(); idx++ )
    {
	const RowCol neighbor = step_*dirs[(idx+udefidx)%dirs.size()]+rc1;
	if ( path && path->indexOf(neighbor)!=-1 )
	    continue;

	if ( isKnotDefined(neighbor) )
	{
	    TypeSet<RowCol> localpath;
	    if ( !path ) path = &localpath;
	    (*path) += rc1;
	    return isAtSameEdge( neighbor, rc2 );
	}
    }

    return false;
}

#define mRemovePart( rc0, rc0start, incop, rc1, removefunc ) \
    for ( int rc0=rc0##rng.rc0start; rc0##rng.includes(rc0,false); \
	  rc0 incop rc0##rng.step ) \
    { \
	bool founddefknot = false; \
	for ( int rc1=rc1##rng.start; rc1<=rc1##rng.stop; \
	      rc1+=rc1##rng.step ) \
	{ \
	    if ( isKnotDefined( RowCol(row,col) ) ) \
	    { founddefknot = true; break; } \
	} \
 \
	if ( founddefknot ) \
	{ \
	    if ( rc0!=rc0##rng.rc0start ) \
	    { \
		Interval<int> rg( rc0##rng.rc0start, prev ); \
		rg.sort(); \
		removefunc( rg.start, rg.stop ); \
	    } \
	    break; \
	} \
\
	prev = rc0; \
    } \
    rc0##rng = rc0##Range()

void ParametricSurface::trimUndefParts()
{
    StepInterval<int> rowrng = rowRange();
    StepInterval<int> colrng = colRange();

    int prev;
    mRemovePart( row, start, +=, col, removeRow );
    mRemovePart( row, stop,  -=, col, removeRow );
    mRemovePart( col, start, +=, row, removeCol );
    mRemovePart( col, stop,  -=, row, removeCol );
}



}; 

