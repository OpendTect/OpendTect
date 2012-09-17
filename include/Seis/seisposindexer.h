#ifndef seisposindexer_h
#define seisposindexer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2008
 RCS:           $Id: seisposindexer.h,v 1.11 2011/03/30 11:47:16 cvsbert Exp $
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

				PosIndexer(const PosKeyList&,bool doindex,
					   bool excludeunreasonable);
				/*!<\param excludeunreasonable enables rejection
					   of traces far outside survey. */
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
    od_int64			findOcc(const PosKey&,int occ) const;
    				//!< ignores offset
    TypeSet<od_int64>		findAll(const PosKey&) const;
    				//!< ignores offset

    inline bool			validIdx( od_int64 idx ) const
				{ return idx >= 0 && idx < maxidx_; }
    inline od_int64		maxIdx() const		{ return maxidx_; }

    void			reIndex();

    inline Seis::GeomType	geomType() const
    				{ return Seis::geomTypeOf(is2d_,isps_); }

    const Interval<int>&	inlRange() const	{ return inlrg_; }
    const Interval<int>&	crlRange() const	{ return crlrg_; }
    const Interval<int>&	trcNrRange() const	{ return crlrg_; }
    const Interval<float>&	offsetRange() const	{ return offsrg_; }
    const od_int64		nrRejected() const	{ return nrrejected_; }

    bool			dumpTo(std::ostream& strm) const;
    bool			readFrom(const char* nm, od_int64 offset,
	    				bool all,
	    				DataInterpreter<int>*  =0,
					DataInterpreter<od_int64>* =0,
					DataInterpreter<float>* =0 );

    const TypeSet<int>&		getInls() const { return inls_; }
    void			getCrls(int inl,TypeSet<int>&) const;

    void			add(const Seis::PosKey&, od_int64 offset );
    				//!<Adds the pk to index. Called from reIndex
    void			empty();
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
    bool			excludeunreasonable_;
    TypeSet<int>		inls_;
    ObjectSet< TypeSet<int> >	crlsets_;
    ObjectSet< TypeSet<od_int64> > idxsets_;
    od_int64			maxidx_;

    Interval<int>		inlrg_;
    Interval<int>		crlrg_;
    Interval<float>		offsrg_;

    Interval<int>		goodinlrg_;
    Interval<int>		goodcrlrg_;
    od_int64			nrrejected_;

    bool			isReasonable(const BinID&) const;
    int				getFirstIdxs(const BinID&,int&,int&);
};



} // namespace

#endif
