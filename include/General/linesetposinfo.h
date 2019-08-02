#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		2005 / Mar 2008
________________________________________________________________________

-*/

#include "generalmod.h"
#include "posinfo2d.h"
#include "objectset.h"
class BinnedValueSet;


namespace PosInfo
{

/*!\brief Position info for a set of 2D lines */

mExpClass(General) LineSet2DData
{
public:

    mUseType( Line2DData,	dist_type );

    virtual		~LineSet2DData()	{ deepErase(data_); }

    Line2DData&		addLine(const char*);
    int			nrLines() const		{ return data_.size(); }
    const char*		lineName( int idx ) const
			{ return data_[idx]->lnm_.buf(); }
    const Line2DData&	lineData( int idx ) const
			{ return data_[idx]->pos_; }
    const Line2DData*	getLineData(const char*) const;
    void		removeLine(const char*);

    struct IR
    {
			IR() : posns_(0)	{}
			~IR(); //		{ delete posns_; }

	BufferString	lnm_;
	BinnedValueSet*	posns_;
    };
    void		intersect(const BinnedValueSet&,ObjectSet<IR>&) const;

    dist_type		getDistBetwTrcs(bool,const char* linenm =0) const;
    void                compDistBetwTrcsStats();
    bool		areStatsComputed() const
			{ return trcdiststatsperlines_.size(); }

protected:

    struct Info
    {
	BufferString	lnm_;
	Line2DData	pos_;
    };

    ObjectSet<Info>	data_;

    Info*		findLine(const char*) const;

    mStruct(General) LineTrcDistStats
    {
				LineTrcDistStats( BufferString linename,
						  dist_type mediandist,
						  dist_type maxdist )
				    : linename_(linename)
				    , mediandist_(mediandist)
				    , maxdist_( maxdist )               {};

	bool                    operator ==( LineTrcDistStats ltds ) const
				{
				    return ltds.linename_ == linename_
					&& ltds.mediandist_ == mediandist_
					&& ltds.maxdist_ == maxdist_;
				}

	BufferString		linename_;
	dist_type		mediandist_;
	dist_type		maxdist_;
    };

    TypeSet<LineTrcDistStats>   trcdiststatsperlines_;

};

} // namespace PosInfo
