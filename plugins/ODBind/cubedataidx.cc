/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "cubedataidx.h"
#include "math2.h"
#include "posinfo.h"

#include <algorithm>


PosInfo::CubeDataIndex::CubeDataIndex( const PosInfo::CubeData& cd )
{
    buildIndex( cd );
}


PosInfo::CubeDataIndex::~CubeDataIndex()
{}


bool PosInfo::CubeDataIndex::isValid( od_int64 trcnum ) const
{
    return trcnum>=0 && trcnum<=lastTrc();
}


bool PosInfo::CubeDataIndex::isValid( const BinID& bid ) const
{
    return !bid.isUdf() && trcNumber( bid )!=-1;
}


BinID PosInfo::CubeDataIndex::binID( od_int64 trcnum ) const
{
    BinID bid;
    if ( !isValid(trcnum) )
	return BinID::udf();

    const auto it = std::lower_bound( cdidx_.begin(), cdidx_.end(), trcnum,
				[]( const Segment& seg, od_int64 val )
				{
				    return seg.trcnumber_.stop < val;
				} );
    if ( it==cdidx_.end() )
	return BinID::udf();

    bid.inl() = it->inline_;
    const int idx = it->trcnumber_.getIndex( trcnum );
    bid.crl() = it->crlseg_.atIndex( idx );
    return bid;
}


od_int64 PosInfo::CubeDataIndex::trcNumber( const BinID& bid ) const
{
    od_int64 trcnum = -1;

    const InlIndex ndx {bid.inl(), 0};
    const auto inlit = std::equal_range( inlidx_.begin(), inlidx_.end(), ndx );
    if ( inlit.first==inlidx_.end() || inlit.first==inlit.second )
	return -1;

    auto start = cdidx_.begin() + inlit.first->index_;
    auto stop = cdidx_.begin() + inlit.second->index_;
    if ( inlit.second==inlidx_.end() )
	stop =	cdidx_.end();

    const auto it = std::lower_bound( start, stop, bid.crl(),
				     []( const Segment& seg, int val )
				     {
					 return seg.crlseg_.step>=0 ?
						     seg.crlseg_.stop<val :
						     seg.crlseg_.start<val;
				    } );
    if ( it!=stop )
    {
	const int idx = it->crlseg_.getIndex( bid.crl() );
	trcnum = it->trcnumber_.atIndex( idx );
    }

    return trcnum;
}


od_int64 PosInfo::CubeDataIndex::lastTrc() const
{
    return cdidx_.back().trcnumber_.stop;
}

void PosInfo::CubeDataIndex::buildIndex( const PosInfo::CubeData& cd )
{
    od_int64 next_start_trc = 0;
    for ( int idx=0; idx<cd.size(); idx++ )
    {
	for ( auto& seg : cd[idx]->segments_ )
	{
	    cdidx_.emplace_back( next_start_trc, cd[idx]->linenr_, seg );
	    const int ndx = cdidx_.size()-1;
	    inlidx_.emplace_back( cd[idx]->linenr_, ndx );
	    next_start_trc = cdidx_.back().trcnumber_.stop +
						cdidx_.back().trcnumber_.step;
	}
    }
    std::sort( inlidx_.begin(), inlidx_.end() );
}


PosInfo::CubeDataIndex::Segment::Segment( od_int64 startTrc, int inl,
					  const StepInterval<int>& crlseg )
    : inline_(inl)
    , crlseg_(crlseg)
{
    od_int64 start = startTrc;
    od_int64 step = 1;
    od_int64 stop = start + step * crlseg.nrSteps();
    trcnumber_ = StepInterval<od_int64>( start, stop, step );
}
