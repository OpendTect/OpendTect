/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2005
-*/

static const char* rcsID = "$Id: posvecdataset.cc,v 1.1 2005-01-17 16:35:02 bert Exp $";

#include "posvecdataset.h"


PosVecDataSet::ColumnDef::MatchLevel PosVecDataSet::ColumnDef::compare(
		const PosVecDataSet::ColumnDef& cd, bool usenm ) const
{
    const BufferString& mystr = usenm ? name_ : ref_;
    const BufferString& cdstr = usenm ? cd.name_ : cd.ref_;
    if ( mystr == cdstr )
	return PosVecDataSet::ColumnDef::Exact;

    if ( matchString(mystr.buf(),cdstr.buf())
      || matchString(cdstr.buf(),mystr.buf()) )
	return PosVecDataSet::ColumnDef::Start;

    return PosVecDataSet::ColumnDef::None;
}


PosVecDataSet::PosVecDataSet()
    	: data_(1,true)
{
    empty();
}


void PosVecDataSet::empty()
{
    deepErase(coldefs_);
    coldefs_ += new ColumnDef( "Z" );
    data_.setNrVals( 1 );
}


void PosVecDataSet::add( ColumnDef* cd )
{
    coldefs_ += cd;
    data_.setNrVals( data_.nrVals() + 1 );
}


void PosVecDataSet::removeColumn( int colidx )
{
    if ( colidx > 0 && colidx < coldefs_.size() )
    {
	ColumnDef* cd = coldefs_[colidx];
	coldefs_.remove( colidx );
	delete cd;
	data_.removeVal( colidx );
    }
}


void PosVecDataSet::mergeColDefs( const PosVecDataSet& vds, ColMatchPol cmpol,
				 int* colidxs )
{
    const bool use_name = cmpol == NameExact || cmpol == NameStart;
    const bool match_start = cmpol == NameStart || cmpol == RefStart;
    const int orgcdsz = coldefs_.size();
    colidxs[0] = 0;
    for ( int idxvds=1; idxvds<vds.coldefs_.size(); idxvds++ )
    {
	const ColumnDef& cdvds = *vds.coldefs_[idxvds];
	int matchidx = -1;
	for ( int idx=1; idx<orgcdsz; idx++ )
	{
	    ColumnDef::MatchLevel ml = cdvds.compare(*coldefs_[idx],use_name);
	    if ( ml == ColumnDef::Exact
	      || (ml == ColumnDef::Start && match_start) )
		{ matchidx = idx; break; }
	}
	if ( matchidx >= 0 )
	    colidxs[idxvds] = matchidx;
	else
	{
	    add( new ColumnDef(cdvds) );
	    colidxs[idxvds] = coldefs_.size() - 1;
	}
    }
}


void PosVecDataSet::merge( const PosVecDataSet& vds, OvwPolicy ovwpol,
			   ColMatchPol cmpol )
{
    int colidxs[vds.coldefs_.size()];
    const int orgnrcds = coldefs_.size();
    mergeColDefs( vds, cmpol, colidxs );

    if ( vds.data_.isEmpty() )
	return;

    BinIDValueSet::Pos vdspos;
    const int vdsnrvals = vds.data_.nrVals();
    BinID bid; float* vals;
    while ( vds.data_.next(vdspos) )
    {
	const float* vdsvals = vds.data_.getVals(vdspos);
	vds.data_.get( vdspos, bid );
	BinIDValueSet::Pos pos = data_.findFirst( bid );
	vals = 0;
	while ( pos.valid() )
	{
	    vals = data_.getVals( pos );
	    const float z = *vals;
	    if ( mIsUndefined(z) || mIsEqual(*vdsvals,z,1e-6) )
		break;
	}
	if ( !pos.valid() )
	    vals = 0;

	const bool newpos = !vals;
	if ( newpos )
	{
	    pos = data_.add( bid );
	    vals = data_.getVals( pos );
	}

	for ( int idx=0; idx<vdsnrvals; idx++ )
	{
	    int targidx = colidxs[ idx ];
	    if ( newpos || targidx >= orgnrcds // new column
	      || ovwpol == Ovw
	      || (ovwpol == OvwIfUdf && mIsUndefined(vals[targidx])) )
		vals[targidx] = vdsvals[idx];
	}
    }
}
