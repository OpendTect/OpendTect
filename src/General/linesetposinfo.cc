/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : July 2005 / Mar 2008
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "linesetposinfo.h"
#include "survinfo.h"
#include "binidvalset.h"


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
    BinIDValueSet* globalbivset = new BinIDValueSet( bivset.nrVals(), true );
    for ( int idx=0; idx<nrLines(); idx++ )
    {
	BinIDValueSet* newbivset = new BinIDValueSet( bivset.nrVals(), true );
	BinID prevbid(-1,-1);
	for ( int idy=0; idy<lineData(idx).positions().size(); idy++ )
	{
	    BinID bid = SI().transform( lineData(idx).positions()[idy].coord_ );
	    if ( bid == prevbid ) continue;
	    prevbid = bid;
	    if ( bivset.includes(bid) )
	    {
		BinIDValueSet::Pos pos = bivset.findFirst(bid);

		while ( true )
		{
		    BinIDValues bidvalues;
		    bivset.get(pos,bidvalues);
		    if ( !globalbivset->areBinidValuesThere( bidvalues ) )
		    {
			newbivset->add(bidvalues);
			globalbivset->add(bidvalues);
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
