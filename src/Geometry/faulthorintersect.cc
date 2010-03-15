/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y. Liu
 * DATE     : March 2010
-*/

static const char* rcsID = "$Id: faulthorintersect.cc,v 1.1 2010-03-15 19:24:57 cvsyuancheng Exp $";

#include "faulthorintersect.h"

#include "binidsurface.h"
#include "faultstickset.h"
#include "indexedshape.h"
#include "positionlist.h"
#include "ranges.h"
#include "survinfo.h"


namespace Geometry
{

FaultBinIDSurfaceIntersector::FaultBinIDSurfaceIntersector( float horshift,
	const BinIDSurface& sf, const FaultStickSet& ft, Coord3List& cl )
    : ft_( ft )
    , surf_( sf )
    , crdlist_( cl )
    , output_( 0 )
    , zshift_( horshift ) 
{
    doPrepare();
}


FaultBinIDSurfaceIntersector::~FaultBinIDSurfaceIntersector()	
{ 
    deepErase( ftbids_ ); 
    deepErase( zrgs_ );
    deepErase( stickitsinfo_ );
}


void FaultBinIDSurfaceIntersector::doPrepare()
{
    const StepInterval<int> rrg = surf_.rowRange();
    const StepInterval<int> crg = surf_.colRange();

    for ( int idx=0; idx<ft_.nrSticks(); idx++ )
    {
	ftbids_ += new TypeSet<BinID>;
	zrgs_ += new Interval<float>();
	stickitsinfo_ += new IntSectInfo();
	
	const TypeSet<Coord3>* knots = ft_.getStick(idx);
	if ( !knots )
	    continue;
    
	bool defined = false;
	for ( int idy=0; idy<knots->size(); idy++ )
	{
	    const BinID bid = SI().transform( (*knots)[idy] );
	    (*ftbids_[idx]) += bid;

	    if ( bid.inl<rrg.start || bid.inl>rrg.stop ||
		 bid.crl<crg.start || bid.crl>crg.stop )
		continue;
	    
	    const BinID horbid( rrg.snap(bid.inl), crg.snap(bid.crl) );
	    const double zv = surf_.getKnot(horbid,false).z + zshift_;
	    
	    if ( !mIsUdf(zv) )
	    {
		if ( !defined )
		{
		    (*zrgs_[idx]).start = (*zrgs_[idx]).stop = zv;
		    defined = true;
		}
		else
		    (*zrgs_[idx]).include( zv );
	    }
	}
    }
}


void FaultBinIDSurfaceIntersector::compute()
{
    for ( int idx=0; idx<ft_.nrSticks(); idx++ )
	computeStickIntersectionInfo( idx );

    addIntersectionsToList();
}    	


void FaultBinIDSurfaceIntersector::setShape( const IndexedShape& ns )
{
    delete output_;
    output_ = &ns;
}


const IndexedShape* FaultBinIDSurfaceIntersector::getShape( bool takeover ) 
{ 
    return takeover ? output_ : new IndexedShape(*output_); 
}


void FaultBinIDSurfaceIntersector::addIntersectionsToList()
{
    const bool geoexit = output_ && output_->getGeometry()[0];
    IndexedGeometry* geo = geoexit ? 
	const_cast<IndexedGeometry*>( output_->getGeometry()[0] ) : 0;
    if ( geo ) geo->removeAll();
    
    for ( int idx=0; idx<ft_.nrSticks()-1; idx++ )
    {
	const int knotidx0 = (*stickitsinfo_[idx]).lowknotidx;
	const int knotidx1 = (*stickitsinfo_[idx+1]).lowknotidx;
	if ( knotidx0!=-1 && knotidx1!=-1 )
	{
	    int nid0 = crdlist_.add( (*stickitsinfo_[idx]).intsectpos );
	    int nid1 = crdlist_.add( (*stickitsinfo_[idx+1]).intsectpos );
	    if ( geo )
	    {
	    	geo->coordindices_ += nid0;
    		geo->coordindices_ += nid1;
	    }
	}
    }

    if ( geo )
    	geo->ischanged_ = true;

    /*TODO: more details
    Coord3 a, b, c, res;
    for ( int idx=0; idx<ft_.nrSticks()-1; idx++ )
    {
	const TypeSet<Coord3>& knots = *ft_.getStick(idx);
	const int nrknots = knots.size();
	if ( !nrknots )
	    continue;

	const TypeSet<Coord3>& nextknots = *ft_.getStick(idx+1);
	const int knotidx0 = (*stickitsinfo_[idx]).lowknotidx;
	const int knotidx1 = (*stickitsinfo_[idx+1]).lowknotidx;

	if ( nrknots==1 && nextknots.size()==1 )
	    continue;
	
	if ( knotidx0 == -1 )
	{
	    a = knots[0];
	    if ( knotidx1==-1 )
	    {
		b = nextknots[0];
		//Go through all?
	    }
	    else
	    {
    		b = nextknots[knotidx1];
    		c = nextknots[knotidx1+1];
	    }
	}
	else
	{
	    a = knots[knotidx0];
	    b = knots[knotidx0+1];
	    if ( knotidx1 != -1 )
    		c = nextknots[knotidx1];
	    else
		c = nextknots[0];//the one connect a b
	}
	
	TypeSet<Coord3>* pts = new TypeSet<Coord3>();
	(*pts) += (*stickitsinfo_[idx]).intsectpos;
	

	result_ += pts;
	
	
    }
    */
}

void FaultBinIDSurfaceIntersector::computeStickIntersectionInfo( int stkidx )
{
    (*stickitsinfo_[stkidx]).lowknotidx = -1;
    const TypeSet<BinID>& stick = *ftbids_[stkidx];
    const int nrknots = stick.size();
    if ( nrknots < 2 ) return;

    const TypeSet<Coord3>& knots = *ft_.getStick(stkidx);
    const StepInterval<int> rrg = surf_.rowRange();
    const StepInterval<int> crg = surf_.colRange();
    for ( int idx=0; idx<nrknots-1; idx++ )
    {
	float z0 = knots[idx].z;
	float z1 = knots[idx+1].z;
	if ( z0>z1 ) { float tmp = z0; z0 = z1; z1 = tmp; }

	if ( z1 < (*zrgs_[stkidx]).start || z0 > (*zrgs_[stkidx]).stop )
	    continue;

	const BinID hb0( rrg.snap(stick[idx].inl), crg.snap(stick[idx].crl) );
	const BinID hb1( rrg.snap(stick[idx+1].inl),crg.snap(stick[idx+1].crl));

	Coord3 a = surf_.getKnot( RowCol(hb0.inl,hb0.crl), false );
	a.z += zshift_;
	Coord3 b = surf_.getKnot( RowCol(hb1.inl,hb1.crl), false );
	b.z += zshift_;
	double zd0 = a.z - knots[idx].z; if ( zd0<0 ) zd0 = -zd0;
	double zd1 = b.z - knots[idx+1].z; if ( zd1<0 ) zd1 = -zd1;
	
	(*stickitsinfo_[stkidx]).intsectpos = 
	    zd0+zd1 > 0 ? a + (b-a) * zd0 / (zd0+zd1) : a;
	(*stickitsinfo_[stkidx]).lowknotidx = idx;
    }	
}

/*
bool FaultBinIDSurfaceIntersector::triangleIntersetLineSegment( 
	Coord3 a, Coord3 b, Coord3 c, Coord3 k0, Coord3 k1, Coord3& res )
{
    Line3 line( k0, k1-k0 );
    Plane3 plane( a, b, c );

    double t;
    if ( !line.intersectWith(plane,t) )
	return false;
    
    const Coord3 itsect = line.getPoint( t );
    double epsilon = (a-b).abs() * 0.001;

    if ( !pointInTriangle3D(itsect,a,b,c, epsilon) )
	return false;

    res = itsect;
    return true;
}
*/

};

