/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y. Liu
 * DATE     : March 2010
-*/


#include "faulthorintersect.h"

#include "binidsurface.h"
#include "indexedshape.h"
#include "positionlist.h"
#include "survinfo.h"
#include "trigonometry.h"
#include "explfaultsticksurface.h"
#include "isocontourtracer.h"


#define mDistLimitation 10.0f
namespace Geometry
{


class FBIntersectionCalculator : public ParallelTask
{
public:
FBIntersectionCalculator( const BinIDSurface& surf, float surfshift, 
	const ExplFaultStickSurface& shape )
    : surf_( surf )
    , surfzrg_( -1, -1 )
    , zshift_( surfshift )
    , shape_( shape )
{}

~FBIntersectionCalculator()		{ stickintersections_.erase(); }
od_int64 nrIterations() const   	{ return shape_.getGeometry().size(); }
TypeSet<Coord3>& result() 		{ return finalres_; }

bool doPrepare( int )
{
    for ( int idx=0; idx<nrIterations(); idx++ )
	stickintersections_ += TypeSet<Coord3>();

    const Array2D<float>* depths = surf_.getArray();
    const float* data = depths ? depths->getData() : 0;
    if ( !data )
	return false;

    bool found = false;
    const int totalsz = mCast( int, depths->info().getTotalSz() );
    for ( int idx=0; idx<totalsz; idx++ )
    {
	if ( mIsUdf(data[idx]) )
	    continue;

	if ( !found )
	{
	    found = true;
	    surfzrg_.start = surfzrg_.stop = data[idx];
	}
	else
	    surfzrg_.include( data[idx] );
    }

    return found;
}


bool doFinish( bool )
{
    finalres_.erase();
    for ( int idx=0; idx<stickintersections_.size(); idx++ )
    {
	if ( stickintersections_[idx].size()>0 )
		finalres_.append( stickintersections_[idx] );
    }

    if ( finalres_.size()==0 )
	return false;

    int minidx = -1;
    if ( !findMin(finalres_, minidx, true) )
    {
	if ( !findMin(finalres_,minidx,false) )
	    return false;
    }

    if ( minidx> 1 )
    {
	const Coord3 mincrd = finalres_[minidx];
	finalres_.removeSingle( minidx );
	finalres_.insert( 0, mincrd );
    }
  
    return true;
 }


bool findMin( TypeSet<Coord3>& res, int& minidx, bool isx )
{
    if ( res.size()==0 ) return false;
 
    double minval = isx ? res[0].x : res[0].y;
    minidx = 0;
    Coord3 mincrd;
    double deltaval = 0;
    
    for ( int idx = 1; idx<res.size(); idx++ )
    {
	const double val = isx ? res[idx].x : res[idx].y;
	if ( val < minval )
	{
	    minval = val;
	    minidx = idx;
	}
    }

    bool side1 = false;
    bool side2 = false;

    for ( int idx = 0; idx<res.size(); idx++ )
    {
	const double val = isx ? res[idx].x : res[idx].y;
	deltaval += val - minval;

	if ( side1 && side2 )
	    break;
		
	const double anotherval = isx ? res[idx].y : res[idx].x;
	const double minpairval = isx ? res[minidx].y : res[minidx].x;
	
	bool diff = anotherval - minpairval>0 ? true : false;
	
	if ( !side1 && diff )
	    side1 =  true;
	else if ( !side2 && !diff )
	    side2 = true;
    }

    return (side1 && side2) ? false : ( (deltaval-mDistLimitation)>0 ? true : 
	false );
}


bool doWork( od_int64 start, od_int64 stop, int )
{
    const float zscale = SI().zScale();
    const StepInterval<int>& surfrrg = surf_.rowRange();
    const StepInterval<int>& surfcrg = surf_.colRange();
    ConstRefMan<Coord3List> coordlist = shape_.coordList();

    for ( int idx=mCast(int,start); idx<=stop; idx++ )
    {	
    	const IndexedGeometry* inp = shape_.getGeometry()[idx];
	if ( !inp ) continue;

	IndexedGeometry* idxgeom = const_cast<IndexedGeometry*> (inp);
	Geometry::PrimitiveSet* geomps = idxgeom->getCoordsPrimitiveSet();

	TypeSet<Coord3>& res = stickintersections_[idx];
	for ( int idy=0; idy<geomps->size()-3; idy+=4 )
	{
	    Coord3 v[3];
	    for ( int k=0; k<3; k++ )
		v[k] = coordlist->get(geomps->get(idy+k));
	    const Coord3 center = (v[0]+v[1]+v[2])/3;
      
	    Interval<int> trrg, tcrg; 
	    Coord3 rcz[3];
	    bool allabove = true;
	    bool allbelow = true;
	    for ( int k=0; k<3; k++ )
	    {
		BinID bid = SI().transform( v[k] );
		RowCol rc(surfrrg.snap(bid.inl()),surfcrg.snap(bid.crl()));

		const double pz = surf_.getKnot(rc, false).z + zshift_;
		rcz[k] = Coord3( rc.row(), rc.col(), pz );
		bool defined = !mIsUdf(pz);
		if ( allabove )
    		    allabove = defined ? v[k].z>=pz : v[k].z >= surfzrg_.stop;
		if ( allbelow )
    		    allbelow = defined ? v[k].z<=pz : v[k].z <= surfzrg_.start;

		if ( !k )
		{
		    trrg.start = trrg.stop = rc.row();
		    tcrg.start = tcrg.stop = rc.col();
		}
		else
		{
		    trrg.include( rc.row() );
		    tcrg.include( rc.col() );
		}
	    }

	    if ( trrg.start > surfrrg.stop || trrg.stop < surfrrg.start ||
		 tcrg.start > surfcrg.stop || tcrg.stop < surfcrg.start ||
		 allabove || allbelow ) 
		continue; 

	    Coord3 tri[3];
	    for ( int k=0; k<3; k++ )
	    {
		tri[k] = v[k] - center;
		tri[k].z *= zscale;
	    }
	    Plane3 triangle( tri[0], tri[1], tri[2] );
	   
	    trrg.start = surfrrg.snap( trrg.start );
	    trrg.stop = surfrrg.snap( trrg.stop );
	    tcrg.start = surfcrg.snap( tcrg.start );
	    tcrg.stop = surfcrg.snap( tcrg.stop );

	    StepInterval<int> smprrg( mMAX(surfrrg.start, trrg.start),
		    mMIN(surfrrg.stop, trrg.stop), surfrrg.step );
	    StepInterval<int> smpcrg( mMAX(surfcrg.start, tcrg.start),
		    mMIN(surfcrg.stop, tcrg.stop), surfcrg.step );
	    const int smprsz = smprrg.nrSteps()+1;
	    const int smpcsz = smpcrg.nrSteps()+1;
	    Array2DImpl<float> field( smprsz, smpcsz );
	    for ( int ridx=0; ridx<smprsz; ridx++ )
	    {
		for ( int cidx=0; cidx<smpcsz; cidx++ )
		{
		    const int row = smprrg.atIndex( ridx );
		    const int col = smpcrg.atIndex( cidx );
		    Coord3 pos = surf_.getKnot(RowCol(row,col), false);
		    float dist = mUdf( float );
		    if ( !mIsUdf(pos.z) )
		    {
			pos.z += zshift_;
			pos -= center;
			pos.z *= zscale;
			dist = (float) triangle.distanceToPoint(pos,true);
		    }

		    field.set( ridx, cidx, dist );
		}
	    }

	    IsoContourTracer ictracer( field );
	    ictracer.setSampling( smprrg, smpcrg );
	    ObjectSet<ODPolygon<float> > isocontours;
	    ictracer.getContours( isocontours, 0, false );
	    
	    TypeSet<Coord3> tmp;
	    for ( int cidx=0; cidx<isocontours.size(); cidx++ )
	    {
		const ODPolygon<float>& ic = *isocontours[cidx];
		const bool isclosed = ic.isClosed();
		Coord3 firstpos(0,0,mUdf(float));

		for ( int vidx=0; vidx<ic.size(); vidx++ )
		{
		    const Geom::Point2D<float> vertex = ic.getVertex( vidx );
		    if ( !pointInTriangle2D( Coord(vertex.x,vertex.y),
				rcz[0],rcz[1],rcz[2],0) )
			continue;

		    Coord3 intersect;
		    if ( !getSurfacePos(vertex,intersect) )
			continue;

		    if ( tmp.isPresent(intersect) )
			continue;

		    Coord3 temp = intersect - center;
		    temp.z *= zscale;
		    if ( pointInTriangle3D(temp,tri[0],tri[1],tri[2],0) )
		    {
			tmp += intersect;
			if ( isclosed && mIsUdf(firstpos.z) )
			    firstpos = intersect;
		    }
		}
		
		if ( isclosed && !mIsUdf(firstpos.z) )
		    tmp += firstpos; 
	    }
	    if ( tmp.size()>0 )
		res.append( tmp );
	    deepErase( isocontours );
	}
    }

    return true;
}

protected:

bool getSurfacePos( const Geom::Point2D<float>& vertex, Coord3& res )
{
    const int minrow = (int)vertex.x;
    const int maxrow = minrow < vertex.x ? minrow+1 : minrow;
    const int mincol = (int)vertex.y;
    const int maxcol = mincol < vertex.y ? mincol+1 : mincol;

    TypeSet<Coord3> neighbors;
    TypeSet<float> weights;
    float weightsum = 0;
    for ( int r=minrow; r<=maxrow; r++ )
    {
	for ( int c=mincol; c<=maxcol; c++ )
	{
	    Coord3 pos = surf_.getKnot( RowCol(r,c), false );
	    if ( mIsUdf(pos.z) )
		continue;
	    else
		pos.z += zshift_;

	    float dist = fabs(r-vertex.x) + fabs(c-vertex.y);
	    if ( mIsZero(dist,1e-5) )
	    {
		res = pos;
		return true;
	    }
	    else
		dist = 1.f/dist;
	    
	    weights += dist;
	    weightsum += dist;
	    neighbors += pos;
	}
    }

    if ( !neighbors.size() )
	return false;

    res = Coord3(0,0,0);
    for ( int pidx=0; pidx<neighbors.size(); pidx++ )
	res += neighbors[pidx] * weights[pidx];
    res /= weightsum; 

    return true;
}


const ExplFaultStickSurface&	shape_;
const BinIDSurface&		surf_;
Interval<float>			surfzrg_;
float				zshift_;
TypeSet<TypeSet<Coord3> >	stickintersections_;
TypeSet<Coord3>			finalres_;
};


FaultBinIDSurfaceIntersector::FaultBinIDSurfaceIntersector( float horshift,
	const BinIDSurface& surf, const ExplFaultStickSurface& eshape, 
	Coord3List& cl )
    : surf_( surf )
    , crdlist_( cl )
    , output_( 0 )
    , zshift_( horshift )
    , eshape_( eshape )		
{}


void FaultBinIDSurfaceIntersector::setShape( const IndexedShape& ns )
{
    delete output_;
    output_ = &ns;
}


const IndexedShape* FaultBinIDSurfaceIntersector::getShape( bool takeover ) 
{ 
    return takeover ? output_ : new IndexedShape(*output_); 
}


void FaultBinIDSurfaceIntersector::compute()
{
    FBIntersectionCalculator calculator( surf_, zshift_, eshape_ );
    if ( !calculator.execute() )
	return;

    TypeSet<Coord3>& calcres = calculator.result();
    TypeSet<Coord3> res;
    sortPointsToLine( calcres, res );
    
    if ( res.size()==0 )
	return;

    const int possize = optimizeOrder( res );

    IndexedGeometry* geo = !output_ || !output_->getGeometry().size() ? 0 :
	const_cast<IndexedGeometry*>(output_->getGeometry()[0]);

    if ( !geo )
    {
    	for ( int idx=0; idx<possize; idx++ )
    	    crdlist_.add( res[idx] );
	return;
    }
    
    geo->removeAll( false );
    Geometry::PrimitiveSet* idxps = geo->getCoordsPrimitiveSet();
    idxps->setEmpty();

    for ( int idx=0; idx<possize; idx++ )
	idxps->append( crdlist_.add( res[idx] ) );

    geo->ischanged_ = true;
}


int FaultBinIDSurfaceIntersector::optimizeOrder( TypeSet<Coord3>& res )
{
    IntervalND<float> bbox(2);
    bbox.setRange( Coord(res[0].x,res[0].y) );
    int detectnr = 0;

    TypeSet<int> idxs;
    for ( int idx = 1; idx<res.size(); idx++ )
    {
	 const Coord xy( res[idx].x,res[idx].y );
	 if ( bbox.getRange(0).start<xy.x && bbox.getRange(0).stop>xy.x &&
	      bbox.getRange(1).start<xy.y && bbox.getRange(1).stop>xy.y )
	 {
	     detectnr ++;
	     idxs += idx;
	 }
	 else
	 {
	     bbox.include( xy );
	 }
    }

    if ( idxs.size()>0 )
    {
	 for ( int idx = idxs.size()-1; idx>=0; idx-- )
	     res.removeSingle( idxs[idx] );
    }

    return res.size();
}


void FaultBinIDSurfaceIntersector::sortPointsToLine( 
    TypeSet<Coord3>& in, TypeSet<Coord3>& out )
{
    out.erase();
    if ( in.isEmpty() )
	return;

    const Coord3 startpoint = in[0];
    Coord3 pnt = findNearestPoint( startpoint, in );

    while ( pnt.isDefined() )
    {
	Coord3 lastpnt;
	if ( out.size()>0 )
	    lastpnt = out[out.size()-1];

	if ( lastpnt.sqDistTo(pnt) >mDistLimitation || out.size()==0 )
	    out += pnt;
        pnt = findNearestPoint( out[out.size()-1], in );
    }
}


const Coord3 FaultBinIDSurfaceIntersector::findNearestPoint( 
    const Coord3& pnt,	TypeSet<Coord3>& res )
{
    double dist = MAXFLOAT;
    int distidx = -1;
    for ( int idx=0; idx<res.size(); idx++ )
    {
	const double disval = pnt.sqDistTo( res[idx] );
	if ( disval< dist )
	{
	    dist = disval;
	    distidx = idx;
	}
    }

    if ( distidx==-1 )
	return  Coord3().udf();

    const Coord3 retpnt = res[distidx];
    res.removeSingle( distidx );

    return retpnt;
}


};
