/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jul 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: wellhorpos.cc,v 1.2 2010-07-15 14:33:54 cvsbruno Exp $";


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
	const BinID bid = BinID( wellbids_[idwellbid] );
	bidset.add( bid );
	setBidsFromHorType( bidset );
    }
    intersectBinIDsHor( bidset );
}


void WellHorPos::setBidsFromHorType( BinIDValueSet& bidset ) const
{
    EM::EMObject* emobj = EM::EMM().getObject( horid_ );
    if ( !emobj ) return;
    mDynamicCastGet(EM::Horizon2D*,hor2d,emobj)
    if ( hor2d )
    {
	for ( int idline=0; idline<hor2d->geometry().nrLines(); idline++ )
	{
	    BinIDValueSet::Pos pos = bidset.getPos( 0 );
	    float zval; BinID bid; bidset.get( pos, bid, zval );
	    const int lineID = hor2d->geometry().lineID(idline);
	    if ( lineID < 0 ) continue;
	    bidset.add( BinID( lineID, bid.crl ) );
	}
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
    if ( !hor ) return;
    RowCol rc = BinID( bid );
    const EM::PosID posid(hor->id(),hor->sectionID(0),rc.toInt64());
    pos = hor->getPos( posid ).z;
}
