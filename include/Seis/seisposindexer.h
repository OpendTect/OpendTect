#ifndef seisposindexer_h
#define seisposindexer_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Nov 2008
 RCS:           $Id: seisposindexer.h,v 1.2 2008-11-26 12:50:46 cvsbert Exp $
________________________________________________________________________


-*/

#include "seisposkey.h"
#include "sets.h"

namespace Seis
{

class PosKeyList
{
public:

    virtual od_int64	size() const			= 0;
    virtual PosKey	key(od_int64) const		= 0;

};

/*!\brief builds an index of a list of positions, making it easy to find a
  specific position.

 Sorting in in-line direction is assumed.

*/

class PosIndexer
{
public:

				PosIndexer(const PosKeyList&);
    virtual			~PosIndexer();

    od_int64			findFirst(const BinID&) const;
    				//!< -1 = inl not found
   				//!< -2 crl/trcnr not found
    od_int64			findFirst(int) const;
    				//!< -1 = empty
   				//!< -2 trcnr not found
    od_int64			findFirst(const PosKey&,
					  bool chckoffs=true) const;
    				//!< -1 = inl not found or empty
   				//!< -2 crl/trcnr not found
   				//!< -3 offs not found

    inline bool			validIdx( od_int64 idx ) const
				{ return idx >= 0 && idx < maxidx_; }
    inline od_int64		maxIdx() const		{ return maxidx_; }

    void			reIndex();

protected:

    const PosKeyList&		pkl_;
    bool			is2d_;
    bool			isps_;
    TypeSet<int>		inls_;
    ObjectSet< TypeSet<int> >	crlsets_;
    ObjectSet< TypeSet<od_int64> > idxsets_;
    od_int64			maxidx_;

    void			empty();
    void			add(const PosKey&,od_int64);
    int				getFirstIdxs(const BinID&,int&,int&) const;
};



} // namespace

#endif
