#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odbindmod.h"

#include "binid.h"
#include "ranges.h"
#include <vector>

namespace PosInfo
{

class CubeData;

/*!
\brief CubeData index for fast trace number <-> BinID lookups.
*/

mExpClass(ODBind) CubeDataIndex
{
public:
				CubeDataIndex(const PosInfo::CubeData&);
				~CubeDataIndex();

    od_int64			trcNumber(const BinID&) const;
    BinID			binID(od_int64) const;
    od_int64			lastTrc() const;
    bool			isValid(od_int64) const;
    bool			isValid(const BinID&) const;

    struct Segment
    {
				Segment( od_int64 startTrc, int inl,
					 const StepInterval<int>& seg );

	StepInterval<od_int64>	trcnumber_;
	int			inline_;
	StepInterval<int>	crlseg_;
    };

    struct InlIndex
    {
				InlIndex( int inl, int idx )
				    : inline_(inl), index_(idx) {}

	int			inline_;
	int			index_;
	bool operator<(const InlIndex& i) const { return inline_ < i.inline_; }
    };

protected:
    std::vector<Segment>		cdidx_;
    std::vector<InlIndex>		inlidx_;

    void				buildIndex(const PosInfo::CubeData&);

};

} // namespace PosInfo
