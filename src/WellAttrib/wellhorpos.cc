/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jul 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: wellhorpos.cc,v 1.3 2010-08-13 12:31:12 cvsbruno Exp $";


#include "wellhorpos.h"

#include "binidvalset.h"
#include "emhorizon.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "position.h"
#include "survinfo.h"
#include "welltrack.h"


WellHorPos::WellHorPos( const Well::Track& tr, const EM::ObjectID& mid )
    : horid_(mid)
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
    for ( int idwellbid=0; idwellbid<wellbids_.size(); idwellbid ++ )
    {
	BinID bid = BinID( wellbids_[idwellbid] );
	setBidsFromHorType( bid );
	bidset.add( bid );
    }
    intersectBinIDsHor( bidset );
}


void WellHorPos::setBidsFromHorType( BinID& bid ) const
{
    EM::EMObject* emobj = EM::EMM().getObject( horid_ );
    if ( !emobj ) return;
    mDynamicCastGet(EM::Horizon2D*,hor2d,emobj)
    if ( hor2d )
    {
	for ( int idline=0; idline<hor2d->geometry().nrLines(); idline++ )
	    bid.inl = idline;
    }
}


void WellHorPos::intersectBinIDsHor( BinIDValueSet& bidset ) const
{
    for ( int idx=0; idx<bidset.nrVals(); idx ++ )
    {
	BinIDValueSet::Pos pos = bidset.getPos( idx );
	float zval; BinID bid;
	bidset.get( pos, bid, zval );
	intersectBinIDHor( bid, zval );
	if ( mIsUdf( zval ))
	    bidset.remove( pos );
	else 
	    bidset.set( pos, zval );
    }
}


void WellHorPos::intersectBinIDHor( const BinID& bid, float& pos ) const
{
    EM::EMObject* emobj = EM::EMM().getObject( horid_ );
    if ( !emobj ) return;
    mDynamicCastGet(EM::Horizon*,hor,emobj)
    pos = hor ? hor->getPos( hor->sectionID(0), bid.getSerialized() ).z 
	      : mUdf( float );
}
