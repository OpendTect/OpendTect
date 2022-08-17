#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2008
________________________________________________________________________


-*/

#include "seismod.h"
#include "seisposkey.h"
#include "threadlock.h"
#include "ranges.h"
#include "sets.h"
#include "od_iosfwd.h"


template <class T> class DataInterpreter;

namespace Seis
{

mExpClass(Seis) PosKeyList
{
public:

    typedef od_int64	FileIdxType;

    virtual		~PosKeyList()			{}
    virtual od_int64	size() const			= 0;
    virtual bool	key(od_int64,PosKey&) const	= 0;

};

/*!\brief builds an index of a list of positions, making it easy to find a
  specific position.

  In principle, no sorting is required.
  While at it, in/xline and offset ranges are determined.
*/

mExpClass(Seis) PosIndexer
{
public:

    typedef Index_Type			KeyIdxType;
    typedef TypeSet<KeyIdxType>		KeyIdxSet;
    typedef PosKeyList::FileIdxType	FileIdxType;
    typedef TypeSet<FileIdxType>	FileIdxSet;
    typedef od_stream_Pos		FileOffsType;
    typedef TypeSet<FileOffsType>	FileOffsSet;
    typedef DataInterpreter<int>	Int32Interpreter;
    typedef DataInterpreter<od_int64>	Int64Interpreter;
    typedef DataInterpreter<float>	FloatInterpreter;


				PosIndexer(const PosKeyList&,bool doindex,
					   bool excludeunreasonable);
				/*!<
				  \param doindex
				  \param excludeunreasonable enables rejection
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
    od_int64			nrRejected() const	{ return nrrejected_; }

    bool			ioCompressed() const
				{ return iocompressedmgr_; }
    void			setIOCompressed(bool yn=true)
				{ iocompressedmgr_ = yn; }
    bool			dumpTo(od_ostream& strm) const;
    bool			readFrom(const char* nm, od_int64 offset,
					bool all,
					DataInterpreter<int>*  =0,
					DataInterpreter<od_int64>* =0,
					DataInterpreter<float>* =0 );

    const TypeSet<int>&		getInls() const { return inls_; }
    void			getCrls(int inl,TypeSet<int>&) const;

    void			add(const Seis::PosKey&, od_int64 offset );
				//!<Adds the pk to index. Called from reIndex

    mDeprecated("Use setEmpty") void		empty()		{ setEmpty(); }
    void			setEmpty();

protected:

    bool			readHeader(DataInterpreter<int>*,
					DataInterpreter<od_int64>*,
					DataInterpreter<float>* );
    bool			readLine(TypeSet<int>& crl,
				    TypeSet<od_int64>&,
				    DataInterpreter<int>*,
				    DataInterpreter<od_int64>* ) const;

    od_istream*			strm_;
    DataInterpreter<int>*	int32interp_;
    DataInterpreter<od_int64>*	int64interp_;
    TypeSet<od_int64>		inlfileoffsets_;

    mutable Threads::Lock	lock_;
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
    bool			iocompressedmgr_;

    bool			isReasonable(const BinID&) const;
    int				getFirstIdxs(const BinID&,int&,int&);
    void			dumpLineCompressed(od_ostream&,const KeyIdxSet&,
						   const FileIdxSet&) const;
    bool			readLineCompressed(KeyIdxSet&,
						   FileIdxSet&) const;

};

} // namespace Seis
