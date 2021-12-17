#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2004/Oct 2013/Mar 2016
________________________________________________________________________

-*/

#include "generalmod.h"
#include "posidxpair.h"
#include "sets.h"
#include "ranges.h"
#include "od_iosfwd.h"
namespace PosInfo { class CubeData; }
class TrcKeySampling;


namespace Pos
{

class IdxPairDataSet;
using EntryCreatedFn = void(*)(IdxPairDataSet&,int spos_i,int spos_j);

/*!\brief A sorted set of IdxPairs and associated data buffer.

  The set is sorted on both first and second key (inl()/row()/lineNr() and
  crl()/col()/traceNr()). This has a cost when creating the set, and it will
  be slower than a normal TypeSet for small sets. Further, the order in which
  you add positions will not be preserved.

  Luckily there are also advantages. It is *much* faster for searching when
  the set is large. When you have a block of N rows x N columns, the
  search time will be O( 2 log2(N) ) instead of O( N^2 / 2 ). Thus for
  1000x1000 instead of 500000 you need 20 comparisons (probably a few more but
  not a lot more).

  The iteration through the set should be done using the SPos iterator class.
  All positioning is already done with SPos, but you can in theory still use
  \code
  const od_int64 sz = totalSize();
  for ( od_int64 idx=0; idx<sz; idx++ )
  {
      Pos::IdxPairDataSet::SPos pos = set.getPos( idx );
      // etc.
  }
  \endcode
  but that will be very inefficient.

  Note that you can use null pointers to add or set 'empty' slots.
  Note also: ObjSz is od_int64. Keep it like that as it ensures good index
  arithmetic for the buffers.

 */


mExpClass(General) IdxPairDataSet
{
public:

    typedef IdxPair::IdxType		IdxType;
    typedef TypeSet<IdxType>::size_type	ArrIdxType;
    typedef od_int64			ObjSzType;
    typedef od_int64			GlobIdxType;


    /*!\brief Set Position: position in IdxPairDataSet

      Note that the iterator becomes invalid when adding or removing from
      the set.
     */
    struct SPos
    {
			SPos( ArrIdxType ii=-1, ArrIdxType jj=-1 )
			    : i(ii), j(jj)	{}
	void		reset()		{ i = j = -1; }
	inline bool	operator ==( const SPos& oth ) const
					{ return i == oth.i && j == oth.j; }
	inline bool	operator !=( const SPos& oth ) const
					{ return i != oth.i || j != oth.j; }
	inline bool	operator>(const SPos&) const;
	inline bool	operator<(const SPos&) const;
	inline bool	isValid() const	{ return i > -1 && j > -1; }

	ArrIdxType	i, j;
    };


			IdxPairDataSet(ObjSzType,bool allow_duplicate_idxs,
					bool manage_data=true);
			IdxPairDataSet(const IdxPairDataSet&);
    virtual		~IdxPairDataSet();
    IdxPairDataSet&	operator =(const IdxPairDataSet&);

    inline bool		isEmpty() const		{ return frsts_.isEmpty(); }
    void		setEmpty();
    void		copyStructureFrom(const IdxPairDataSet&);
						//!< will also empty this set

    ObjSzType		objSize() const			{ return objsz_; }
    bool		managesData() const		{ return mandata_; };
    bool		allowsDuplicateIdxPairs() const	{ return allowdup_; }
    void		allowDuplicateIdxPairs(bool);
    bool		append(const IdxPairDataSet&);
    void		remove(const IdxPairDataSet&);
    void		remove(const TrcKeySampling& hrg,bool inside);
    void		remove(SPos); //!< afterwards, input SPos may be invalid
    void		remove(const TypeSet<SPos>&);
			//!< You cannot remove while iterating, so ...
			//!< collect the to-be-removed and use this instead

    inline SPos		find( const IdxPair& ip ) const
						{ return findFirst(ip); }
    inline SPos		findFirst( const IdxPair& ip ) const
						{ return findOccurrence(ip,0); }
    SPos		findOccurrence(const IdxPair&,int occ=0) const;
    bool		next(SPos&,bool skip_duplicate_idxpairs=false) const;
    bool		prev(SPos&,bool skip_duplicate_idxpairs=false) const;
    bool		isValid(const IdxPair&) const;

    IdxPair		getIdxPair(SPos) const;
    const void*		getObj(SPos) const;
    const void*		get(SPos,IdxPair&) const;
    SPos		getPos(GlobIdxType) const;  //!< Very slow.
    SPos		add(const IdxPair&,const void* obj=0);
			    //!< If returned SPos is not valid memory was full
    void		set(SPos,const void* obj=0);
			    //!< Will *not* check whether SPos is in set
			    //!< When iterating, this is not an issue
			    //!< If unsure, check isValid(SPos)
    void		set( const IdxPair& ip, const void* obj=0 )
						{ set( findFirst(ip), obj ); }
			    //!< May do the wrong thing if you have duplicates
    SPos		update(const IdxPair&,const void* obj=0);
			    //!< May do the wrong thing if you have duplicates
			    //!< Will add if necessary
			    //!< If returned SPos is not valid memory was full

    inline ArrIdxType	nrFirst() const		{ return frsts_.size(); }
    ArrIdxType		nrSecond(IdxType firstidx) const;
    inline bool		includes( const IdxPair& ip ) const
						{ return find(ip).j > -1; }
    bool		hasFirst(IdxType) const;
    bool		hasSecond(IdxType) const;
    IdxPair		firstIdxPair() const; //!< when empty returns udf()
    GlobIdxType		totalSize() const;
    Interval<IdxType>	firstRange() const;
    Interval<IdxType>	secondRange(IdxType firsidx=-1) const;

    bool		hasDuplicateIdxPairs() const;
    ArrIdxType		nrDuplicateIdxPairs() const;
    void		removeDuplicateIdxPairs();
    ArrIdxType		nrPos(ArrIdxType lineidx) const;

    void		extend(const IdxPairDelta& stepout,const IdxPairStep&,
				EntryCreatedFn fn=0);
			    //!< Adds only IdxPair postions not yet in set
    void		add(const PosInfo::CubeData&,EntryCreatedFn fn=0);
			    //!< Adds only IdxPair postions not yet in set
    void		randomSubselect(GlobIdxType maxsz);

			// I/O
    bool		dump(od_ostream&,bool binary) const;
    bool		slurp(od_istream&,bool binary);

			// aliases
    inline ArrIdxType	nrInls() const		    { return nrFirst(); }
    inline ArrIdxType	nrCrls( IdxType inl ) const { return nrSecond(inl); }
    inline ArrIdxType	nrRows() const		    { return nrFirst(); }
    inline ArrIdxType	nrCols( IdxType row ) const { return nrSecond(row); }
    bool		hasInl( IdxType inl ) const { return hasFirst(inl); }
    bool		hasCrl( IdxType crl ) const { return hasSecond(crl); }
    inline bool		hasRow( IdxType row ) const { return hasFirst(row); }
    inline bool		hasCol( IdxType col ) const { return hasSecond(col); }
    Interval<IdxType>	inlRange() const	    { return firstRange(); }
    Interval<IdxType>	rowRange() const	    { return firstRange(); }
    Interval<IdxType>	crlRange( IdxType inl=-1 ) const
						    { return secondRange(inl); }
    Interval<IdxType>	colRange( IdxType row=-1 ) const
						    { return secondRange(row); }

			// Maybe messing with managed objects in buf.
    bool		setObjSize(ObjSzType sz,ObjSzType offs_in_objs=-1,
				    const void* initwith=0);
			// offs_in_objs < 0: operate at end
    void		decrObjSize(ObjSzType,ObjSzType offs=-1);
    bool		incrObjSize(ObjSzType,ObjSzType offs=-1,const void* =0);

protected:

    typedef TypeSet<IdxType>	IdxSet;
    typedef od_int64		BufSzType;

    class ObjData
    {
    public:

	typedef unsigned char	BufType;

				ObjData() : buf_(0), bufsz_(0)
						    { objs_.allowNull(true); }
				ObjData(const ObjData&);
				~ObjData()	    { delete [] buf_; }

	const void*		getObj(bool,ArrIdxType,ObjSzType) const;
	void			putObj(bool,ArrIdxType,ObjSzType,const void*);
	bool			addObjSpace(bool,ArrIdxType,ObjSzType);
	void			removeObj(bool,ArrIdxType,ObjSzType);
	void			decrObjSize(ObjSzType orgsz,ObjSzType newsz,
					    ObjSzType at_offs);
	bool			incrObjSize(ObjSzType,ObjSzType,ObjSzType,
					    const void*);

    private:

	ObjectSet<const void>	objs_; // contains const bool* when mandata

	BufType*	buf_;
	BufSzType	bufsz_;
	bool		manageBufCapacity(ObjSzType);

    };

    const ObjSzType	objsz_;
    const bool		mandata_;
    bool		allowdup_;

    IdxSet		frsts_;
    ObjectSet<IdxSet>	scndsets_;
    ObjectSet<ObjData>	objdatas_;

    static ArrIdxType	findIndexFor(const IdxSet&,IdxType,bool* found=0);
    const void*		gtObj(const SPos&) const;
    bool		addObj(SPos&,IdxType,const void*);
    void		putObj(const SPos&,const void*);
    void		addEntry(const Pos::IdxPair&,const void*,SPos&);

    // All 'gt' functions return unchecked
    inline IdxType	gtFrst( const SPos& pos ) const
				{ return frsts_[pos.i]; }
    inline IdxType	gtScnd( const SPos& pos ) const
				{ return gtScndSet(pos)[pos.j]; }
    inline IdxPair	gtIdxPair( const SPos& pos ) const
				{ return IdxPair( gtFrst(pos), gtScnd(pos) ); }
    inline IdxSet&	gtScndSet( const SPos& pos )
				{ return *scndsets_[pos.i]; }
    inline const IdxSet& gtScndSet( const SPos& pos ) const
				{ return *scndsets_[pos.i]; }
    inline ObjData&	gtObjData( const SPos& pos )
				{ return *objdatas_[pos.i]; }
    inline const ObjData& gtObjData( const SPos& pos ) const
				{ return *objdatas_[pos.i]; }
    inline IdxSet&	gtScndSet( ArrIdxType idx )
				{ return *scndsets_[idx]; }
    inline const IdxSet& gtScndSet( ArrIdxType idx ) const
				{ return *scndsets_[idx]; }
    inline ObjData&	gtObjData( ArrIdxType idx )
				{ return *objdatas_[idx]; }
    inline const ObjData& gtObjData( ArrIdxType idx ) const
				{ return *objdatas_[idx]; }

};


inline bool IdxPairDataSet::SPos::operator >( const SPos& oth ) const
{
    if ( i > oth.i )
	return true;
    return i == oth.i && j > oth.j;
}


inline bool IdxPairDataSet::SPos::operator <( const SPos& oth ) const
{
    if ( i < oth.i )
	return true;
    return i == oth.i && j < oth.j;
}


} // namespace Pos


