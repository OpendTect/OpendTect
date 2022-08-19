/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "linesetposinfo.h"
#include "survinfo.h"
#include "statruncalc.h"
#include "binidvalset.h"
#include "binidvalue.h"


PosInfo::LineSet2DData::IR::~IR()
{
    delete posns_;
}


const PosInfo::Line2DData* PosInfo::LineSet2DData::getLineData(
					const char* lnm ) const
{
    PosInfo::LineSet2DData::Info* li = findLine( lnm );
    return li ? &li->pos_ : 0;
}


PosInfo::LineSet2DData::Info* PosInfo::LineSet2DData::findLine(
			const char* lnm ) const
{
    if ( !lnm || !*lnm ) return 0;

    for ( int idx=0; idx<data_.size(); idx++ )
    {
	if ( data_[idx]->lnm_ == lnm )
	    return const_cast<PosInfo::LineSet2DData::Info*>(data_[idx]);
    }

    return 0;
}


PosInfo::Line2DData& PosInfo::LineSet2DData::addLine( const char* lnm )
{
    PosInfo::LineSet2DData::Info* li = findLine( lnm );
    if ( li ) return li->pos_;

    li = new Info;
    li->lnm_ = lnm;
    li->pos_.setLineName( lnm );
    data_ += li;
    return li->pos_;
}


void PosInfo::LineSet2DData::removeLine( const char* lnm )
{
    PosInfo::LineSet2DData::Info* li = findLine( lnm );
    if ( li )
    {
	data_ -= li;
	delete li;
    }
}


void PosInfo::LineSet2DData::intersect( const BinIDValueSet& bivset,
			ObjectSet<PosInfo::LineSet2DData::IR>& resultset ) const
{
    const int nrvals = bivset.nrVals();
    BinIDValueSet* globalbivset = new BinIDValueSet( nrvals, true );
    for ( int idx=0; idx<nrLines(); idx++ )
    {
	BinIDValueSet* newbivset = new BinIDValueSet( nrvals, true );
	BinID prevbid(-1,-1);
	for ( int idy=0; idy<lineData(idx).positions().size(); idy++ )
	{
	    BinID bid = SI().transform( lineData(idx).positions()[idy].coord_ );
	    if ( bid == prevbid ) continue;
	    prevbid = bid;
	    if ( bivset.includes(bid) )
	    {
		BinIDValueSet::SPos pos = bivset.find(bid);

		while ( true )
		{
		    BinIDValues bidvalues( bid, nrvals );
		    bivset.get( pos, bidvalues, bidvalues.values() );
		    if ( !globalbivset->includes(bid) )
		    {
			newbivset->add( bid, bidvalues.values() );
			globalbivset->add( bid, bidvalues.values() );
		    }
		    bivset.next( pos );
		    if ( bid != bivset.getBinID(pos) )
			break;
		}
	    }
	}

	if ( newbivset->totalSize() > 0 )
	{
	    IR* result = new IR;
	    result->lnm_ = lineName(idx);
	    result->posns_ = newbivset;
	    resultset += result;
	}
	else
	    delete newbivset;
    }
}


float PosInfo::LineSet2DData::getDistBetwTrcs( bool ismax,
					       const char* linenm ) const
{
    Stats::CalcSetup rcsetup;
    if ( ismax )
	rcsetup.require( Stats::Max );
    else
	rcsetup.require( Stats::Median );

    Stats::RunCalc<float> stats( rcsetup );
    for ( int idx=0; idx<trcdiststatsperlines_.size(); idx++ )
    {
	if ( BufferString(linenm) == trcdiststatsperlines_[idx].linename_ )
	    return ismax ? trcdiststatsperlines_[idx].maxdist_
			 : trcdiststatsperlines_[idx].mediandist_;
	else
	    stats += ismax ? trcdiststatsperlines_[idx].maxdist_
			   : trcdiststatsperlines_[idx].mediandist_;
    }

    return ismax ? stats.max() : stats.median();
}


void PosInfo::LineSet2DData::compDistBetwTrcsStats()
{
    for ( int lidx=0; lidx<nrLines(); lidx++ )
    {
	float median, max;
	lineData( lidx ).compDistBetwTrcsStats( max, median );
	LineTrcDistStats ltrcdiststats( lineName( lidx ), median, max );
	trcdiststatsperlines_ += ltrcdiststats;
    }
}


BinID PosInfo::LineSet2DData::getElementStepout( const char* linenm ) const
{
    const Line2DData* l2dd = getLineData( linenm );
    if ( l2dd )
	return BinID( 1, l2dd->trcNrRange().step );

    return BinID(1,1);
}
