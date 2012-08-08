/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
-*/

static const char* rcsID mUnusedVar = "$Id: cubicbeziersurface.cc,v 1.35 2012-08-08 05:26:28 cvssalil Exp $";

#include "cubicbeziersurface.h"

#include "parametricsurfaceimpl.h"

#include "arrayndimpl.h"
#include "cubicbeziercurve.h"
#include "errh.h"
#include "mathfunc.h"
#include "rowcol.h"
#include "survinfo.h"
#include "trigonometry.h"


namespace Geometry
{

#define idx00	0
#define idx01	1
#define idx02	2
#define idx03	3
#define idx10	4
#define idx11	5
#define idx12	6
#define idx13	7
#define idx20	8
#define idx21	9
#define idx22	10
#define idx23	11
#define idx30	12
#define idx31	13
#define idx32	14
#define idx33	15

CubicBezierSurfacePatch::CubicBezierSurfacePatch(
    const Coord3& t00, const Coord3& t01, const Coord3& t02, const Coord3& t03,
    const Coord3& t10, const Coord3& t11, const Coord3& t12, const Coord3& t13,
    const Coord3& t20, const Coord3& t21, const Coord3& t22, const Coord3& t23,
    const Coord3& t30, const Coord3& t31, const Coord3& t32, const Coord3& t33)
{
    pos[idx00] = t00; pos[idx01] = t01; pos[idx02] = t02; pos[idx03] = t03;
    pos[idx10] = t10; pos[idx11] = t11; pos[idx12] = t12; pos[idx13] = t13;
    pos[idx20] = t20; pos[idx21] = t21; pos[idx22] = t22; pos[idx23] = t23;
    pos[idx30] = t30; pos[idx31] = t31; pos[idx32] = t32; pos[idx33] = t33;
}


CubicBezierSurfacePatch* CubicBezierSurfacePatch::clone() const
{
    return new CubicBezierSurfacePatch(
			        pos[idx00], pos[idx01], pos[idx02], pos[idx03],
				pos[idx10], pos[idx11], pos[idx12], pos[idx13],
				pos[idx20], pos[idx21], pos[idx22], pos[idx23],
				pos[idx30], pos[idx31], pos[idx32], pos[idx33]);
}


IntervalND<float> CubicBezierSurfacePatch::computeBoundingBox() const
{
    IntervalND<float> bbox(3);
    bbox.setRange(*pos);
    for ( int idx=1; idx<16; idx++ )
	bbox.include( pos[idx] );

    return bbox;
}


Coord3 CubicBezierSurfacePatch::computePos( float u, float v ) const
{
    if ( mIsZero(u,1e-3) )
	return cubicDeCasteljau(pos,idx00,1,v);
    else if ( mIsEqual(u,1,1e-3) )
	return cubicDeCasteljau(pos, idx30, 1 ,v);
   
    Coord3 temppos [] = { cubicDeCasteljau(pos,idx00,1,v),
			  cubicDeCasteljau(pos,idx10,1,v),
			  cubicDeCasteljau(pos,idx20,1,v),
			  cubicDeCasteljau(pos,idx30,1,v) };

    return cubicDeCasteljau( temppos, 0, 1, u );
}


Coord3 CubicBezierSurfacePatch::computeUTangent( float u, float v ) const
{
    Coord3 temppos [] = { cubicDeCasteljau(pos,idx00,1,v),
			  cubicDeCasteljau(pos,idx10,1,v),
			  cubicDeCasteljau(pos,idx20,1,v),
			  cubicDeCasteljau(pos,idx30,1,v) };

    return cubicDeCasteljauTangent( temppos, 0, 1, u );
}


Coord3 CubicBezierSurfacePatch::computeVTangent( float u, float v ) const
{
    Coord3 temppos [] = { cubicDeCasteljau(pos,idx00,4,u),
			  cubicDeCasteljau(pos,idx01,4,u),
			  cubicDeCasteljau(pos,idx02,4,u),
			  cubicDeCasteljau(pos,idx03,4,u) };

    return cubicDeCasteljauTangent( temppos, 0, 1, v );
}


Coord3 CubicBezierSurfacePatch::computeNormal( float u, float v ) const
{
    return computeUTangent(u,v).cross(computeVTangent(u,v));
}


bool CubicBezierSurfacePatch::intersectWithLine( const Line3& line,
				float& u, float& v, float eps ) const
{
    const Coord3 linepoint(line.x0_, line.y0_, line.z0_ );
    const Coord3 linedir( line.alpha_, line.beta_, line.gamma_ );
    for ( int idx=0; idx<20; idx++ )
    {
	const Coord3 currentpos = computePos(u,v);
	const float sqdist = (float) (currentpos-linepoint).cross(linedir).sqAbs();
	if ( sqdist<eps ) return true;

	const Coord3 upos = computePos(u+1e-3f,v);
	const float udist = (float) (upos-linepoint).cross(linedir).sqAbs()-sqdist;

	const Coord3 vpos = computePos(u,v+1e-3f);
	const float vdist = (float) (vpos-linepoint).cross(linedir).sqAbs()-sqdist;

 	if ( fabs(udist)>fabs(vdist) )	
	    u = u-(sqdist/udist*1e-3f);
	else
	    v = v-(sqdist/vdist*1e-3f);

	if ( u<0 || u>1 ) return false;
	if ( v<0 || v>1 ) return false;
    }

    pErrMsg( "Maximum nr of iterations reached" );

    return false;
}


CubicBezierSurface::CubicBezierSurface( const RowCol& newstep )
    : ParametricSurface( RowCol(0,0) , newstep )
    , positions( 0 )
    , rowdirections( 0 )
    , coldirections( 0 )
    , directioninfluence( 1.0/3 )
{ }


CubicBezierSurface::CubicBezierSurface( const CubicBezierSurface& b )
    : ParametricSurface( b.origin_, b.step_ )
    , positions( b.positions ? new Array2DImpl<Coord3>(*b.positions) : 0 )
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
    if ( approx ) return ParametricSurface::boundingBox(approx);

    IntervalND<float> bbox(3);
    const StepInterval<int> rowrange = rowRange();
    const StepInterval<int> colrange = colRange();

    for ( RowCol rc(rowrange.start,0); rc.row<=rowrange.stop;
	  rc.row+=rowrange.step )
    {
	for ( rc.col = colrange.start; rc.col<=colrange.stop;
	      rc.col+=colrange.step )
	{
	    for ( RowCol relrc(-1,-1); relrc.row<=1; relrc.row++ )
	    {
		for ( relrc.col=-1; relrc.col<=1; relrc.col++ )
		{
		    const Coord3 vertex = getBezierVertex( rc, relrc );
		    if ( !vertex.isDefined() )
			continue;

		    if ( !bbox.isSet() ) bbox.setRange(vertex);
		    else bbox.include(vertex);
		}
	    }
	}
    }

    return bbox;
}


Coord3 CubicBezierSurface::computePosition( const Coord& params ) const
{
    const StepInterval<int> rowrange = rowRange();
    const StepInterval<int> colrange = colRange();

    int prevrowidx = rowrange.getIndex(params.x);
    if ( prevrowidx<0 || prevrowidx>nrRows()-1 )
	return Coord3::udf();
    else if ( prevrowidx==nrRows()-1 )
    {
	if ( rowrange.atIndex(prevrowidx)<=params.x )
	    prevrowidx--;
	else
	    return Coord3::udf();
    }

    int prevcolidx = colrange.getIndex(params.y);
    if ( prevcolidx<0 || prevcolidx>nrCols()-1 )
	return Coord3::udf();
    else if ( prevcolidx==nrCols()-1 )
    {
	if ( colrange.atIndex(prevcolidx)<=params.y )
	    prevcolidx--;
	else
	    return Coord3::udf();
    }

    const int prevrow = rowrange.atIndex(prevrowidx);
    const int prevcol = colrange.atIndex(prevcolidx);

    const CubicBezierSurfacePatch* patch = getPatch(RowCol(prevrow,prevcol));

    if ( !patch ) return Coord3::udf();
    return patch->computePos( (float) ((params.x - prevrow)/rowrange.step),
	    		     (float) ((params.y - prevcol)/colrange.step));
}


Coord3 CubicBezierSurface::computeNormal( const Coord& ) const
{
    pErrMsg( "Not impl" );
    return Coord3::udf();
}


bool CubicBezierSurface::intersectWithLine(const Line3& line, Coord& res) const
{
    IntervalND<float> bbox = boundingBox(false);
    if ( !bbox.isSet() ) return false;

    Coord3 center( bbox.getRange(0).center(), bbox.getRange(1).center(),
			 bbox.getRange(2).center() );
    Coord3 closestpointonline = line.getPoint( line.closestPoint(center) );
    if ( !bbox.includes(closestpointonline,false) )
	return false;

    RowCol rc(origin_);
    for ( int rowidx=0; rowidx<nrRows()-1; rowidx++, rc.row+=step_.row )
    {
	rc.col=origin_.col;
	for ( int colidx=0; colidx<nrCols()-1; colidx++, rc.col+=step_.col )
	{
	    const CubicBezierSurfacePatch* patch = getPatch(rc);
	    if ( !patch ) continue;

	    bbox = patch->computeBoundingBox();
	    if ( !bbox.isSet() ) continue;

	    center.x = bbox.getRange(0).center();
	    center.y = bbox.getRange(1).center();
	    center.z = bbox.getRange(2).center();

	    closestpointonline = line.getPoint( line.closestPoint(center) );
	    if ( !bbox.includes(closestpointonline,false) )
		continue;

	    PtrMan<CubicBezierSurfacePatch> dummypatch = 0;
	    const int zfactor = SI().zDomain().userFactor();
	    Line3 intersectionline( line );
	    if ( zfactor!=1 )
	    {
		dummypatch = patch->clone();
		for ( int idx=dummypatch->nrPos()-1; idx>=0; idx-- )
		    dummypatch->pos[idx].z *= zfactor;

		patch = dummypatch;

		intersectionline.z0_ *= zfactor;
		intersectionline.gamma_ *= zfactor;
	    }

	    float u = (float) res.x;
	    float v = (float) res.y;
	    if ( !patch->intersectWithLine( intersectionline, u, v, 1 ) )
		continue;

	    res.x = u; res.y=v; return true;
	}
    }

    return false;
}


Coord3 CubicBezierSurface::getBezierVertex( const RowCol& knot,
					    const RowCol& relpos ) const
{
    Coord3 pos = getKnot( knot );
    if ( !pos.isDefined() ) return pos;

    if ( relpos.row )
    {
	const Coord3 rowdir = getRowDirection( knot, true );
	if ( !rowdir.isDefined() ) return rowdir;

	if ( relpos.row>0 )
	    pos += rowdir;
	else
	    pos -= rowdir;
    }

    if ( relpos.col )
    {
	const Coord3 coldir = getColDirection( knot, true );
	if ( !coldir.isDefined() ) return coldir;

	if ( relpos.col>0 )
	    pos += coldir;
	else 
	    pos -= coldir;
    }

    return pos;
}


bool CubicBezierSurface::insertRow(int row, int nrtoinsert )
{
    mInsertStart( rowidx, row, nrRows() );

    TypeSet<GeomPosID> movedpos;
    if ( !addedinfront )
    {
	for ( int idx=rowidx; idx<curnrrows; idx++ )
	{
	    const int currow = origin_.row+idx*step_.row;
	    for ( int idy=0; idy<curnrcols; idy++ )
		movedpos +=
		    RowCol(currow,origin_.col+idy*step_.col).toInt64();
	}
    }

    TypeSet<GeomPosID> addedpos;
    const int newrow = addedinfront 
		       ? origin_.row : origin_.row+curnrrows*step_.row;
    for ( int idy=0; idy<curnrcols; idy++ )
	addedpos += RowCol(newrow,origin_.col+idy*step_.col).toInt64();

    mCloneRowVariable( Coord3, positions, computePosition(param), Coord3::udf())
    mCloneRowVariable( Coord3, rowdirections, Coord3::udf(), Coord3::udf() )
    mCloneRowVariable( Coord3, coldirections, Coord3::udf(), Coord3::udf() )

    if ( addedpos.size() ) triggerNrPosCh( addedpos );
    if ( movedpos.size() ) triggerMovement( movedpos );
    return true;
}


bool CubicBezierSurface::insertCol(int col, int nrtoinsert )
{
    mInsertStart( colidx, col, nrCols() );
    TypeSet<GeomPosID> movedpos;
    if ( !addedinfront )
    {
	for ( int idx=colidx; idx<curnrcols; idx++ )
	{
	    const int curcol = origin_.col+idx*step_.col;
	    for ( int idy=0; idy<curnrrows; idy++ )
		movedpos +=
		    RowCol(origin_.row+idy*step_.row,curcol).toInt64();
	}
    }

    TypeSet<GeomPosID> addedpos;
    const int newcol = addedinfront 
		       ? origin_.col : origin_.col+curnrcols*step_.col;
    for ( int idy=0; idy<curnrrows; idy++ )
	addedpos += RowCol(origin_.row+idy*step_.row,newcol).toInt64();

    mCloneColVariable( Coord3, positions, computePosition(param), Coord3::udf())
    mCloneColVariable( Coord3, rowdirections, Coord3::udf(), Coord3::udf() )
    mCloneColVariable( Coord3, coldirections, Coord3::udf(), Coord3::udf() )

    if ( addedpos.size() ) triggerNrPosCh( addedpos );
    if ( movedpos.size() ) triggerMovement( movedpos );
    return true;
}


bool CubicBezierSurface::removeRow( int row, int )
{
    const int curnrrows = nrRows();
    const int curnrcols = nrCols();

    const int rowidx = rowIndex( row );
    if ( rowidx<0 || rowidx>=curnrrows )
    { 
	errmsg() = "Row to remove does not exist"; 
	return false; 
    }

    TypeSet<GeomPosID> removedpos;
    TypeSet<GeomPosID> movedpos;

    for ( int idy=0; idy<curnrcols; idy++ )
    {
	int curcol = origin_.col + idy*step_.col;
	removedpos += RowCol( row, curcol ).toInt64();

	for ( int idx=rowidx+1; idx<curnrrows; idx++ )
	{
	    movedpos += 
		RowCol( origin_.row + idx*step_.row, curcol ).toInt64();
	}
    }

    Array2D<Coord3>* newpositions = 
	positions ? new Array2DImpl<Coord3>( curnrrows-1, curnrcols ) : 0;
    
    for ( int idx=0; newpositions && idx<curnrrows-1; idx++ )
    {
	for ( int idy=0; idy<curnrcols; idy++ )
	{
	    const int srcrow = idx<rowidx ? idx : idx+1;
	    newpositions->set( idx, idy, positions->get( srcrow, idy ) );
	}
    }
    if ( newpositions ) 
	{ delete positions; positions = newpositions; }
    
    Array2D<Coord3>* newrowdirections = 
	rowdirections ? new Array2DImpl<Coord3>( curnrrows-1, curnrcols ) : 0;
    
    for ( int idx=0; newrowdirections && idx<curnrrows-1; idx++ )
    {
	for ( int idy=0; idy<curnrcols; idy++ )
	{
	    const int srcrow = idx<rowidx ? idx : idx+1;
	    newrowdirections->set( idx, idy, rowdirections->get(srcrow,idy) );
	}
    }
    if ( newrowdirections ) 
	{ delete rowdirections; rowdirections = newrowdirections; }
    
    Array2D<Coord3>* newcoldirections = 
	coldirections ? new Array2DImpl<Coord3>( curnrrows-1, curnrcols ) : 0;

    for ( int idx=0; newcoldirections && idx<curnrrows-1; idx++ )
    {
	for ( int idy=0; idy<curnrcols; idy++ )
	{
	    const int srcrow = idx<rowidx ? idx : idx+1;
	    newcoldirections->set( idx, idy, coldirections->get(srcrow,idy) );
	}
    }
    if ( newcoldirections )
	{ delete coldirections; coldirections = newcoldirections; }

    if ( removedpos.size() ) triggerNrPosCh( removedpos );
    if ( movedpos.size() ) triggerMovement( movedpos );

    return true;
}


bool CubicBezierSurface::removeCol( int col, int )
{
    const int curnrrows = nrRows();
    const int curnrcols = nrCols();

    const int colidx = colIndex( col );
    if ( colidx<0 || colidx>=curnrcols )
    { 
	errmsg() = "Column to remove does not exist"; 
	return false; 
    }

    TypeSet<GeomPosID> removedpos;
    TypeSet<GeomPosID> movedpos;

    for ( int idx=0; idx<curnrrows; idx++ )
    {
	int currow = origin_.row + idx*step_.row;
	removedpos += RowCol( currow, col ).toInt64();

	for ( int idy=colidx+1; idy<curnrcols; idy++ )
	{
	    movedpos += 
		RowCol( currow, origin_.col + idy*step_.col ).toInt64();
	}
    }

    Array2D<Coord3>* newpositions = 
	positions ? new Array2DImpl<Coord3>( curnrrows, curnrcols-1 ) : 0;

    for ( int idx=0; newpositions && idx<curnrrows; idx++ )
    {
	for ( int idy=0; idy<curnrcols-1; idy++ )
	{
	    const int srccol = idy<colidx ? idy : idy+1;
	    newpositions->set( idx, idy, positions->get( idx, srccol ) );
	}
    }
    if ( newpositions ) 
	{ delete positions; positions = newpositions; }

    Array2D<Coord3>* newrowdirections = 
	rowdirections ? new Array2DImpl<Coord3>( curnrrows, curnrcols-1 ) : 0;

    for ( int idx=0; newrowdirections && idx<curnrrows; idx++ )
    {
	for ( int idy=0; idy<curnrcols-1; idy++ )
	{
	    const int srccol = idy<colidx ? idy : idy+1;
	    newrowdirections->set( idx, idy, rowdirections->get(idx,srccol) );
	}
    }
    if ( newrowdirections ) 
	{ delete rowdirections; rowdirections = newrowdirections; }
    
    Array2D<Coord3>* newcoldirections = 
	coldirections ? new Array2DImpl<Coord3>( curnrrows, curnrcols-1 ) : 0;
   
    for ( int idx=0; newcoldirections && idx<curnrrows; idx++ )
    {
	for ( int idy=0; idy<curnrcols-1; idy++ )
	{
	    const int srccol = idy<colidx ? idy : idy+1;
	    newcoldirections->set( idx, idy, coldirections->get(idx,srccol) );
	}
    }
    if ( newcoldirections )
	{ delete coldirections; coldirections = newcoldirections; }

    if ( removedpos.size() ) triggerNrPosCh( removedpos );
    if ( movedpos.size() ) triggerMovement( movedpos );

    return true;
}


Coord3 CubicBezierSurface::getKnot( const RowCol& rc,
				    bool estimateifundef ) const
{
    const int index = getKnotIndex(rc);
    if ( index==-1 ) return Coord3::udf();

    const Coord3* data = positions->getData();

    if ( estimateifundef && !data[index].isDefined() )
    {
	RowCol neighborrc( rc );
	neighborrc.row = rc.row-step_.row;
	Coord3 pos = getKnot( neighborrc, false );
	if ( pos.isDefined() )
	{
	    const Coord3 dir = getRowDirection( neighborrc, true );
	    if ( dir.isDefined() )
		return pos+dir;
	}

	neighborrc.row = rc.row+step_.row;
	pos = getKnot( neighborrc, false );
	if ( pos.isDefined() )
	{
	    const Coord3 dir = getRowDirection( neighborrc, true );
	    if ( dir.isDefined() )
		return pos-dir;
	}

	neighborrc.row = rc.row;
	neighborrc.col = rc.col-step_.col;
	pos = getKnot( neighborrc, false );
	if ( pos.isDefined() )
	{
	    const Coord3 dir = getColDirection( neighborrc, true );
	    if ( dir.isDefined() )
		return pos+dir;
	}

	neighborrc.col = rc.col+step_.col;
	pos = getKnot( neighborrc, false );
	if ( pos.isDefined() )
	{
	    const Coord3 dir = getColDirection( neighborrc, true );
	    if ( dir.isDefined() )
		return pos-dir;
	}
    }

    return data[index];
}

#define mGetDirectionImpl( dirptr, compfunc ) \
    if ( dirptr ) \
    { \
	const int index = getKnotIndex(rc); \
	if ( index!=-1 ) \
	{ \
	    const Coord3& dir = dirptr->getData()[index]; \
	    if ( dir.isDefined() ) \
		return dir; \
	} \
    } \
 \
    if ( !computeifudf ) \
	return Coord3::udf(); \
 \
    Coord3 res = compfunc(rc); \
    if ( res.isDefined() ) return res; \
 \
    const TypeSet<RowCol>& directions = RowCol::clockWiseSequence(); \
    for ( int idx=0; idx<directions.size(); idx++ ) \
    { \
	res = compfunc(directions[idx]*step_ + rc); \
	if ( res.isDefined() ) return res; \
    } \
\
    return Coord3::udf(); \

Coord3 CubicBezierSurface::getRowDirection( const RowCol& rc,
					    bool computeifudf ) const
{
    mGetDirectionImpl( rowdirections, computeRowDirection);
}



Coord3 CubicBezierSurface::getColDirection( const RowCol& rc,
					    bool computeifudf ) const
{
    mGetDirectionImpl( coldirections, computeColDirection);
}


float CubicBezierSurface::directionInfluence() const
{ return directioninfluence; }


void CubicBezierSurface::setDirectionInfluence(float ndi)
{
    directioninfluence = ndi;
    triggerMovement();
}


const CubicBezierSurfacePatch*
CubicBezierSurface::getPatch(const RowCol& rc) const
{
    const RowCol rc01( rc.row, rc.col+step_.col );
    const RowCol rc10( rc.row+step_.row, rc.col );
    const RowCol rc11( rc.row+step_.row, rc.col+step_.col );

    return new CubicBezierSurfacePatch( getKnot(rc,true),
	    				getBezierVertex(rc,RowCol(0,1)),
	    				getBezierVertex(rc01,RowCol(0,-1)),
					getKnot(rc01,true ),

    					getBezierVertex(rc,  RowCol(1,0)),
	    				getBezierVertex(rc,  RowCol(1,1)),
	    				getBezierVertex(rc01,RowCol(1,-1)),
	    				getBezierVertex(rc01,RowCol(1,0)),

    					getBezierVertex(rc10,  RowCol(-1,0)),
	    				getBezierVertex(rc10,  RowCol(-1,1)),
	    				getBezierVertex(rc11,  RowCol(-1,-1)),
	    				getBezierVertex(rc11,  RowCol(-1,0)),

    					getKnot(rc10, true ),
	    				getBezierVertex(rc10,  RowCol(0,1)),
	    				getBezierVertex(rc11,  RowCol(0,-1)),
	    				getKnot(rc11, true ));
}


ParametricCurve* CubicBezierSurface::createRowCurve( float row,
				 const Interval<int>* cl ) const
{
    const StepInterval<int> colrange = colRange();
    StepInterval<int> outputrange = colrange;
    if ( cl )
    {
	if ( outputrange.includes( cl->start,false ) )
	    outputrange.start = outputrange.snap(cl->start);
	if ( outputrange.includes( cl->stop,false ) )
	    outputrange.stop = outputrange.snap(cl->stop);
    }

    if ( outputrange.start>=outputrange.stop )
	return 0;

    const Coord3 p0 = computePosition( Coord(row,outputrange.start) );
    const Coord3 p1 = computePosition( Coord(row,outputrange.atIndex(1)) );

    if ( !p0.isDefined() || !p1.isDefined() )
	return 0;

    CubicBezierCurve* res =
	new CubicBezierCurve( p0, p1, outputrange.start, outputrange.step );

    Coord param( row, 0 );
    for ( int idx=outputrange.atIndex(2); idx<=outputrange.stop;
	  idx+=outputrange.step )
    {
	param.y = idx;
	if ( !res->setPosition(idx, computePosition(param)) )
	{ delete res; return 0; }
    }

    return res;
}


ParametricCurve* CubicBezierSurface::createColCurve( float col,
					     const Interval<int>* rw ) const
{
    const StepInterval<int> rowrange = rowRange();
    StepInterval<int> outputrange = rowrange;
    if ( rw )
    {
	if ( outputrange.includes(rw->start,false) )
	    outputrange.start = outputrange.snap( rw->start );
	if ( outputrange.includes(rw->stop,false) )
	    outputrange.stop = outputrange.snap( rw->stop );
    }

    if ( outputrange.start>=outputrange.stop )
	return 0;

    const Coord3 p0 = computePosition( Coord(outputrange.start,col) );
    const Coord3 p1 = computePosition( Coord(outputrange.atIndex(1),col) );

    if ( !p0.isDefined() || !p1.isDefined() )
	return 0;

    CubicBezierCurve* res =
	new CubicBezierCurve( p0, p1, outputrange.start, outputrange.step );

    Coord param( 0, col );
    for ( int idx=outputrange.atIndex(2); idx<=outputrange.stop;
	  idx+=outputrange.step )
    {
	param.x = idx;
	if ( !res->setPosition(idx,computePosition(param)) )
	{ delete res; return 0; }
    }

    return res;
}


bool CubicBezierSurface::checkSelfIntersection( const RowCol& ownrc ) const
{
    if ( !checkselfintersection_ )
	return false;

    const StepInterval<int> rowrange = rowRange();
    const StepInterval<int> colrange = colRange();

    TypeSet<RowCol> affectedrcs;
    RowCol affectedrc;
    for ( RowCol rc(-2,-2); rc.row<=1; rc.row++ )
    {
	affectedrc.row = ownrc.row+rc.row*step_.row;
	if ( !rowrange.includes(affectedrc.row,false) )
	    continue;

	for ( rc.col=-2; rc.col<=1; rc.col++ )
	{
	    affectedrc.col = ownrc.col+rc.col*step_.col;
	    if ( !colrange.includes(affectedrc.col,false) )
		continue;

	    affectedrcs += affectedrc;
	}
    }


    for ( int idx=0; idx<affectedrcs.size(); idx++ )
    {
	affectedrc = affectedrcs[idx];

	const IntervalND<float> ownbbox = boundingBox(affectedrc, false );
	if ( !ownbbox.isSet() ) return false;

	for ( RowCol rc(rowrange.start,0); rc.row<=rowrange.stop;
	      rc.row+=rowrange.step )
	{
	    for ( rc.col = colrange.start; rc.col<=colrange.stop;
		  rc.col+=colrange.step )
	    {
		if ( rc==affectedrc ) continue;
		if ( abs(rc.row-affectedrc.row)<=step_.row &&
		     abs(rc.col-affectedrc.col)<=step_.col )
		{
		    //TODO
		    continue;
		}

		const IntervalND<float> bbox = boundingBox(rc,false);
		if ( !bbox.isSet() ) continue;

		if ( ownbbox.intersects(bbox,false) )
		    return true;
	    }
	}
    }

    return false;
}


#define mGetBBoxVertex( relrow, relcol ) \
vertex = getBezierVertex(currc, RowCol(0,0)); \
if ( vertex.isDefined() ) \
{ \
    if ( bbox.isSet() ) bbox.include(vertex); \
    else bbox.setRange(vertex); \
\
    if ( rowext ) \
    {\
	vertex = getBezierVertex(currc, RowCol(relrow,0)); \
	if ( vertex.isDefined() ) bbox.include(vertex); \
    }\
    if ( rowext && colext )\
    {\
	vertex = getBezierVertex(currc, RowCol(relrow,relcol)); \
	if ( vertex.isDefined() ) bbox.include(vertex); \
    }\
    if ( colext )\
    {\
	vertex = getBezierVertex(currc, RowCol(0,relcol)); \
	if ( vertex.isDefined() ) bbox.include(vertex); \
    }\
}


IntervalND<float> CubicBezierSurface::boundingBox( const RowCol& rc,
						   bool ownvertices ) const
{
    IntervalND<float> bbox(3);

    if ( ownvertices )
    {
	for ( RowCol relrc(-1,-1); relrc.row<=1; relrc.row++ )
	{
	    for ( relrc.col=-1; relrc.col<=1; relrc.col++ )
	    {
		const Coord3 vertex = getBezierVertex( rc, relrc );
		if ( !vertex.isDefined() )
		    continue;

		if ( !bbox.isSet() ) bbox.setRange(vertex);
		else bbox.include(vertex);
	    }
	}
    }
    else
    {
	const bool rowext = rc.row<origin_.row+(nrRows()-1)*step_.row;
	const bool colext = rc.col<origin_.col+(nrCols()-1)*step_.col;
	if ( rowext || colext )
	{
	    Coord3 vertex;
	    RowCol currc(rc);
	    mGetBBoxVertex( 1, 1 );

	    currc.row += step_.row;
	    if ( rowext )
	    { mGetBBoxVertex( -1, 1 ); }

	    currc.col += step_.col;
	    if ( rowext && colext )
	    { mGetBBoxVertex( -1, -1 ); }

	    currc.row = rc.row;
	    if ( colext )
	    { mGetBBoxVertex( 1, -1 ); }
	}
    }

    return bbox;
}


#ifndef mEPS
#define mEPS 1e-10
#endif


#define mComputeDirImpl(_rowcol_) \
    const Coord3* ptr = positions->getData(); \
    int diff = 2*step_._rowcol_; \
    int previndex = getKnotIndex(prev); \
    if ( previndex!=-1 && !ptr[previndex].isDefined() ) previndex=-1; \
 \
    int nextindex = getKnotIndex(next); \
    if ( nextindex!=-1 && !ptr[nextindex].isDefined() ) nextindex=-1; \
 \
    if ( previndex==-1 ) \
    { \
	previndex=getKnotIndex(rc); \
	if ( previndex!=-1 && !ptr[previndex].isDefined() ) previndex=-1; \
	diff = step_._rowcol_; \
    } \
    else if ( nextindex==-1) \
    { \
	nextindex=getKnotIndex(rc); \
	if ( nextindex!=-1 && !ptr[nextindex].isDefined() ) nextindex=-1; \
	diff = step_._rowcol_; \
    } \
 \
    if ( previndex==-1 || nextindex==-1 ) \
	return Coord3::udf(); \
 \
    const Coord3 prevcoord = ptr[previndex]; \
    const Coord3 nextcoord = ptr[nextindex]; \
    if ( prevcoord.sqDistTo(nextcoord)<mEPS ) \
	return Coord3::udf(); \
 \
    return (nextcoord-prevcoord)*step_._rowcol_/diff*directioninfluence;


Coord3 CubicBezierSurface::computeRowDirection( const RowCol& rc ) const
{
    const int lastrow = origin_.row + (nrRows()-1)*step_.row;
    const RowCol prev( rc.row==origin_.row && circularRows() 
	    ? lastrow : rc.row-step_.row, rc.col );
    const RowCol next( rc.row==lastrow && circularRows() 
	    ? origin_.row : rc.row+step_.row, rc.col );

    mComputeDirImpl(row);
}


Coord3 CubicBezierSurface::computeColDirection( const RowCol& rc ) const
{
    const int lastcol = origin_.col + (nrCols()-1)*step_.col;
    const RowCol prev( rc.row, rc.col==origin_.col && circularCols() 
	    ? lastcol : rc.col-step_.col );
    const RowCol next( rc.row, rc.col==lastcol && circularCols() 
	    ? origin_.col : rc.col+step_.col);

    mComputeDirImpl(col);
}


void CubicBezierSurface::_setKnot( int idx, const Coord3& np )
{
    if ( !positions )
    {
	positions = new Array2DImpl<Coord3>( 1, 1 );
	idx = 0;
    }

    positions->getData()[idx] = np;
}


int CubicBezierSurface::nrRows() const
{ return positions ? positions->info().getSize(rowDim()) : 0; }


int CubicBezierSurface::nrCols() const
{ return positions ? positions->info().getSize(colDim()) : 0; }

};

