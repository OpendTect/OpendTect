/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
-*/

static const char* rcsID = "$Id: parametricsurface.cc,v 1.7 2005-03-10 11:45:04 cvskris Exp $";

#include "parametricsurface.h"

#include "arrayndimpl.h"
#include "errh.h"
#include "rowcol.h"
#include "survinfo.h"

namespace Geometry
{


ParametricSurface::ParametricSurface( const RCol& origo_, const RCol& step_ )
    : origo( origo_ )
    , step( step_ )
    , checksupport( true )
{ }


ParametricSurface::~ParametricSurface()
{ }



Coord3 ParametricSurface::computePosition( const Coord& param ) const
{
    RowCol rc00( (int)param.x, (int)param.y );
    /*
           TODO: Get p00-p11;
	   const float one_minus_u = 1-u;
	   return (one_minus_u*p00 + u*p10) * (1-v) +
	  (one_minus_u*p01 + u*p11) * v;
    */

    pErrMsg( "Not impl" );
    return Coord3::udf();
}


Coord3 ParametricSurface::computeNormal( const Coord& ) const
{
    pErrMsg( "Not impl" );
    return Coord3::udf();
}




void ParametricSurface::getPosIDs( TypeSet<GeomPosID>& pids ) const
{
    pids.erase();

    RowCol rc;
    for ( int rowidx=0; rowidx<nrRows(); rowidx++ )
    {
	rc.row = origo.row+rowidx*step.row;
	for ( int colidx=0; colidx<nrCols(); colidx++ )
	{
	    rc.col = origo.col+colidx*step.col;

	    if ( isKnotDefined( rc ) )
		pids += rc.getSerialized();
	}
    }
}


StepInterval<int> ParametricSurface::rowRange() const
{
    return StepInterval<int>( origo.row, origo.row+(nrRows()-1)*step.row,
	   		      step.row );
}


StepInterval<int> ParametricSurface::colRange() const
{
    return StepInterval<int>( origo.col, origo.col+(nrCols()-1)*step.col,
	    		      step.col );
}


int ParametricSurface::nrKnots() const { return nrCols()*nrRows(); }


RowCol ParametricSurface::getKnotRowCol( int idx ) const
{
    return RowCol( origo.row+idx/nrRows()*step.row,
	    	   origo.col+idx%nrRows()*step.col);
}


int ParametricSurface::getKnotIndex( const RCol& rc ) const
{
    RowCol relbid = -(origo-rc);
    if ( relbid.row<0 || relbid.col<0 )
	return -1;

    if ( relbid.row%step.row || relbid.col%step.col )
	return -1;

    relbid /= step;
    
    if ( relbid.row>=nrRows() || relbid.col>=nrCols() )
	return -1;

    return relbid.row*nrCols()+relbid.col;
}



bool ParametricSurface::setKnot( const RCol& rc, const Coord3& np )
{
    if ( !np.isDefined() ) 
	return unsetKnot( rc );

    const Coord3 oldpos = getKnot(rc);
    bool wasundef = oldpos.isDefined();

    int index;
    if ( nrKnots() )
    {
	if ( wasundef && !hasSupport(rc) )
	{
	    pErrMsg("New rc does not have any support");
	    return false;
	}

	const int rowindex = rowIndex( rc.r() );
	if ( rowindex==-1 || rowindex==nrRows() )
	{
	    if ( !insertRow(rc.r()) )
		return false;

	    wasundef = true;
	}

	const int colindex = colIndex( rc.c() );
	if  ( colindex==-1 || colindex==nrCols() )
	{
	    if ( !insertCol(rc.c()) )
		return false;
	    
	    wasundef = true;
	}


    	index = getKnotIndex(rc);
    }
    else
    {
	origo = rc;
	index = 0;
    }

    _setKnot(index, np);

    if ( checkSelfIntersection(rc) )
    {
	_setKnot(index,oldpos);
	return false;
    }

    const GeomPosID gpos = rc.getSerialized();
    if ( wasundef ) triggerNrPosCh(gpos);
    else triggerMovement(gpos);

    return true;
}


bool ParametricSurface::unsetKnot( const RCol& rc )
{
    if ( !getKnot(rc).isDefined() )
	return true;

    const int index = getKnotIndex(rc);
    if ( index==-1 )
    {
	errmsg() = "Cannot unset non-existing knot";
	return false;
    }

    const TypeSet<RowCol>& dirs = RowCol::clockWiseSequence();

    TypeSet<RowCol> edgepair0, edgepair1;
    if ( isAtEdge( rc ) )
    {
	TypeSet<RowCol> neighborsonedge;
	for ( int idx=0; idx<dirs.size(); idx++ )
	{
	    const RowCol neighbor = dirs[idx]*step+rc;
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
	const RowCol neighbor = dirs[idx]*step+rc;
	if ( !isKnotDefined(neighbor) )
	    continue;

	if ( !hasSupport(neighbor) )
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

    triggerNrPosCh(rc.getSerialized());
    return true;
}


bool ParametricSurface::isKnotDefined( const RCol& rc ) const
{
    const int index = getKnotIndex(rc);
    if ( index==-1 ) return false;

    return getKnot(rc).isDefined();
}


Coord3 ParametricSurface::getPosition( int64 pid ) const
{ return getKnot( RowCol(pid) ); }


bool ParametricSurface::setPosition( int64 pid, const Coord3& pos )
{ return setKnot( RowCol(pid), pos ); }


bool ParametricSurface::isDefined( int64 pid ) const
{ return isKnotDefined( RowCol(pid) ); }


bool ParametricSurface::checkSupport(bool yn)
{
    const bool oldstatus = checksupport;
    checksupport = yn;
    return oldstatus;
}


bool ParametricSurface::checksSupport() const { return checksupport; }


bool ParametricSurface::checkSelfIntersection(const RCol& ) const
{ return false; }


bool ParametricSurface::hasSupport( const RCol& rc ) const
{
    if ( !checksupport ) return true;

    const TypeSet<RowCol>& dirs = RowCol::clockWiseSequence();
    const int nrdirs = dirs.size();

    bool prevdirdefined = false;
    for ( int idx=0; idx<=nrdirs; idx++ )
    {
	const RowCol neighbor = dirs[idx%nrdirs]*step+rc;
	if ( isKnotDefined(neighbor) )
	{
	    if ( prevdirdefined )
		return true;

	    prevdirdefined = true;
	}
    }

    return false;
}


bool ParametricSurface::isAtEdge( const RCol& rc ) const
{
    return !isKnotDefined(RowCol(rc.r()+step.row, rc.c()+step.col) ) ||
	   !isKnotDefined(RowCol(rc.r()+step.row, rc.c()-step.col) ) ||
	   !isKnotDefined(RowCol(rc.r()-step.row, rc.c()+step.col) ) ||
	   !isKnotDefined(RowCol(rc.r()-step.row, rc.c()-step.col) );
}


bool ParametricSurface::isAtSameEdge( const RCol& rc1, const RCol& rc2, 
       TypeSet<RowCol>* path ) const
{
    if ( !isAtEdge(rc1)  || !isAtEdge(rc2) )
	return false;

    if ( rc1.isNeighborTo(rc2, step, true ) )
	return true;

    const TypeSet<RowCol>& dirs = RowCol::clockWiseSequence();

    int udefidx = -1;
    if ( !isKnotDefined(RowCol(rc1.r()+step.row, rc1.c()+step.col)) )
	udefidx = dirs.indexOf(RowCol(1,1));
    else if ( !isKnotDefined(RowCol(rc1.r()+step.row, rc1.c()-step.col)) )
	udefidx = dirs.indexOf(RowCol(1,-1));
    else if ( !isKnotDefined(RowCol(rc1.r()-step.row, rc1.c()+step.col)) )
	udefidx = dirs.indexOf(RowCol(-1,1));
    else if ( !isKnotDefined(RowCol(rc1.r()-step.row, rc1.c()-step.col)) )
	udefidx = dirs.indexOf(RowCol(-1,-1));

    if ( udefidx==-1 )
    {
	pErrMsg( "hugh?" );
	return false;
    }

    for ( int idx=0; idx<dirs.size(); idx++ )
    {
	const RowCol neighbor = step*dirs[(idx+udefidx)%dirs.size()]+rc1;
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


};

