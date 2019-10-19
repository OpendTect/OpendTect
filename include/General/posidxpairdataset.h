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
namespace PosInfo { class LineCollData; }
class TrcKeySampling;


namespace Pos
{

class IdxPairDataSet;
typedef void (*EntryCreatedFn)(IdxPairDataSet&,int spos_i,int spos_j);

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

    mUseType( IdxPair,	pos_type );
    typedef int		idx_type;
    typedef idx_type	size_type;
    typedef od_int64	obj_size_type;
    typedef od_int64	glob_idx_type;


    /*!\brief Set Position: position in IdxPairDataSet

      Note that the iterator becomes invalid when adding to or removing from
      the set.
     */
    typedef IJPos	SPos;

			IdxPairDataSet(obj_size_type,bool allow_duplicate_idxs,
					bool manage_data=true);
			IdxPairDataSet(const IdxPairDataSet&);
    virtual		~IdxPairDataSet();
    IdxPairDataSet&	operator =(const IdxPairDataSet&);

    inline bool		isEmpty() const		{ return frsts_.isEmpty(); }
    void		setEmpty();
    void		copyStructureFrom(const IdxPairDataSet&);
						//!< will also empty this set

    obj_size_type	objSize() const			{ return objsz_; }
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
    SPos		findNearest(const IdxPair&) const;
    SPos		findNearestOnFirst(pos_type frst,pos_type scnd) const;
    bool		next(SPos&,bool skip_duplicate_idxpairs=false) const;
    bool		prev(SPos&,bool skip_duplicate_idxpairs=false) const;
    bool		isValid(SPos) const;
    bool		isValid(const IdxPair&) const;

    IdxPair		getIdxPair(SPos) const;
    const void*		getObj(SPos) const;
    const void*		get(SPos,IdxPair&) const;
    SPos		getPos(glob_idx_type) const;  //!< Very slow.
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

    glob_idx_type	totalSize() const;
    inline bool		includes( const IdxPair& ip ) const
						{ return find(ip).j() > -1; }
    inline size_type	nrFirst() const		{ return frsts_.size(); }
    size_type		nrSecond(pos_type firstpos) const;
    size_type		nrSecondAtIdx(idx_type firstidx) const;
    bool		hasFirst(pos_type) const;
    bool		hasSecond(pos_type) const;
    Interval<pos_type>	firstRange() const;
    Interval<pos_type>	secondRange(pos_type firstpos=-1) const;

    idx_type		firstIdx(pos_type) const;
    idx_type		secondIdx(idx_type firstidx,pos_type) const;
    pos_type		firstAtIdx(idx_type firstidx) const;
    IdxPair		positionAtIdxs(idx_type,idx_type) const;
    void		getSecondsAtIndex(idx_type,TypeSet<pos_type>&) const;

    IdxPair		firstIdxPair() const; //!< when empty returns udf()
    bool		hasDuplicateIdxPairs() const;
    size_type		nrDuplicateIdxPairs() const;
    void		removeDuplicateIdxPairs();
    idx_type		nrPos(idx_type lineidx) const;

    void		extendHor3D(const IdxPairDelta& stepout,
				  const IdxPairStep&,EntryCreatedFn fn=0);
    void		extendHor2D(pos_type stepout,EntryCreatedFn fn=0);
    void		add(const PosInfo::LineCollData&,EntryCreatedFn fn=0);
			    //!< Adds only IdxPair positions not yet in set
    void		randomSubselect(glob_idx_type maxsz);

			// I/O
    bool		dump(od_ostream&,bool binary) const;
    bool		slurp(od_istream&,bool binary);

			// aliases
    inline size_type	nrInls() const		    { return nrFirst(); }
    inline size_type	nrCrls( pos_type inl ) const { return nrSecond(inl); }
    inline size_type	nrRows() const		    { return nrFirst(); }
    inline size_type	nrCols( pos_type row ) const { return nrSecond(row); }
    bool		hasInl( pos_type inl ) const { return hasFirst(inl); }
    bool		hasCrl( pos_type crl ) const { return hasSecond(crl); }
    inline bool		hasRow( pos_type row ) const { return hasFirst(row); }
    inline bool		hasCol( pos_type col ) const { return hasSecond(col); }
    Interval<pos_type>	inlRange() const	    { return firstRange(); }
    Interval<pos_type>	rowRange() const	    { return firstRange(); }
    Interval<pos_type>	crlRange( pos_type inl=-1 ) const
						    { return secondRange(inl); }
    Interval<pos_type>	colRange( pos_type row=-1 ) const
						    { return secondRange(row); }

			// Maybe messing with managed objects in buf.
    bool		setObjSize(obj_size_type sz,
				   obj_size_type offs_in_objs=-1,
				   const void* initwith=0);
			// offs_in_objs < 0: operate at end
    void		decrObjSize(obj_size_type,obj_size_type offs);
    bool		incrObjSize(obj_size_type,obj_size_type offs,
				    const void* =0);

protected:

    typedef TypeSet<pos_type>	IdxSet;
    typedef od_int64		buf_size_type;

    class ObjData
    {
    public:

	typedef unsigned char	BufType;

				ObjData() : buf_(0), bufsz_(0)
						{ objs_.setNullAllowed(true); }
				ObjData(const ObjData&);
				~ObjData()	{ delete [] buf_; }

	const void*		getObj(bool,idx_type,obj_size_type) const;
	void			putObj(bool,idx_type,obj_size_type,
				       const void*);
	bool			addObjSpace(bool,idx_type,obj_size_type);
	void			removeObj(bool,idx_type,obj_size_type);
	void			decrObjSize(obj_size_type orgsz,
					    obj_size_type newsz,
					    obj_size_type at_offs);
	bool			incrObjSize(obj_size_type,obj_size_type,
					    obj_size_type,const void*);

    private:

	ObjectSet<const void>	objs_; // contains const bool* when mandata

	BufType*	buf_;
	buf_size_type	bufsz_;
	bool		manageBufCapacity(obj_size_type);

    };

    const obj_size_type	objsz_;
    const bool		mandata_;
    bool		allowdup_;

    IdxSet		frsts_;
    ObjectSet<IdxSet>	scndsets_;
    ObjectSet<ObjData>	objdatas_;

    static idx_type	findIndexFor(const IdxSet&,pos_type,bool* found=0);
    const void*		gtObj(const SPos&) const;
    bool		addObj(SPos&,pos_type,const void*);
    void		putObj(const SPos&,const void*);
    void		addEntry(const Pos::IdxPair&,const void*,SPos&);
    void		updNearest(const IdxPair&,const SPos&,
				   od_int64&,SPos&) const;
    void		addHorPosIfNeeded(const IdxPair&,EntryCreatedFn);

    // All 'gt' functions return unchecked
    inline pos_type	gtFrst( const SPos& pos ) const
				{ return frsts_[pos.i()]; }
    inline pos_type	gtScnd( const SPos& pos ) const
				{ return gtScndSet(pos)[pos.j()]; }
    inline IdxPair	gtIdxPair( const SPos& pos ) const
				{ return IdxPair( gtFrst(pos), gtScnd(pos) ); }
    inline IdxSet&	gtScndSet( const SPos& pos )
				{ return *scndsets_[pos.i()]; }
    inline const IdxSet& gtScndSet( const SPos& pos ) const
				{ return *scndsets_[pos.i()]; }
    inline ObjData&	gtObjData( const SPos& pos )
				{ return *objdatas_[pos.i()]; }
    inline const ObjData& gtObjData( const SPos& pos ) const
				{ return *objdatas_[pos.i()]; }
    inline IdxSet&	gtScndSet( idx_type idx )
				{ return *scndsets_[idx]; }
    inline const IdxSet& gtScndSet( idx_type idx ) const
				{ return *scndsets_[idx]; }
    inline ObjData&	gtObjData( idx_type idx )
				{ return *objdatas_[idx]; }
    inline const ObjData& gtObjData( idx_type idx ) const
				{ return *objdatas_[idx]; }

};



} // namespace Pos
