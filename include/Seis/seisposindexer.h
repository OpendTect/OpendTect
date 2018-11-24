#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2008
________________________________________________________________________


-*/

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
    virtual FileIdxType	size() const			= 0;
    virtual bool	key(FileIdxType,PosKey&) const	= 0;

};

/*!\brief builds an index of a list of positions, making it easy to find a
  specific position.

  In principle, no sorting is required.
  While at it, in/xline and offset ranges are determined.
*/

mExpClass(Seis) PosIndexer
{
public:

    typedef Index_Type			pos_type;
    typedef TypeSet<pos_type>		PosSet;
    typedef PosSet::size_type		size_type;
    typedef PosSet::idx_type		idx_type;
    typedef PosKeyList::FileIdxType	FileIdxType;
    typedef TypeSet<FileIdxType>	FileIdxSet;
    typedef od_stream_Pos		FileOffsType;
    typedef TypeSet<FileOffsType>	FileOffsSet;
    typedef DataInterpreter<int>	Int32Interpreter;
    typedef DataInterpreter<od_int64>	Int64Interpreter;
    typedef DataInterpreter<float>	FloatInterpreter;


				PosIndexer(const PosKeyList&,bool doindex,
					   bool exclude_unreasonable_traces);
					//!< unreasonable == far outside survey
    virtual			~PosIndexer();
    void			setEmpty();

    bool			ioCompressed() const	{ return iocompressed_;}
    void			setIOCompressed(bool yn=true)
							{ iocompressed_ = yn; }

    void			add(const PosKey&,FileIdxType);
    void			reIndex(); //!<Calls add() multiple times

    inline GeomType		geomType() const
				{ return geomTypeOf(is2d_,isps_); }
    const Interval<pos_type>&	inlRange() const	{ return inlrg_; }
    const PosSet&		getInls() const		{ return inls_; }
    const Interval<pos_type>&	crlRange() const	{ return crlrg_; }
    const Interval<pos_type>&	trcNrRange() const	{ return crlrg_; }
    void			getCrls(pos_type,PosSet&) const;
    const Interval<float>&	offsetRange() const	{ return offsrg_; }

    FileIdxType			nrRejected() const	{ return nrrejected_; }
    inline bool			validFileIdx( FileIdxType idx ) const
				{ return idx >= 0 && idx < maxfileidx_; }
    inline FileIdxType		maxFileIdx() const	{ return maxfileidx_; }

				    // -1 = inl not found (or empty in poskey)
				    // -2 crl/trcnr not found
    FileIdxType			findFirst(const BinID&) const;
    FileIdxType			findFirst(pos_type trcnr) const;
    FileIdxType			findFirst(const PosKey&,
					  bool chckoffs=true) const;
				    //!< -3 offs not found
    FileIdxType			findOcc(const PosKey&,int occ) const;
				    //!< ignores offset
    FileIdxSet			findAll(const PosKey&) const;
				    //!< ignores offset

    bool			dumpTo(od_ostream&) const;
    bool			readFrom(const char* fnm,FileIdxType,bool all,
				     Int32Interpreter* =0,Int64Interpreter* =0,
				     FloatInterpreter* =0);

protected:

    bool			readHeader(Int32Interpreter*,Int64Interpreter*,
				    FloatInterpreter*);
    bool			readLine(PosSet& crls,FileIdxSet&,
				    Int32Interpreter*,Int64Interpreter*) const;

    od_istream*			strm_;
    Int32Interpreter*		int32interp_;
    Int64Interpreter*		int64interp_;
    FileOffsSet			inlfileoffsets_;

    mutable Threads::Lock	lock_;
    mutable FileIdxSet		curfileidxs_;
    mutable PosSet		curcrlset_;
    mutable pos_type		curinl_;

    const PosKeyList&		pkl_;
    const bool			is2d_;
    const bool			isps_;
    bool			iocompressed_;
    bool			excludeunreasonable_;
    PosSet			inls_;
    ObjectSet<PosSet>		crlsets_;
    ObjectSet<FileIdxSet>	fileidxsets_;
    FileIdxType			maxfileidx_;

    Interval<pos_type>		inlrg_;
    Interval<pos_type>		crlrg_;
    Interval<float>		offsrg_;

    Interval<pos_type>		goodinlrg_;
    Interval<pos_type>		goodcrlrg_;
    FileIdxType			nrrejected_;

    bool			isReasonable(const BinID&) const;
    int				getFirstIdxs(const BinID&,int&,int&) const;
    void			dumpLineCompressed(od_ostream&,const PosSet&,
						   const FileIdxSet&) const;
    bool			readLineCompressed(PosSet&,
						   FileIdxSet&) const;

};


} // namespace Seis
