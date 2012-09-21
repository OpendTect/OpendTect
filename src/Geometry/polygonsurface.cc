/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : July 2008
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "polygonsurface.h"

#include "cubicbeziercurve.h"
#include "polygon.h"
#include "trigonometry.h"
#include "survinfo.h"

namespace Geometry
{


#define mGetValidPolygonIdx( polygonidx, polygonnr, extra, errorres ) \
\
    int polygonidx = polygonnr - firstpolygon_; \
    if ( polygonidx<-extra || polygonidx>polygons_.size()+extra ) \
	return errorres;

#define mGetValidKnotIdx( knotidx, knotnr, polygonidx, extra, errorres ) \
\
    if ( !firstknots_.size() ) return errorres; \
    int knotidx = knotnr - firstknots_[polygonidx]; \
    if ( knotidx<0 && knotidx>polygons_[polygonidx]->size()+extra ) \
	return errorres;

    
PolygonSurface::PolygonSurface()
    : firstpolygon_( 0 )
    , beziernrpts_( 10 )  
{}


PolygonSurface::~PolygonSurface()
{
    deepErase( polygons_ );
}


Element* PolygonSurface::clone() const
{
    PolygonSurface* res = new PolygonSurface;
    deepCopy( res->polygons_, polygons_ );
    res->firstknots_ = firstknots_;
    res->firstpolygon_ = firstpolygon_;
    res->polygonnormals_ = polygonnormals_;
    res->concavedirs_ = concavedirs_;
    res->beziernrpts_ = beziernrpts_;

    return res;
}


char PolygonSurface::bodyDimension() const
{
    const int plygsz = polygons_.size();
    if ( !plygsz )
	return 0;
    
    if ( plygsz==1 )
	return polygons_[0]->size() >= 3 ? 2 : polygons_[0]->size()-1;
    
    int totalpts = 0;
    for ( int idx=0; idx<plygsz; idx++ )
	totalpts += polygons_[idx]->size();
    
    if ( !totalpts ) return 0;
    
    return totalpts>3 ? 3 : totalpts-1;
}


bool PolygonSurface::insertPolygon( const Coord3& firstpos, 
	const Coord3& normal, int polygonnr, int firstknot )
{
    if ( !firstpos.isDefined() || !normal.isDefined() )
	return false;

    if ( polygons_.isEmpty() )
	firstpolygon_ = polygonnr;

    mGetValidPolygonIdx( polygonidx, polygonnr, 1, false );
    if ( polygonidx==-1 )
    {
	firstpolygon_--;
	polygonidx++;
    }

    if ( polygonidx==polygons_.size() )
    {
	polygons_ += new TypeSet<Coord3>;
	polygonnormals_ += normal;
	firstknots_ += firstknot;
	concavedirs_ += normal;
    }
    else
    {
	polygons_.insertAt( new TypeSet<Coord3>, polygonidx );
	polygonnormals_.insert( polygonidx, normal );
	firstknots_.insert( polygonidx,firstknot );
	concavedirs_.insert( polygonidx, normal );
    }

    polygons_[polygonidx]->insert( 0, firstpos );

    triggerNrPosCh( RowCol(polygonidx,PolygonInsert).toInt64() );
    if ( blocksCallBacks() )
	blockCallBacks( true, true );

    return true;
}


bool PolygonSurface::removePolygon( int polygonnr )
{
    mGetValidPolygonIdx( polygonidx, polygonnr, 0, false );

    polygons_.remove( polygonidx );
    polygonnormals_.remove( polygonidx );
    firstknots_.remove( polygonidx );
    concavedirs_.remove( polygonidx );

    triggerNrPosCh( RowCol(polygonidx,PolygonRemove).toInt64() );
    if ( blocksCallBacks() )
	blockCallBacks( true, true );
    
    return true;
}


bool PolygonSurface::insertKnot( const RowCol& rc, const Coord3& pos )
{
    if ( !pos.isDefined() )
	return false;

    mGetValidPolygonIdx( polygonidx, rc.row, 0, false );
    mGetValidKnotIdx( knotidx, rc.col, polygonidx, 1, false );
    if ( knotidx==-1 )
    {
	firstknots_[polygonidx]--;
	knotidx++;
    }

    const int nrknots = polygons_[polygonidx]->size();
    if ( nrknots==3 )
    {
	concavedirs_[polygonidx] = ( ((*polygons_[polygonidx])[1]-
		(*polygons_[polygonidx])[0]).cross( (*polygons_[polygonidx])[2]-
		(*polygons_[polygonidx])[1]) ).normalize();
    }

    if ( knotidx==nrknots )
	(*polygons_[polygonidx]) += pos;
    else
	polygons_[polygonidx]->insert( knotidx, pos );

    triggerNrPosCh( RowCol(polygonidx,PolygonChange).toInt64() );

    return true;
}


bool PolygonSurface::removeKnot( const RowCol& rc )
{
    mGetValidPolygonIdx( polygonidx, rc.row, 0, false );
    mGetValidKnotIdx( knotidx, rc.col, polygonidx, 0, false );

    if ( polygons_[polygonidx]->size() <= 1 )
	return removePolygon( rc.row );

    polygons_[polygonidx]->remove( knotidx );
    triggerNrPosCh( RowCol(polygonidx,PolygonChange).toInt64() );
    
    return true;
}


#define mEmptyInterval() StepInterval<int>( mUdf(int), mUdf(int), mUdf(int) )

StepInterval<int> PolygonSurface::rowRange() const
{
    if ( polygons_.isEmpty() )
	return mEmptyInterval();
    
    return StepInterval<int>(firstpolygon_,firstpolygon_+polygons_.size()-1,1); 
}


StepInterval<int> PolygonSurface::colRange( int polygonnr ) const
{
    mGetValidPolygonIdx( polygonidx, polygonnr, 0, mEmptyInterval() );

    const int firstknot = firstknots_[polygonidx];
    return StepInterval<int>( firstknot,
	    firstknot+polygons_[polygonidx]->size()-1, 1 );
}


void PolygonSurface::setBezierCurveSmoothness( int nrpoints_on_segment )
{
    beziernrpts_ = nrpoints_on_segment;
}


void PolygonSurface::getCubicBezierCurve( int plg, TypeSet<Coord3>& pts, 
					  const float zscale ) const
{
    if ( beziernrpts_<0 )
	return;

    const int polygonidx = plg - firstpolygon_; 
    if ( polygonidx<0 || polygonidx>polygons_.size() ) 
	return;

    const int nrknots = (*polygons_[polygonidx]).size();
    if ( nrknots<3 )
    {
	for ( int idx=0; idx<nrknots; idx++ )
	    pts += (*polygons_[polygonidx])[idx];

	return;
    }

    TypeSet<Coord3> knots;
    for ( int idx=0; idx<nrknots; idx++ )
    {
	Coord3 pos = (*polygons_[polygonidx])[idx]; pos.z *= zscale;
	knots += pos;
    }

    CubicBezierCurve curve( knots[0], knots[1], 0, 1 );
    for( int knot=2; knot<knots.size(); knot++ )
	curve.insertPosition(knot, knots[knot]);
  
    curve.setCircular( true ); 
    for ( int knot=0; knot<nrknots; knot++ )
    {
	const Coord3 prvpos = knots[knot==0 ? nrknots-1 : knot-1];
	const Coord3 nextpos = knots[knot==nrknots-1 ? 0 : knot+1];
	curve.setTangentInfluence( (float) ((prvpos-nextpos).abs())/5.0f );

	for ( int nr=0; nr<beziernrpts_+1; nr++ )
	{
	    Coord3 pt = curve.computePosition(
		    knot+nr*1.0f/(float)(beziernrpts_+1) );
	    
	    pt.z /= zscale;
	    pts += pt;
	}
    }
}


void PolygonSurface::getAllKnots( TypeSet<Coord3>& result ) const
{
    for ( int plg=0; plg<polygons_.size(); plg++ )
    {
	for ( int ptidx=0; ptidx<(*polygons_[plg]).size(); ptidx++ )
	    result += (*polygons_[plg])[ptidx];
    }
}


bool PolygonSurface::setKnot( const RowCol& rc, const Coord3& pos )
{
    if ( !pos.isDefined() )
	return removeKnot( rc );

    mGetValidPolygonIdx( polygonidx, rc.row, 0, false );
    mGetValidKnotIdx( knotidx, rc.col, polygonidx, 0, false );
    (*polygons_[polygonidx])[knotidx] = pos;
    triggerMovement( RowCol(polygonidx,PolygonChange).toInt64() );
    return true;
}


Coord3 PolygonSurface::getKnot( const RowCol& rc ) const
{
    mGetValidPolygonIdx( polygonidx, rc.row, 0, Coord3::udf() );
    mGetValidKnotIdx( knotidx, rc.col, polygonidx, 0, Coord3::udf() );
    
    return (*polygons_[polygonidx])[knotidx];
}


bool PolygonSurface::isKnotDefined( const RowCol& rc ) const
{
    mGetValidPolygonIdx( polygonidx, rc.row, 0, false );
    mGetValidKnotIdx( knotidx, rc.col, polygonidx, 0, false );

    return true;
}


const Coord3& PolygonSurface::getPolygonNormal( int polygon ) const
{
    mGetValidPolygonIdx( polygonidx, polygon, 0, Coord3::udf() );
    if ( polygonidx < polygonnormals_.size() )
	return  polygonnormals_[polygonidx];

    return Coord3::udf();
}


const Coord3& PolygonSurface::getPolygonConcaveDir( int polygon ) const
{
    mGetValidPolygonIdx( polygonidx, polygon, 0, Coord3::udf() );
    if ( polygonidx < concavedirs_.size() )
	return  concavedirs_[polygonidx];

    return Coord3::udf();
}


void PolygonSurface::getPolygonConcaveTriangles( int polygon, 
						 TypeSet<int>& triangles ) const
{
    const StepInterval<int> colrg = colRange( polygon );
    if ( colrg.isUdf() || colrg.nrSteps()<3 )
	return;

    const Coord3 dir = getPolygonConcaveDir( polygon );

    for ( int idx=colrg.start; idx<=colrg.stop; idx += colrg.step )
    {
    	for ( int idy=idx+colrg.step; idy<=colrg.stop; idy += colrg.step )
    	{
	    for ( int idz=idy+colrg.step; idz<=colrg.stop; idz += colrg.step )
	    {
		const Coord3 v0 = getKnot( RowCol(polygon,idx) );
		const Coord3 v1 = getKnot( RowCol(polygon,idy) );
		const Coord3 v2 = getKnot( RowCol(polygon,idz) );
		const Coord3 tridir = ( (v1-v0).cross(v2-v1) ).normalize();
		if ( dir.dot(tridir)<0 )  
		{
		    triangles += idx;
		    triangles += idy;
		    triangles += idz;
		}
	    }
	}
    }
}


bool PolygonSurface::includesEdge( const TypeSet<int> edges, 
				   int v0, int v1 ) const
{
    for ( int idx=0; idx<edges.size()/2; idx++ )
    {
	if ( (edges[2*idx]==v0 && edges[2*idx+1]==v1) ||
	     (edges[2*idx]==v1 && edges[2*idx+1]==v0) )
	    return true;
    }

    return false;
}


void PolygonSurface::getExceptionEdges( int plg, TypeSet<int>& edges ) const
{
    TypeSet<int> triangles;
    getPolygonConcaveTriangles( plg, triangles );
   
    const int nrknots = colRange(plg).nrSteps()+1;
    for ( int ti=0; ti<triangles.size()/3; ti++ )
    {
	if ( triangles[3*ti]==0 && triangles[3*ti+2]==nrknots-1 )
	{
	    if ( !includesEdge( edges, triangles[3*ti], triangles[3*ti+1] ) )
	    {
    		edges += triangles[3*ti];   
    		edges += triangles[3*ti+1];
	    }
	}
	else if ( abs(triangles[3*ti+2]-triangles[3*ti])!=1 )
	{
	    if ( !includesEdge( edges, triangles[3*ti], triangles[3*ti+2] ) )
	    {
    		edges += triangles[3*ti];
    		edges += triangles[3*ti+2];
	    }
	}
    }

    const StepInterval<int> colrg = colRange( plg );
    for ( int idx=colrg.start; idx<colrg.stop-colrg.step; idx += colrg.step )
    {
	const Coord3 v0 = getKnot( RowCol(plg,idx) );
	for ( int idy=idx+2*colrg.step; idy<=colrg.stop; idy += colrg.step )
	{
	    if ( includesEdge(edges,idx,idy) )
	       	continue;

	    const Coord3 v1 = getKnot( RowCol(plg,idy) );	    
	    bool intersecting = false;

	    for ( int idz=colrg.start; idz<colrg.stop; idz += colrg.step )
	    {
		const Coord3 p0 = getKnot( RowCol(plg,idz) );
		const Coord3 p1 = getKnot( RowCol(plg,idz+colrg.step) );
		if ( linesegmentsIntersecting( v0, v1, p0, p1 ) )
		{
		    intersecting = true;
		    break;
		}
	    }

	    if ( !intersecting ) 
		intersecting = linesegmentsIntersecting( v0, v1,
			getKnot( RowCol(plg,colrg.start) ), 
			getKnot( RowCol(plg,colrg.stop) ) );

	    if ( intersecting )
	    {
		edges += idx;
		edges += idy;
	    }
	}
    }
}


bool PolygonSurface::linesegmentsIntersecting( const Coord3& v0,  
	const Coord3& v1,  const Coord3& p0,  const Coord3& p1 ) const
{
    Coord3 norm0 = (v1-v0).cross(p0-v0);
    Coord3 norm1 = (v1-v0).cross(p1-v0);
    if ( norm0.dot(norm1)>0 )
	return false;

    norm0 = (p1-p0).cross(v0-p0);
    norm1 = (p1-p0).cross(v1-p0);
    if ( norm0.dot(norm1)>0 )
	return false;

    Line3 segment0( v0, v1 );
    Line3 segment1( p0, p1 );
    double t0, t1;
    segment0.closestPoint( segment1, t0, t1 );
    return (t0>0 && t0<1) && (t1>0 && t1<1);
}


void PolygonSurface::addEditPlaneNormal( const Coord3& normal )
{
    polygonnormals_ += normal;
    concavedirs_ += normal;
}


void PolygonSurface::addUdfPolygon( int polygonnr, int firstknotnr, int nrknots)
{
    if ( isEmpty() )
	firstpolygon_ = polygonnr;

    firstknots_ += firstknotnr;    
    polygons_ += new TypeSet<Coord3>( nrknots, Coord3::udf() );
}


} // namespace Geometry
