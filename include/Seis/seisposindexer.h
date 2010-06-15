#ifndef seisposindexer_h
#define seisposindexer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2008
 RCS:           $Id: seisposindexer.h,v 1.7 2010-06-15 19:03:46 cvskris Exp $
________________________________________________________________________


-*/

#include "seisposkey.h"
#include "sets.h"
#include "thread.h"


template <class T> class DataInterpreter;

namespace Seis
{

mClass PosKeyList
{
public:
    virtual		~PosKeyList()			{}
    virtual od_int64	size() const			= 0;
    virtual PosKey	key(od_int64) const		= 0;
};

/*!\brief builds an index of a list of positions, making it easy to find a
  specific position.
  
  In principle, no sorting is required.
  While at it, in/xline and offset ranges are determined.

*/

mClass PosIndexer
{
public:

				PosIndexer(const PosKeyList&, bool doindex);
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

    inline Seis::GeomType	geomType() const
    				{ return Seis::geomTypeOf(is2d_,isps_); }

    inline Interval<int>	inlRange() const	{ return inlrg_; }
    inline Interval<int>	crlRange() const	{ return crlrg_; }
    inline Interval<int>	trcNrRange() const	{ return crlrg_; }
    inline Interval<float>	offsetRange() const	{ return offsrg_; }

    bool			dumpTo(std::ostream& strm) const;
    bool			readFrom(const char* nm, od_int64 offset,
	    				bool all,
	    				DataInterpreter<int>*  =0,
					DataInterpreter<od_int64>* =0,
					DataInterpreter<float>* =0 );

protected:
    bool			readHeader(DataInterpreter<int>*,
					DataInterpreter<od_int64>*,
					DataInterpreter<float>* );
    bool			readLine(TypeSet<int>& crl,
				    TypeSet<od_int64>&,
				    DataInterpreter<int>*,
				    DataInterpreter<od_int64>* ) const;

    std::istream*		strm_;
    DataInterpreter<int>*	int32interp_;
    DataInterpreter<od_int64>*	int64interp_;
    TypeSet<od_int64>		inlfileoffsets_;

    mutable Threads::Mutex	lock_;
    TypeSet<od_int64>		curidxset_;
    TypeSet<int>		curcrlset_;
    int				curinl_;

    const PosKeyList&		pkl_;
    bool			is2d_;
    bool			isps_;
    TypeSet<int>		inls_;
    ObjectSet< TypeSet<int> >	crlsets_;
    ObjectSet< TypeSet<od_int64> > idxsets_;
    od_int64			maxidx_;

    Interval<int>		inlrg_;
    Interval<int>		crlrg_;
    Interval<float>		offsrg_;

    void			empty();
    void			add(const PosKey&,od_int64);
    int				getFirstIdxs(const BinID&,int&,int&);
};



} // namespace

#endif
