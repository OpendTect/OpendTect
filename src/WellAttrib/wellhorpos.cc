/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jul 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: wellhorpos.cc,v 1.5 2010-08-24 12:41:13 cvsbruno Exp $";


#include "wellhorpos.h"

#include "binidvalset.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "position.h"
#include "survinfo.h"
#include "welltrack.h"


WellHorPos::WellHorPos( const Well::Track& tr  )
    : horid_(-1)
    , track_(tr)  
{
    transformWellCoordsToBinIDs();
}


#define mDoTransInLoop(idx,bid)\
    Coord3 pos = track_.pos( idx );\
    bid = SI().transform( pos );
void WellHorPos::transformWellCoordsToBinIDs()
{
    wellbids_.erase();
    if ( track_.size() <=0 ) return;
    BinID prevbid; mDoTransInLoop( 0, prevbid )
    wellbids_ += prevbid;	
    for ( int idx=0; idx<track_.size(); idx++ )
    {
	BinID bid; mDoTransInLoop( idx, bid )
	if ( prevbid != bid )
	    wellbids_ += bid;
	prevbid = bid;
    }
}


void WellHorPos::intersectWellHor( BinIDValueSet& bidset ) const
{
    bidset.empty();
    for ( int idx=0; idx<wellbids_.size(); idx ++ )
    {
	float zval; BinID bid = wellbids_[idx];
	intersectBinIDHor( bid, zval );
	if ( !mIsUdf( zval ))
	    bidset.add( bid, zval );
    }
}


void WellHorPos::intersectBinIDHor( const BinID& bid, float& zpos ) const
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
    else
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
		{
		    zpos = pos.z;
		    return;
		}
	    }
	}
    }
}
