/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y. Liu
 * DATE     : March 2010
-*/

static const char* rcsID = "$Id: faulthorintersect.cc,v 1.4 2011-01-14 21:00:14 cvsyuancheng Exp $";

#include "faulthorintersect.h"

#include "binidsurface.h"
#include "faultstickset.h"
#include "indexedshape.h"
#include "positionlist.h"
#include "ranges.h"
#include "survinfo.h"

#define mKnotBelowHor		-1
#define mKnotAboveHor		2
#define mStickIntersectsHor	0

namespace Geometry
{

class FaultStickHorizonIntersector : public ParallelTask
{
public:    
FaultStickHorizonIntersector( FaultBinIDSurfaceIntersector& fhi )
    : fhi_( fhi )
{}

od_int64 nrIterations() const	{ return fhi_.ft_.nrSticks(); }
const char* message() const	{ return "Calculate stick intersections"; }


bool doPrepare( int )
{
    for ( int idx=0; idx<nrIterations(); idx++ )
    {
	fhi_.ftbids_ += new TypeSet<BinID>();
	fhi_.zprojs_ += new TypeSet<double>();
	
	FaultBinIDSurfaceIntersector::StickIntersectionInfo* stickinfo = 
	    new FaultBinIDSurfaceIntersector::StickIntersectionInfo();
	stickinfo->lowknotidx = -1;
	fhi_.itsinfo_ += stickinfo;
    }

    return true;
}


bool doWork( od_int64 start, od_int64 stop, int )
{
    for ( int idx=start; idx<=stop; idx++ )
    {
	const TypeSet<Coord3>& knots = *fhi_.ft_.getStick(idx);
	const int ktsz = knots.size();
	if ( !ktsz ) continue;
	
	for ( int idy=0; idy<ktsz; idy++ )
	{
	    const BinID bid = SI().transform( knots[idy] );
	    (*fhi_.ftbids_[idx]) += bid;

	    const RowCol hbid(fhi_.rrg_.snap(bid.inl),fhi_.crg_.snap(bid.crl));
	    Coord3 horpos = fhi_.surf_.getKnot(hbid,true);
	    if ( mIsUdf(horpos.z) )
		horpos = fhi_.surf_.computePosition( horpos );

	    (*fhi_.zprojs_[idx]) += horpos.z + fhi_.zshift_;
	}

	const bool prevbelow = knots[0].z < (*fhi_.zprojs_[idx])[0];
	(*fhi_.itsinfo_[idx]).intersectstatus = 
	    prevbelow ? mKnotBelowHor : mKnotAboveHor;
	
	for ( int idy=0; idy<knots.size()-1; idy++ )
	{
	    const bool nextbelow = knots[idy+1].z < (*fhi_.zprojs_[idx])[idy+1];
	    if ( (prevbelow && nextbelow) || (!prevbelow && !nextbelow) )
		continue;
    
	    const RowCol bid0( fhi_.rrg_.snap((*fhi_.ftbids_[idx])[idy].inl), 
		        fhi_.crg_.snap((*fhi_.ftbids_[idx])[idy].crl) );
	    const RowCol bid1( fhi_.rrg_.snap((*fhi_.ftbids_[idx])[idy+1].inl), 
    			fhi_.crg_.snap((*fhi_.ftbids_[idx])[idy+1].crl) );
	    Coord3 horpos0 = fhi_.surf_.getKnot(bid0,true); 
	    if ( mIsUdf(horpos0.z) )
		horpos0 = fhi_.surf_.computePosition( horpos0 );
	    if ( mIsUdf(horpos0.z) )
		continue;
	    
	    Coord3 horpos1 = fhi_.surf_.getKnot(bid1,true); 
	    if ( mIsUdf(horpos1.z) )
		horpos1 = fhi_.surf_.computePosition( horpos1 );
	    if ( mIsUdf(horpos1.z) )
		continue;
	    
	    horpos0.z += fhi_.zshift_;
	    horpos1.z += fhi_.zshift_;
	    
	    double zd0 = horpos0.z - knots[idy].z; if ( zd0<0 ) zd0 = -zd0;    
	    double zd1 = horpos1.z - knots[idy+1].z; if ( zd1<0 ) zd1 = -zd1;
	 
	    (*fhi_.itsinfo_[idx]).intsectpos = zd0+zd1 > 0 ? 
		horpos0 + (horpos1-horpos0) * zd0 / (zd0+zd1) : horpos0;
	    (*fhi_.itsinfo_[idx]).lowknotidx = idy;
	    (*fhi_.itsinfo_[idx]).intersectstatus = mStickIntersectsHor;
	    break;
	}
    }

    return true;
}

protected:

FaultBinIDSurfaceIntersector&	fhi_;

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
    deepErase( zprojs_ );
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

