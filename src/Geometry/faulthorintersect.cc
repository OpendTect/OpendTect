/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y. Liu
 * DATE     : March 2010
-*/

static const char* rcsID = "$Id: faulthorintersect.cc,v 1.9 2011-02-09 16:54:35 cvsyuancheng Exp $";

#include "faulthorintersect.h"

#include "arraynd.h"
#include "binidsurface.h"
#include "faultstickset.h"
#include "indexedshape.h"
#include "positionlist.h"
#include "ranges.h"
#include "survinfo.h"
#include "trigonometry.h"

#define mKnotBelowHor		-1
#define mStickAwayHor		2
#define mKnotAboveHor		1
#define mStickIntersectsHor	0


namespace Geometry
{

class FaultStickHorizonIntersector : public ParallelTask
{
public:    
FaultStickHorizonIntersector( FaultBinIDSurfaceIntersector& fhi )
    : fhi_( fhi )
    , bidsurfzrg_( -1, -1 )
{}


od_int64 nrIterations() const	{ return fhi_.ft_.nrSticks(); }
const char* message() const	{ return "Calculate stick intersections"; }


bool doPrepare( int )
{
    const Array2D<float>* depths = fhi_.surf_.getArray();
    const float* data = depths ? depths->getData() : 0;
    if ( !data )
	return false;

    bool found = false;
    const int totalsz = depths->info().getTotalSz();
    for ( int idx=0; idx<totalsz; idx++ )
    {
	if ( mIsUdf(data[idx]) )
	    continue;

	if ( !found )
	{
	    found = true;
	    bidsurfzrg_.start = bidsurfzrg_.stop = data[idx];
	}
	else
	    bidsurfzrg_.include( data[idx] );
    }

    if ( !found )
	return false;

    const StepInterval<int> rrg = fhi_.surf_.rowRange();
    TypeSet<RowCol> rcs;
    TypeSet<Coord3> poses;
    for ( int row=rrg.start; row<=rrg.stop; row += rrg.step )
    {
	if ( poses.size()==3 )
	    break;

	StepInterval<int> crg = fhi_.surf_.colRange( row );
	for ( int col=crg.start; col<=crg.stop; col +=crg.step )
	{
	    RowCol rc(row,col);
	    const Coord3 pos = fhi_.surf_.getKnot( rc, false );
	    if ( mIsUdf(pos.z) )
		continue;

	    if ( poses.size()<2 )
	    {
		poses += pos;
		rcs += rc;
	    }
	    else
	    {
		if ( rcs[0].row==rcs[1].row && rcs[1].row==row )
    		    break;

		if ( rcs[0].col==rcs[1].col && rcs[1].col==col )
		    continue;

		poses += pos;
		break;
	    }
	}
    }

    if ( poses.size()<3 )
	return false;

    plane_.set( poses[0], poses[1], poses[2] );

    for ( int idx=0; idx<nrIterations(); idx++ )
    {
	fhi_.ftbids_ += new TypeSet<BinID>();
	FaultBinIDSurfaceIntersector::StickIntersectionInfo* stickinfo = 
	    new FaultBinIDSurfaceIntersector::StickIntersectionInfo();
	stickinfo->lowknotidx = -1;
	fhi_.itsinfo_ += stickinfo;
    }

    return true;
}


#define mSetIntersectionInfo( horpos, idy ) \
    (*fhi_.itsinfo_[idx]).intsectpos = horpos; \
    (*fhi_.itsinfo_[idx]).lowknotidx = idy; \
    (*fhi_.itsinfo_[idx]).intersectstatus = mStickIntersectsHor; \
    found = true; \
    break


#define mContIfOnOneSide() \
    if ( (horpos0.z < prevz && horpos1.z < nextz) || \
	 (horpos0.z > prevz && horpos1.z > nextz) ) \
	continue

bool doWork( od_int64 start, od_int64 stop, int )
{
    const StepInterval<int>& surfrrg = fhi_.surf_.rowRange();
    const StepInterval<int>& surfcrg = fhi_.surf_.colRange();

    for ( int idx=start; idx<=stop; idx++ )
    {
	const TypeSet<Coord3>& knots = *fhi_.ft_.getStick(idx);
	const int lastknotidx = knots.size() - 1;
	if ( lastknotidx<0 ) continue;

	bool found = false;
	BoolTypeSet definedp;
	TypeSet<Coord3> horposes;
	TypeSet<RowCol> rcs;
	Interval<float> stickzrg;
	for ( int idy=0; idy<=lastknotidx; idy++ )
	{
	    if ( !idy )
		stickzrg.start = stickzrg.stop = knots[idy].z;
	    else
		stickzrg.include( knots[idy].z );

	    const BinID bid = SI().transform( knots[idy] );
	    (*fhi_.ftbids_[idx]) += bid;

	    rcs += RowCol( surfrrg.snap(bid.inl), surfcrg.snap(bid.crl) );

	    Coord3 horpos = fhi_.surf_.getKnot( rcs[idy], false );
	    if ( !mIsUdf(horpos.z) )
	    {
		horpos.z += fhi_.zshift_;
		definedp += true;
		if ( mIsEqual( horpos.z, knots[idy].z,
			       (horpos.z+knots[idy].z) * 5e-4 ) )
		{
		    found = true;
		    mSetIntersectionInfo( horpos, idy );
		}
	    }
	    else
		definedp += false;

	    horposes += horpos;
	}

	if ( found )
	    continue;

	if ( stickzrg.start > bidsurfzrg_.stop )
	{
	    (*fhi_.itsinfo_[idx]).intersectstatus = mKnotAboveHor;
	    continue;
	}
	else if ( stickzrg.stop < bidsurfzrg_.start )
	{
	    (*fhi_.itsinfo_[idx]).intersectstatus = mKnotBelowHor;
	    continue;
	}

	for ( int idy=0; idy<lastknotidx; idy++ )
	{
	    const float prevz = knots[idy].z;
	    const float nextz = knots[idy+1].z;
	    if ( (prevz > bidsurfzrg_.stop && nextz > bidsurfzrg_.stop) || 
		 (prevz < bidsurfzrg_.start && nextz < bidsurfzrg_.start) ||
		 (rcs[idy].row>surfrrg.stop && rcs[idy+1].row>surfrrg.stop) ||
		 (rcs[idy].row<surfrrg.start && rcs[idy+1].row<surfrrg.start) ||
		 (rcs[idy].col>surfcrg.stop && rcs[idy+1].col>surfcrg.stop) ||
		 (rcs[idy].col<surfcrg.start && rcs[idy+1].col<surfcrg.start) )
		continue;

	    if ( definedp[idy] || definedp[idy+1] )
	    {
		Coord3 horpos0 = horposes[idy];
		Coord3 horpos1 = horposes[idy+1];
		if ( definedp[idy] && definedp[idy+1] )
		{
		    mContIfOnOneSide();
		}
		else 
		{
		    const float projz = definedp[idy] ? horpos0.z : horpos1.z;
    		    if ( (prevz > projz && nextz > projz) || 
       			 (prevz < projz && nextz < projz) )
    			continue;
		    
		    if ( definedp[idy] )
		    {
			horpos1 = fhi_.surf_.getKnot( rcs[idy+1], true );
			horpos1.z += fhi_.zshift_;
		    }
		    else
		    {
			horpos0 = fhi_.surf_.getKnot( rcs[idy], true );
			horpos0.z += fhi_.zshift_;
		    }

		    mContIfOnOneSide();
		}

		const double zd0 = fabs(horpos0.z - prevz); 
	    	const double zd1 = fabs(horpos1.z - nextz); 
	    	Coord3 intsect = horpos0 + (horpos1-horpos0) * zd0 / (zd0+zd1);
		mSetIntersectionInfo( intsect, idy );
	    }
	    else
	    {
		Coord3 intersectpos;
		Line3 segment( knots[idy], knots[idy+1]-knots[idy] );
		if ( plane_.intersectWith(segment,intersectpos) )
		{
		    //check if the intersection is on the surface or not.
		    const BinID bid = SI().transform( intersectpos );
		    RowCol rc( surfrrg.snap(bid.inl), surfcrg.snap(bid.crl) );
		    const Coord3 horpos = fhi_.surf_.getKnot(rc,false);
		    if ( !mIsUdf( horpos.z ) ) 
		    {
			mSetIntersectionInfo( intersectpos, idy );
		    }
		}
	    }
	}

	if ( !found )
	    (*fhi_.itsinfo_[idx]).intersectstatus = mStickAwayHor;
    }

    return true;
}

protected:

FaultBinIDSurfaceIntersector&	fhi_;
Interval<float>			bidsurfzrg_;
Plane3				plane_;
};    


FaultBinIDSurfaceIntersector::FaultBinIDSurfaceIntersector( float horshift,
	const BinIDSurface& sf, const FaultStickSet& ft, Coord3List& cl )
    : ft_( ft )
    , surf_( sf )
    , crdlist_( cl )
    , output_( 0 )
    , zshift_( horshift )
    , rrg_( surf_.rowRange() )
    , crg_( surf_.colRange() )
{}


FaultBinIDSurfaceIntersector::~FaultBinIDSurfaceIntersector()	
{ 
    deepErase( ftbids_ ); 
    deepErase( itsinfo_ );
}


void FaultBinIDSurfaceIntersector::compute()
{
    const bool geoexit = output_ && output_->getGeometry()[0];
    IndexedGeometry* geo = geoexit ?
	const_cast<IndexedGeometry*>( output_->getGeometry()[0] ) : 0;
    if ( geo ) geo->removeAll();

    //Only add stick intersections.
    FaultStickHorizonIntersector its( *this );
    its.execute();
    
    for ( int idx=0; idx<ft_.nrSticks()-1; idx++ )
    {
	if ( (*itsinfo_[idx]).lowknotidx==-1 || 
	     (*itsinfo_[idx+1]).lowknotidx==-1 )
	    continue;

	const int nid0 = crdlist_.add( (*itsinfo_[idx]).intsectpos );
	const int nid1 = crdlist_.add( (*itsinfo_[idx+1]).intsectpos );
	if ( geo )
	{
	    geo->coordindices_ += nid0;
	    geo->coordindices_ += nid1;
	}
    }

    /* Panel intersections
    for ( int idx=0; idx<ft_.nrSticks()-1; idx++ )
    {
	TypeSet<Coord3> sortedcrds;
	calPanelIntersections( idx, sortedcrds );
    
	for ( int idy=0; idy<sortedcrds.size(); idy++ )
    	{
    	    int nid1 = crdlist_.add( sortedcrds[idy] );
    	    if ( geo ) geo->coordindices_ += nid1;
    	}
    } */
    
    if ( geo )
    	geo->ischanged_ = true;
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


void FaultBinIDSurfaceIntersector::calPanelIntersections( int pnidx, 
       TypeSet<Coord3>& sortedcrds )
{
    const int szd = ft_.getStick(pnidx)->size() - ft_.getStick(pnidx+1)->size();
    const int lidx = szd > 0 ? pnidx : pnidx+1;
    const int sidx = szd > 0 ? pnidx+1 : pnidx;
    
    const TypeSet<Coord3>& knots1 = *ft_.getStick(sidx); 
    if ( ((*itsinfo_[pnidx]).intersectstatus==mKnotBelowHor &&
	  (*itsinfo_[pnidx+1]).intersectstatus==mKnotBelowHor) ||
	 ((*itsinfo_[pnidx]).intersectstatus==mKnotAboveHor && 
	  (*itsinfo_[pnidx+1]).intersectstatus==mKnotAboveHor) )
	return;

    const int sz0 = (*ft_.getStick(lidx)).size();
    const int sz1 = (*ft_.getStick(sidx)).size();
    const int iniskip = (sz0-sz1) / 2;   

    bool reverse = ( (*itsinfo_[pnidx]).intersectstatus==mKnotBelowHor &&
	    	     (*itsinfo_[pnidx+1]).intersectstatus!=mKnotBelowHor ) ||
		   ( (*itsinfo_[pnidx]).intersectstatus==mStickIntersectsHor &&
	   	     (*itsinfo_[pnidx+1]).intersectstatus==mKnotAboveHor );
    bool nmorder = ( (*itsinfo_[pnidx]).intersectstatus==mKnotAboveHor &&
		     (*itsinfo_[pnidx+1]).intersectstatus!=mKnotAboveHor ) ||
		   ( (*itsinfo_[pnidx+1]).intersectstatus!=mKnotAboveHor &&
       		     (*itsinfo_[pnidx]).intersectstatus==mStickIntersectsHor );
    
    for ( int idx=0; idx<sz0; idx++ )
    {
	const RowCol bid0( rrg_.snap((*ftbids_[lidx])[idx].inl), 
		    crg_.snap((*ftbids_[lidx])[idx].crl) );
	
	const int sknotidx = 
	    idx<iniskip ? 0 : ( idx<iniskip+sz1-1 ? idx-iniskip : sz1-1 );
	const RowCol bid1( rrg_.snap((*ftbids_[sidx])[sknotidx].inl), 
		    crg_.snap((*ftbids_[sidx])[sknotidx].crl) );
	
	Coord3 hpos0 = surf_.getKnot(bid0,false); 
	if ( mIsUdf(hpos0.z) ) continue;
	hpos0.z += zshift_;
	
	Coord3 hpos1 = surf_.getKnot(bid1,false); 
	if ( mIsUdf(hpos1.z) ) continue;
	hpos1.z += zshift_;
	
	double zd0 = hpos0.z - (*ft_.getStick(lidx))[idx].z; 
	double zd1 = hpos1.z - (*ft_.getStick(sidx))[sknotidx].z;
	if ( (zd0<0 && zd1<0) || (zd0>0 && zd1>0) )
	    continue;
	
	if ( zd0<0 ) zd0 = -zd0;    
	if ( zd1<0 ) zd1 = -zd1;

	Coord3 pos = zd0+zd1>0 ? hpos0+(hpos1-hpos0)*zd0/(zd0+zd1) : hpos0;
	if ( nmorder )
	    sortedcrds += pos;
	else if ( reverse )
	    sortedcrds.insert( 0, pos );
	
    }

    if ( (*itsinfo_[pnidx]).intersectstatus==mStickIntersectsHor )
	sortedcrds.insert( 0, (*itsinfo_[pnidx]).intsectpos );
    
    if ( (*itsinfo_[pnidx+1]).intersectstatus==mStickIntersectsHor )
	sortedcrds += (*itsinfo_[pnidx+1]).intsectpos;
}


};

