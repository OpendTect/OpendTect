/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
-*/

static const char* rcsID = "$Id: cubicbeziersurface.cc,v 1.3 2005-01-20 07:08:11 kristofer Exp $";

#include "cubicbeziersurface.h"

#include "parametricsurfaceimpl.h"

#include "arrayndimpl.h"
#include "errh.h"
#include "rowcol.h"
#include "survinfo.h"

namespace Geometry
{


CubicBezierSurface::CubicBezierSurface(const Coord3& p0, const Coord3& p1,
				    const RCol& origo_, const RCol& step_ )
    : ParametricSurface( origo_, step_ )
    , positions( new Array2DImpl<Coord3>(1,2) )
    , rowdirections( 0 )
    , coldirections( 0 )
{
     if ( !p0.isDefined() || !p1.isDefined() )
	 pErrMsg("Cannot start surface with undefined positions" );
     else
     {
	 positions->set( 0, 0, p0 );
	 positions->set( 0, 1, p1 );
     }
}


CubicBezierSurface::CubicBezierSurface( const CubicBezierSurface& b )
    : ParametricSurface( b.origo, b.step )
    , positions( new Array2DImpl<Coord3>(*b.positions) )
    , rowdirections( b.rowdirections
	    ? new Array2DImpl<Coord3>(*b.rowdirections) : 0 )
    , coldirections( b.coldirections
	    ? new Array2DImpl<Coord3>(*b.coldirections) : 0 )
{ }


CubicBezierSurface::~CubicBezierSurface()
{
    delete rowdirections;
    delete coldirections;
    delete positions;
}


CubicBezierSurface* CubicBezierSurface::clone() const
{ return new CubicBezierSurface( *this ); }


IntervalND<float> CubicBezierSurface::boundingBox(bool approx) const
{
    //Todo: Own impl
    return ParametricSurface::boundingBox(approx);
}


Coord3 CubicBezierSurface::computePosition( const Coord& ) const
{
    pErrMsg( "Not impl" );
    return Coord3::udf();
}


Coord3 CubicBezierSurface::computeNormal( const Coord& ) const
{
    pErrMsg( "Not impl" );
    return Coord3::udf();
}


bool CubicBezierSurface::insertRow(int row)
{
    mInsertStart( rowidx, Row, rowIndex(row) );
    mCloneRowVariable( Coord3, positions, computePosition(param), Coord3::udf())
    mCloneRowVariable( Coord3, rowdirections, Coord3::udf(), Coord3::udf() )
    mCloneRowVariable( Coord3, coldirections, Coord3::udf(), Coord3::udf() )
    return true;
}


bool CubicBezierSurface::insertColumn(int col)
{
    mInsertStart( colidx, Col, colIndex(col) );
    mCloneColVariable( Coord3, positions, computePosition(param), Coord3::udf())
    mCloneColVariable( Coord3, rowdirections, Coord3::udf(), Coord3::udf() )
    mCloneColVariable( Coord3, coldirections, Coord3::udf(), Coord3::udf() )
    return true;
}


bool CubicBezierSurface::removeRow( int row )
{
    pErrMsg( "not implemented ");
    return true;
}


bool CubicBezierSurface::removeColumn( int col )
{
    pErrMsg( "not implemented ");
    return true;
}


Coord3 CubicBezierSurface::getKnot( const RCol& rc ) const
{
    const int index = getIndex(rc);
    if ( index==-1 ) return Coord3::udf();
	
    return positions->getData()[index];
}

#define mGetDirectionImpl( dirptr, compfunc ) \
    if ( dirptr ) \
    { \
	const int index = getIndex(rc); \
	if ( index!=-1 ) \
	{ \
	    const Coord3& dir = dirptr->getData()[index]; \
	    if ( dir.isDefined() ) return dir; \
	} \
    } \
 \
    return computeifudf ? compfunc(rc) : Coord3::udf()


Coord3 CubicBezierSurface::getRowDirection( const RCol& rc,
					    bool computeifudf ) const
{
    mGetDirectionImpl( rowdirections, computeRowDirection);
}



Coord3 CubicBezierSurface::getColDirection( const RCol& rc,
					    bool computeifudf ) const
{
    mGetDirectionImpl( coldirections, computeColDirection);
}


#ifndef mEPS
#define mEPS 1e-10
#endif


#define mComputeDirImpl(_rowcol_) \
    const Coord3* ptr = positions->getData(); \
    int diff = 2*step._rowcol_; \
    int previndex = getIndex(prev); \
    if ( previndex!=-1 && !ptr[previndex].isDefined() ) previndex==-1; \
 \
    int nextindex = getIndex(next); \
    if ( nextindex!=-1 && !ptr[nextindex].isDefined() ) nextindex==-1; \
 \
    if ( previndex==-1 ) \
    { \
	previndex=getIndex(rc); \
	if ( previndex!=-1 && !ptr[previndex].isDefined() ) previndex==-1; \
	diff = step._rowcol_; \
    } \
    else if ( nextindex==-1) \
    { \
	nextindex=getIndex(rc); \
	if ( nextindex!=-1 && !ptr[nextindex].isDefined() ) nextindex==-1; \
	diff = step._rowcol_; \
    } \
 \
    if ( previndex==-1 || nextindex==-1 ) \
	return Coord3::udf(); \
 \
    const Coord3 prevcoord = ptr[previndex]; \
    const Coord3 nextcoord = ptr[nextindex]; \
    if ( prevcoord.sqDistance(nextcoord)<mEPS ) \
	return Coord3::udf(); \
 \
    return (nextcoord-prevcoord)*step._rowcol_/diff


Coord3 CubicBezierSurface::computeColDirection( const RCol& rc ) const
{
    const int lastcol = origo.col + (nrCols()-1)*step.col;
    const RowCol prev( rc.r(), rc.c()==origo.col && circularCols() 
	    ? lastcol : rc.c()-step.col );
    const RowCol next( rc.r(), rc.c()==lastcol && circularCols() 
	    ? origo.col : rc.c()+step.col);

    mComputeDirImpl(col);
}


Coord3 CubicBezierSurface::computeRowDirection( const RCol& rc ) const
{
    const int lastrow = origo.row + (nrRows()-1)*step.row;
    const RowCol prev( rc.r()==origo.row && circularRows() 
	    ? lastrow : rc.r()-step.row, rc.c() );
    const RowCol next( rc.r()==lastrow && circularRows() 
	    ? origo.row : rc.r()+step.row, rc.c() );

    mComputeDirImpl(row);
}


void CubicBezierSurface::_setKnot( int idx, const Coord3& np )
{ positions->getData()[idx] = np; }


int CubicBezierSurface::nrRows() const
{ return positions ? positions->info().getSize(rowDim()) : 0; }


int CubicBezierSurface::nrCols() const
{ return positions ? positions->info().getSize(colDim()) : 0; }

};

