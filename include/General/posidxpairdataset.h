#ifndef posidxpairdataset_h
#define posidxpairdataset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		July 2004/Oct 2013/Mar 2016
 RCS:		$Id$
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

 */


mExpClass(General) IdxPairDataSet
{
public:

    typedef IdxPair::IdxType	IdxType;

    /*!\brief position in IdxPairDataSet; an iterator.

      Note that the iterator becomes invalid when adding or removing from
      the set.
     */
    struct SPos
    {
			SPos( int ii=-1, int jj=-1 )
			    : i(ii), j(jj)	{}
	void		reset()			{ i = j = -1; }
	inline bool	operator ==( const SPos& p ) const
						{ return i == p.i && j == p.j; }
	inline bool	operator !=( const SPos& p ) const
						{ return i != p.i || j != p.j; }
	inline bool	operator>( const SPos& p ) const
			{ if ( i>p.i ) return true; return i==p.i && j>p.j; }
	inline bool	operator<( const SPos& p ) const
			{ if ( i<p.i ) return true; return i==p.i && j<p.j; }
	inline bool	isValid() const		{ return i > -1 && j > -1; }

	int		i, j;
    };


			IdxPairDataSet(od_int64 objsz,
					bool allow_duplicate_idxpairs,
					bool manage_data=true);
			IdxPairDataSet(const IdxPairDataSet&);
    virtual		~IdxPairDataSet();
    IdxPairDataSet&	operator =(const IdxPairDataSet&);

    inline bool		isEmpty() const		{ return frsts_.isEmpty(); }
    void		setEmpty();
    void		copyStructureFrom(const IdxPairDataSet&);
			//!< will also empty this set

    inline void		allowDuplicateIdxPairs( bool yn )
			{ allowdup_ = yn; if ( !yn ) removeDuplicateIdxPairs();}
    bool		allowsDuplicateIdxPairs() const { return allowdup_; }
    bool		append(const IdxPairDataSet&);
    void		remove(const IdxPairDataSet&);
    void		remove(const TrcKeySampling& hrg,bool inside);
    void		remove(const SPos&);
			//!< afterwards, SPos may be invalid
    void		remove(const TypeSet<SPos>&);
			//!< You cannot remove while iterating, so ...
			//!< collect the to-be-removed and use this instead

    inline SPos		find( const IdxPair& ip ) const
			{ return findOccurrence( ip, 0 ); }
    SPos		findOccurrence(const IdxPair&,int occ=0) const;
			//!< not found: j < 0.
			//!< still, i can be >= 0 , then the inline is present
			//!< Then, next(pos) will return the first on that inl.
    bool		next(SPos&,bool skip_duplicate_idxpairs=false) const;
    bool		prev(SPos&,bool skip_duplicate_idxpairs=false) const;
    bool		isValid(const IdxPair&) const;

    IdxPair		getIdxPair(const SPos&) const;
    const void*		getObj(const SPos&) const;
    SPos		getPos(od_int64 global_idx) const; //!< Very slow.
    SPos		add(const IdxPair&,const void* obj=0);
    void		set(SPos,const void* obj=0);
			    //!< no checks on whether SPos is actually in set!
			    //!< in doubt, use isValid(SPos) .

    inline int		nrFirst() const		{ return frsts_.size(); }
    int			nrSecond(IdxType firstidx) const;
    inline bool		includes( const IdxPair& ip ) const
						{ return find(ip).j > -1; }
    bool		hasFirst(IdxType) const;
    bool		hasSecond(IdxType) const;
    IdxPair		firstIdxPair() const; //!< when empty returns udf()
    od_int64		totalSize() const;
    Interval<IdxType>	firstRange() const;
    Interval<IdxType>	secondRange(IdxType firsidx=-1) const;

    bool		hasDuplicateIdxPairs() const;
    int			nrDuplicateIdxPairs() const;
    void		removeDuplicateIdxPairs();
    int			nrPos(int lineidx) const; //!< nth line in the set

			// Convenience stuff
    void		extend(const IdxPairDelta& stepout,const IdxPairStep&);
			    //!< Adds only IdxPair postions not yet in set
    void		add(const PosInfo::CubeData&);

			// I/O
    bool		dump(od_ostream&,bool binary) const;
    bool		slurp(od_istream&,bool binary);

			// aliases
    inline int		nrInls() const		    { return nrFirst(); }
    inline int		nrCrls( IdxType inl ) const { return nrSecond(inl); }
    inline int		nrRows() const		    { return nrFirst(); }
    inline int		nrCols( IdxType row ) const { return nrSecond(row); }
    bool		hasInl( IdxType inl ) const { return hasFirst(inl); }
    bool		hasCrl( IdxType crl ) const { return hasSecond(crl); }
    inline bool		hasRow( IdxType row ) const { return hasFirst(row); }
    inline bool		hasCol( IdxType col ) const { return hasSecond(col); }
    Interval<int>	inlRange() const	    { return firstRange(); }
    Interval<int>	rowRange() const	    { return firstRange(); }
    Interval<int>	crlRange( IdxType inl=-1 ) const
						    { return secondRange(inl); }
    Interval<int>	colRange( IdxType row=-1 ) const
						    { return secondRange(row); }

protected:

    typedef TypeSet<IdxType>	IdxSet;
    typedef ObjectSet<const void> ObjSet;
    typedef unsigned char	StorType;

    const od_int64	objsz_;
    const bool		mandata_;
    bool		allowdup_;

    IdxSet		frsts_;
    ObjectSet<IdxSet>	scndsets_;
    ObjectSet<ObjSet>	objsets_;

    void*		getObjCopy(const void*) const;
    void		addNew(SPos&,IdxType,const void*);
    void		deleteObj( const void* obj )
				{ delete [] ((StorType*)obj); }
    void		retireObj( const void* obj )
				{ if ( mandata_ ) deleteObj( obj ); }

    // All 'gt' functions return unchecked
    inline IdxType	gtFrst( const SPos& pos ) const
				{ return frsts_[pos.i]; }
    inline IdxType	gtScnd( const SPos& pos ) const
				{ return gtScndSet(pos)[pos.j]; }
    inline const void*	gtObj( const SPos& pos ) const
				{ return gtObjSet(pos)[pos.j]; }
    inline IdxSet&	gtScndSet( const SPos& pos )
				{ return *scndsets_[pos.i]; }
    inline const IdxSet& gtScndSet( const SPos& pos ) const
				{ return *scndsets_[pos.i]; }
    inline ObjSet&	gtObjSet( const SPos& pos )
				{ return *objsets_[pos.i]; }
    inline const ObjSet& gtObjSet( const SPos& pos ) const
				{ return *objsets_[pos.i]; }
    inline IdxSet&	gtScndSet( int idx )
				{ return *scndsets_[idx]; }
    inline const IdxSet& gtScndSet( int idx ) const
				{ return *scndsets_[idx]; }
    inline ObjSet&	gtObjSet( int idx )
				{ return *objsets_[idx]; }
    inline const ObjSet& gtObjSet( int idx ) const
				{ return *objsets_[idx]; }

};


} // namespace Pos


#endif
