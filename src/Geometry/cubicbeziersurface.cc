/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
-*/

static const char* rcsID = "$Id: cubicbeziersurface.cc,v 1.9 2005-03-10 11:45:04 cvskris Exp $";

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
    const Coord3 linepoint(line.x0, line.y0, line.z0 );
    const Coord3 linedir( line.alpha, line.beta, line.gamma );
    for ( int idx=0; idx<20; idx++ )
    {
	const Coord3 currentpos = computePos(u,v);
	const float sqdist = (currentpos-linepoint).cross(linedir).sqAbs();
	if ( sqdist<eps ) return true;

	const Coord3 upos = computePos(u+1e-3,v);
	const float udist = (upos-linepoint).cross(linedir).sqAbs()-sqdist;

	const Coord3 vpos = computePos(u,v+1e-3);
	const float vdist = (vpos-linepoint).cross(linedir).sqAbs()-sqdist;

 	if ( fabs(udist)>fabs(vdist) )	
	    u = u-(sqdist/udist*1e-3);
	else
	    v = v-(sqdist/vdist*1e-3);

	if ( u<0 || u>1 ) return false;
	if ( v<0 || v>1 ) return false;
    }

    pErrMsg( "Maximum nr of iterations reached" );

    return false;
}


CubicBezierSurface::CubicBezierSurface( const RCol& newstep )
    : ParametricSurface( RowCol(0,0) , newstep )
    , positions( 0 )
    , rowdirections( 0 )
    , coldirections( 0 )
    , directioninfluence( 1.0/3 )
{ }


CubicBezierSurface::CubicBezierSurface( const CubicBezierSurface& b )
    : ParametricSurface( b.origo, b.step )
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
    const StepInterval<int> colrange = rowRange();

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
    const StepInterval<int> colrange = rowRange();

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
    return patch->computePos((params.x-prevrow)/rowrange.step,
	    		     (params.y-prevcol)/colrange.step);
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
    Coord3 closestpointonline = line.closestPoint(center);
    if ( !bbox.includes(closestpointonline) )
	return false;

    RowCol rc(origo);
    for ( int rowidx=0; rowidx<nrRows()-1; rowidx++, rc.row+=step.row )
    {
	rc.col=origo.col;
	for ( int colidx=0; colidx<nrCols()-1; colidx++, rc.col+=step.col )
	{
	    const CubicBezierSurfacePatch* patch = getPatch(rc);
	    if ( !patch ) continue;

	    bbox = patch->computeBoundingBox();
	    if ( !bbox.isSet() ) continue;

	    center.x = bbox.getRange(0).center();
	    center.y = bbox.getRange(1).center();
	    center.z = bbox.getRange(2).center();

	    closestpointonline = line.closestPoint(center);
	    if ( !bbox.includes(closestpointonline) )
		continue;

	    PtrMan<CubicBezierSurfacePatch> dummypatch = 0;
	    const float zfactor = SI().zFactor();
	    Line3 intersectionline( line );
	    if ( !mIsEqual(zfactor,1,1e-3) )
	    {
		dummypatch = patch->clone();
		for ( int idx=dummypatch->nrPos()-1; idx>=0; idx-- )
		    dummypatch->pos[idx].z *= zfactor;

		patch = dummypatch;

		intersectionline.z0 *= zfactor;
		intersectionline.gamma *= zfactor;
	    }

	    float u = res.x;
	    float v = res.y;
	    if ( !patch->intersectWithLine( intersectionline, u, v, 1 ) )
		continue;

	    res.x = u; res.y=v; return true;
	}
    }

    return false;
}


Coord3 CubicBezierSurface::getBezierVertex( const RCol& knot,
	const RCol& relpos ) const
{
    Coord3 pos = getKnot( knot );
    if ( !pos.isDefined() ) return pos;

    if ( relpos.r() )
    {
	const Coord3 rowdir = getRowDirection( knot, true );
	if ( !rowdir.isDefined() ) return rowdir;

	if ( relpos.r()>0 )
	    pos += rowdir.normalize()*directioninfluence;
	else
	    pos -= rowdir.normalize()*directioninfluence;
    }

    if ( relpos.c() )
    {
	const Coord3 coldir = getColDirection( knot, true );
	if ( !coldir.isDefined() ) return coldir;

	if ( relpos.c()>0 )
	    pos += coldir.normalize()*directioninfluence;
	else 
	    pos -= coldir.normalize()*directioninfluence;
    }

    return pos;
}


bool CubicBezierSurface::insertRow(int row)
{
    mInsertStart( rowidx, row, nrRows() );

    TypeSet<GeomPosID> movedpos;
    for ( int idx=rowidx; idx<curnrrows; idx++ )
    {
	const int currow = origo.row+idx*step.row;
	for ( int idy=0; idy<curnrcols; idy++ )
	    movedpos += RowCol(currow,origo.col+idy*step.col).getSerialized();
    }

    TypeSet<GeomPosID> addedpos;
    const int newrow = origo.row+curnrrows*step.row;
    for ( int idy=0; idy<curnrcols; idy++ )
	addedpos += RowCol(newrow,origo.col+idy*step.col).getSerialized();

    mCloneRowVariable( Coord3, positions, computePosition(param), Coord3::udf())
    mCloneRowVariable( Coord3, rowdirections, Coord3::udf(), Coord3::udf() )
    mCloneRowVariable( Coord3, coldirections, Coord3::udf(), Coord3::udf() )

    triggerNrPosCh( addedpos );
    triggerMovement( movedpos );
    return true;
}


bool CubicBezierSurface::insertCol(int col)
{
    mInsertStart( colidx, col, nrCols() );
    TypeSet<GeomPosID> movedpos;
    for ( int idx=colidx; idx<curnrcols; idx++ )
    {
	const int curcol = origo.col+idx*step.col;
	for ( int idy=0; idy<curnrrows; idy++ )
	    movedpos += RowCol(origo.row+idy*step.row,curcol).getSerialized();
    }

    TypeSet<GeomPosID> addedpos;
    const int newcol = origo.col+curnrcols*step.col;
    for ( int idy=0; idy<curnrrows; idy++ )
	addedpos += RowCol(origo.row+idy*step.row,newcol).getSerialized();

    mCloneColVariable( Coord3, positions, computePosition(param), Coord3::udf())
    mCloneColVariable( Coord3, rowdirections, Coord3::udf(), Coord3::udf() )
    mCloneColVariable( Coord3, coldirections, Coord3::udf(), Coord3::udf() )

    triggerNrPosCh( addedpos );
    triggerMovement( movedpos );
    return true;
}


bool CubicBezierSurface::removeRow( int row )
{
    pErrMsg( "not implemented ");
    return true;
}


bool CubicBezierSurface::removeCol( int col )
{
    pErrMsg( "not implemented ");
    return true;
}


Coord3 CubicBezierSurface::getKnot( const RCol& rc, bool estimateifundef ) const
{
    const int index = getKnotIndex(rc);
    if ( index==-1 ) return Coord3::udf();

    const Coord3* data = positions->getData();

    if ( estimateifundef && !data[index].isDefined() )
    {
	RowCol neighborrc( rc );
	neighborrc.row = rc.r()-step.row;
	Coord3 pos = getKnot( neighborrc, false );
	if ( pos.isDefined() )
	{
	    const Coord3 dir = getRowDirection( neighborrc, true );
	    if ( dir.isDefined() )
		return pos+dir;
	}

	neighborrc.row = rc.r()+step.row;
	pos = getKnot( neighborrc, false );
	if ( pos.isDefined() )
	{
	    const Coord3 dir = getRowDirection( neighborrc, true );
	    if ( dir.isDefined() )
		return pos-dir;
	}

	neighborrc.row = rc.r();
	neighborrc.col = rc.c()-step.col;
	pos = getKnot( neighborrc, false );
	if ( pos.isDefined() )
	{
	    const Coord3 dir = getColDirection( neighborrc, true );
	    if ( dir.isDefined() )
		return pos+dir;
	}

	neighborrc.col = rc.c()+step.col;
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


float CubicBezierSurface::directionInfluence() const
{ return directioninfluence; }


void CubicBezierSurface::setDirectionInfluence(float ndi)
{
    directioninfluence = ndi;
    triggerMovement();
}


const CubicBezierSurfacePatch*
CubicBezierSurface::getPatch(const RCol& rc) const
{
    const RowCol rc01( rc.r(), rc.c()+step.col );
    const RowCol rc10( rc.r()+step.row, rc.c() );
    const RowCol rc11( rc.r()+step.row, rc.c()+step.col );

    return new CubicBezierSurfacePatch( getKnot(rc,true),
	    				getBezierVertex(rc,RowCol(0,1)),
	    				getBezierVertex(rc01,RowCol(0,-1)),
					getKnot(rc01,true ),

    					getBezierVertex(rc,  RowCol(1,0)),
	    				getBezierVertex(rc,  RowCol(1,1)),
	    				getBezierVertex(rc01,RowCol(1,-1)),
	    				getBezierVertex(rc01,RowCol(1,-1)),

    					getBezierVertex(rc10,  RowCol(-1,0)),
	    				getBezierVertex(rc10,  RowCol(-1,1)),
	    				getBezierVertex(rc11,  RowCol(-1,-1)),
	    				getBezierVertex(rc11,  RowCol(-1,-1)),

    					getBezierVertex(rc10,  RowCol(0,0)),
	    				getBezierVertex(rc10,  RowCol(0,1)),
	    				getBezierVertex(rc11,  RowCol(0,-1)),
	    				getBezierVertex(rc11,  RowCol(0,-1)) );
    }


bool CubicBezierSurface::checkSelfIntersection( const RCol& ownrc ) const
{
    const StepInterval<int> rowrange = rowRange();
    const StepInterval<int> colrange = colRange();

    TypeSet<RowCol> affectedrcs;
    RowCol affectedrc;
    for ( RowCol rc(-2,-2); rc.row<=1; rc.row++ )
    {
	affectedrc.row = ownrc.r()+rc.row*step.row;
	if ( !rowrange.includes(affectedrc.row) )
	    continue;

	for ( rc.col=-2; rc.col<=1; rc.col++ )
	{
	    affectedrc.col = ownrc.c()+rc.col*step.col;
	    if ( !colrange.includes(affectedrc.col) )
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
		if ( abs(rc.row-affectedrc.r())<=step.row &&
		     abs(rc.col-affectedrc.c())<=step.col )
		{
		    //TODO
		    continue;
		}

		const IntervalND<float> bbox = boundingBox(rc,false);
		if ( !bbox.isSet() ) continue;

		if ( ownbbox.intersects(bbox) )
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


IntervalND<float> CubicBezierSurface::boundingBox( const RCol& rc,
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
	const bool rowext = rc.r()<origo.row+(nrRows()-1)*step.row;
	const bool colext = rc.c()<origo.col+(nrCols()-1)*step.col;
	if ( rowext || colext )
	{
	    Coord3 vertex;
	    RowCol currc(rc);
	    mGetBBoxVertex( 1, 1 );

	    currc.row += step.row;
	    if ( rowext )
	    { mGetBBoxVertex( -1, 1 ); }

	    currc.col += step.col;
	    if ( rowext && colext )
	    { mGetBBoxVertex( -1, -1 ); }

	    currc.row = rc.r();
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
    int diff = 2*step._rowcol_; \
    int previndex = getKnotIndex(prev); \
    if ( previndex!=-1 && !ptr[previndex].isDefined() ) previndex==-1; \
 \
    int nextindex = getKnotIndex(next); \
    if ( nextindex!=-1 && !ptr[nextindex].isDefined() ) nextindex==-1; \
 \
    if ( previndex==-1 ) \
    { \
	previndex=getKnotIndex(rc); \
	if ( previndex!=-1 && !ptr[previndex].isDefined() ) previndex==-1; \
	diff = step._rowcol_; \
    } \
    else if ( nextindex==-1) \
    { \
	nextindex=getKnotIndex(rc); \
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
    return (nextcoord-prevcoord)*step._rowcol_/diff*directioninfluence;


Coord3 CubicBezierSurface::computeRowDirection( const RCol& rc ) const
{
    const int lastrow = origo.row + (nrRows()-1)*step.row;
    const RowCol prev( rc.r()==origo.row && circularRows() 
	    ? lastrow : rc.r()-step.row, rc.c() );
    const RowCol next( rc.r()==lastrow && circularRows() 
	    ? origo.row : rc.r()+step.row, rc.c() );

    mComputeDirImpl(row);
}


Coord3 CubicBezierSurface::computeColDirection( const RCol& rc ) const
{
    const int lastcol = origo.col + (nrCols()-1)*step.col;
    const RowCol prev( rc.r(), rc.c()==origo.col && circularCols() 
	    ? lastcol : rc.c()-step.col );
    const RowCol next( rc.r(), rc.c()==lastcol && circularCols() 
	    ? origo.col : rc.c()+step.col);

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

