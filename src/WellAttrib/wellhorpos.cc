/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jul 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: wellhorpos.cc,v 1.6 2011-01-11 10:47:33 cvsbruno Exp $";


#include "wellhorpos.h"

#include "binidvalset.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "position.h"
#include "survinfo.h"
#include "scaler.h"
#include "welld2tmodel.h"
#include "welltrack.h"


WellHorIntersectFinder::WellHorIntersectFinder( const Well::Track& tr, 
						const Well::D2TModel* d2t )
    : horid_(-1)
{
    Well::Track& track = *new Well::Track( tr );
    if ( d2t && !tr.zIsTime() ) 
	track.toTime( *d2t );
    transformWellCoordsToBinIDs( track );
    delete &track;
}


#define mDoTransInLoop(idx,bid)\
    Coord3 pos = track.pos( idx );\
    bid = SI().transform( pos );
void WellHorIntersectFinder::transformWellCoordsToBinIDs( 
						const Well::Track& track )
{
    wellpts_.erase();
    if ( track.size() <=0 ) return;
    BinID prevbid; mDoTransInLoop( 0, prevbid )
    wellpts_ += ZPoint( prevbid, pos.z );
    for ( int idx=0; idx<track.size(); idx++ )
    {
	BinID bid; mDoTransInLoop( idx, bid )
	if ( prevbid != bid )
	    wellpts_ += ZPoint( bid, pos.z );
	prevbid = bid;
    }
}


void WellHorIntersectFinder::findIntersection( TypeSet<ZPoint>& outzpts ) const
{
    outzpts.erase();
    TypeSet<ZPoint> intersectpts;
    findIntersect( wellpts_, intersectpts );
    const int sz = intersectpts.size();
    if ( sz < 2 ) 
	{ if ( sz == 1 ) outzpts += intersectpts[0]; return; }

    for ( int idx=0; idx<sz; idx +=2 )
    {
	const int wellidx1 = wellpts_.indexOf( intersectpts[idx] );
	const int wellidx2 = wellpts_.indexOf( intersectpts[idx+1] );
	if ( !wellpts_.validIdx( wellidx1 ) || !wellpts_.validIdx( wellidx2 ) )
	    return;
	const ZPoint pt1 = wellpts_[wellidx1];
	const ZPoint pt2 = wellpts_[wellidx2];
	HorSampling hs(false); 
	hs.include( pt1.bid_ ); hs.include( pt2.bid_ );
	HorSamplingIterator hsit( hs );
	TypeSet<ZPoint> newintersectzpts;

#define mGetBidDistToPrevBid(bid,dist)\
	int inldist = pt1.bid_.inl-bid.inl; inldist*=inldist;\
	int crldist = pt1.bid_.crl-bid.crl; crldist*=crldist;\
	dist = sqrt( inldist + crldist );
	float refdist; mGetBidDistToPrevBid( pt2.bid_, refdist )
	LinScaler sc( 0, pt1.zval_, refdist, pt2.zval_ );
	BinID bid;
	while ( hsit.next( bid ) )
	{
	    float biddist; mGetBidDistToPrevBid( bid, biddist )
	    float zval = sc.scale( biddist );
	    newintersectzpts += ZPoint( bid, zval );
	}
	TypeSet<ZPoint> zpts;
	findIntersect( newintersectzpts, zpts );
	if ( !zpts.isEmpty() )
	    outzpts += zpts[0];
    }
}


void WellHorIntersectFinder::findIntersect( const TypeSet<ZPoint>& inppts,
						TypeSet<ZPoint>& outpts ) const
{
    bool previsabovehor = true; float prevhorzval = 0; 
    for ( int idx=0; idx<inppts.size(); idx++ )
    {
	float wellzval = inppts[idx].zval_;
	float horzval; BinID bid = inppts[idx].bid_;
	intersectBinIDHor( bid, horzval );
	if ( mIsUdf( horzval ) ) continue;
	if ( inppts.size() == 1 )
	    outpts += ZPoint( bid, horzval );
	const bool isabovehor = wellzval < horzval;
	if ( idx && isabovehor != previsabovehor )
	{ 
	    outpts += ZPoint( inppts[idx-1].bid_, prevhorzval ); 
	    outpts += ZPoint( bid, horzval ); 
	}
	previsabovehor = isabovehor;
	prevhorzval = horzval;
    }
}


void WellHorIntersectFinder::intersectBinIDHor( const BinID& bid, 
						float& zpos ) const
{
    zpos = mUdf( float );
    EM::EMObject* emobj = EM::EMM().getObject( horid_ );
    if ( !emobj ) return;
    mDynamicCastGet(EM::Horizon2D*,hor2d,emobj)
    mDynamicCastGet(EM::Horizon3D*,hor3d,emobj)

    if ( hor3d )
    {
	const EM::SubID subid = bid.toInt64();
	const Coord3& pos = emobj->getPos( emobj->sectionID(0), subid ); 
	const BinID horbid = SI().transform( pos );
	if ( bid == horbid )
	    zpos = pos.z;
	return;
    }
    else if ( hor2d )
    {
	mDynamicCastGet( const Geometry::RowColSurface*, rcs, 
			 emobj->sectionGeometry(0));
	if ( !rcs ) return;

	const StepInterval<int> rowrg = rcs->rowRange();
	RowCol rc;
	for ( rc.row=rowrg.start; rc.row<=rowrg.stop; rc.row+=rowrg.step )
	{
	    const StepInterval<int> colrg = rcs->colRange( rc.row );
	    for ( rc.col=colrg.start; rc.col<=colrg.stop; rc.col+=colrg.step )
	    {
		const Coord3& pos = emobj->getPos( 
					emobj->sectionID(0), rc.toInt64() );
		const BinID horbid = SI().transform( pos );
		if ( bid == horbid )
		    { zpos = pos.z; return; }
	    }
	}
    }
}
