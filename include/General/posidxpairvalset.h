#ifndef posidxpairvalset_h
#define posidxpairvalset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		July 2004/Oct 2013
 RCS:		$Id$
________________________________________________________________________

-*/

#include "generalmod.h"
#include "position.h"
#include "sets.h"
#include "ranges.h"
namespace PosInfo { class CubeData; }
class HorSampling;

namespace Pos
{
template <class IPT,class FT> class ValueIdxPair;
template <class IPT,class FT> class IdxPairValues;


/*!\brief A sorted set of IdxPairs and associated values
 
  The set is sorted on both first and second key (inl()/row()/lineNr() and
  crl()/col()/traceNr()). This has a cost when creating the set, and it will be slower
  than a normal TypeSet for small sets. Further, the order in which you add positions
  will not be preserved.

  Luckily there are also advantages. It is *much* faster for searching when
  the set is large. When you have a block of N inlines x N crosslines, the
  search time will be O( 2 log2(N) ) instead of O( N^2 / 2 ). Thus for
  1000x1000 instead of 500000 you need 20 comparisons (probably a few more but
  not a lot more).

  The iteration through the set should be done using the Pos iterator class.
  All positioning is already done with Pos, but you can in theory still use
  \code
  const od_int64 sz = totalSize();
  for ( od_int64 idx=0; idx<sz; idx++ ) 
  {
      IdxPairValueSet::Pos pos = bivs.getPos( idx );
      // etc.
  }
  \endcode
  but that will be very wasteful.

  When you construct a IdxPairValueSet, you must provide the number of values
  (which can be changed later) and whether duplicate IdxPairs are allowed.
  In the set, new uninitialised values will be set to mUdf(float).

  if you also want the values sorted (for the same IdxPair) use
  sortDuplicateIdxPairs().

  Note: if one of the values is Z, make it the first value. Not that this
  is enforced by the set, but it will be assumed sooner or later.
 
 */


mExpClass(General) IdxPairValueSet
{
public:

    typedef IdxPair::IdxType			IdxType;
    typedef ValueIdxPair<IdxPair,float>		PairVal;
    typedef IdxPairValues<IdxPair,float>	DataRow;

    			IdxPairValueSet(int nr_vals,bool allow_duplucate_bids);
			IdxPairValueSet(const IdxPairValueSet&);
    virtual		~IdxPairValueSet();
    IdxPairValueSet&	operator =(const IdxPairValueSet&);

    inline void		allowDuplicateIdxPairs( bool yn )
			{ allowdup_ = yn; if ( !yn ) removeDuplicateIdxPairs(); }
    void		setEmpty();
    bool		append(const IdxPairValueSet&);
    void		remove(const IdxPairValueSet&);
    void		copyStructureFrom(const IdxPairValueSet&);
    			//!< will also empty this set

    /*!\brief position in IdxPairValueSet; an iterator.

      Note that the iterator becomes invalid when adding or removing from
      the set.
     */
    struct Pos
    {
			Pos( int ii=-1, int jj=-1 ) : i(ii), j(jj)	{}
	void		reset()			{ i = j = -1; }
	inline bool	operator ==( const Pos& p ) const
						{ return i == p.i && j == p.j; }
	inline bool	operator !=( const Pos& p ) const
						{ return i != p.i || j != p.j; }
	inline bool	operator>( const Pos& p ) const
			{ if ( i>p.i ) return true; return i==p.i && j>p.j; }
	inline bool	operator<( const Pos& p ) const
			{ if ( i<p.i ) return true; return i==p.i && j<p.j; }
	inline bool	isValid() const		{ return i > -1 && j > -1; }

	int		i, j;
    };

    inline Pos		find( const IdxPair& ip ) const
			{ return findOccurrence( ip, 0 ); }
    Pos			findOccurrence(const IdxPair&,int occ=0) const;
    			//!< not found: j < 0.
    			//!< still, i can be >= 0 , then the inline is present
    			//!< Then, next(pos) will return the first on that inl.
    bool		next(Pos&,bool skip_duplcate_bids=false) const;
    bool		prev(Pos&,bool skip_duplcate_bids=false) const;
    bool		valid(const IdxPair&) const;

    void		get(const Pos&,IdxPair&,float* v=0,
	    			int mxnrvals=-1) const;
    IdxPair		getIdxPair(const Pos&) const;
    Pos			getPos(od_int64 global_idx) const;
			    //!< Slow. And no check on idx in range
    Pos			add(const IdxPair&,const float* vs=0);
			    //!< Either pass sufficient data or pass null
    Pos			add(const DataRow&);
			    //!< Wrong-sized will be handled correctly
    void		set(Pos,const float* vs=0); //!< null = set to undef
			    //!< no checks on data size (how could there be?)
			    //!< and also not whether Pos is actually in set!
			    //!< in doubt, use valid(Pos) .

    			// more get, set and add: see below

    inline int		nrVals() const		{ return nrvals_; }
    inline int		nrFirst() const		{ return inls_.size(); }
    int			nrSecond(IdxType _first) const;
    inline bool		isEmpty() const		{ return nrFirst() < 1; }
    inline bool		includes( const IdxPair& b ) const
    						{ return findFirst(b).j > -1; }
    bool		hasFirst(IdxType) const;
    bool		hasSecond(IdxType) const;
    IdxPair		firstIdxPair() const;
    				//!< if empty, returns IdxPair::udf()
    od_int64		totalSize() const;
    Interval<int>	firstRange() const;
    Interval<int>	secondRange(int _first=-1) const;
    Interval<float>	valRange(int) const;

    void		remove(const Pos&);
    			//!< afterwards, Pos may be invalid
    void		remove(const TypeSet<Pos>&);
    			//!< You cannot remove while iterating
    			//!< Collect the to-be-removed and use this instead
    void		removeVal(int); //!< Will remove entire 'column'
    bool		insertVal(int); //<! Will add a 'column'
    bool		setNrVals(int,bool kp_data=true);
    int			nrDuplicateIdxPairs() const;
    void		sortDuplicateIdxPairs(int value_nr,bool ascending=true);
    void		removeDuplicateIdxPairs();
    void		randomSubselect(od_int64 maxnr);

    void		extend(const IdxPairDelta& stepout,const IdxPairStep&);
			    //!< Adds only IdxPair postions not yet in set
    void		removeRange(int valnr,const Interval<float>&,
				    bool inside=true);
			    //!< Removes vectors with value for column valnr
			    //!< in- or outside interval
    void		remove(const HorSampling& hrg,bool inside);

    			// Convenience stuff
    Pos			add(const PairVal&);
    Pos			add(const IdxPair&,float);
    Pos			add(const IdxPair&,double);
    Pos			add(const IdxPair&,float,float);
    Pos			add(const IdxPair&,const TypeSet<float>&);
    void		add(const PosInfo::CubeData&);
    void		get(const Pos&,DataRow&) const;
    void		get(const Pos&,PairVal&) const;
    void		get(const Pos&,IdxPair&,float&) const;
    void		get(const Pos&,IdxPair&,float&,float&) const;
    void		get(const Pos&,IdxPair&,TypeSet<float>&,
	    		    int maxnrvals=-1) const; //!< max == -1 => all
    void		set(const Pos&,float);
    void		set(const Pos&,float,float);
    void		set(const Pos&,const TypeSet<float>&);
    void		getColumn(int valnr,TypeSet<float>&,bool incudf) const;

    			// Slow! Can still come in handly for small sets
    void		fillPar(IOPar&,const char* key) const;
    void		usePar(const IOPar&,const char* key);

    			// Fast
    bool		getFrom(std::istream&);
				//!< re-structures but keeps allowdup_
    bool		putTo(std::ostream&) const;
    template <class IPT>
    bool		includes(const DataRow&) const;

    int			nrPos(int lineidx) const; //!< nth line in the set
    inline float*	getVals( const Pos& pos )
			{ return valsets_[pos.i]->arr() + nrvals_*pos.j; }
    			//!< Direct access to value arrays. No check on valid()!
    inline const float*	getVals( const Pos& pos ) const
			{ return valsets_[pos.i]->arr() + nrvals_*pos.j; }
    			//!< Direct access to value arrays. No check on valid()!
    inline float	getVal( const Pos& pos, int valnr ) const
    			//!< Direct access to value arrays. No check on valid()!
			{ return getVals(pos)[valnr]; }
    bool		hasDuplicateIdxPairs() const;
    bool		haveDataRow(const DataRow&) const;

    			// aliases
    inline int		nrInls() const		{ return nrFirst(); }
    inline int		nrCrls( int inl ) const	{ return nrSecond(row); }
    inline int		nrRows() const		{ return nrFirst(); }
    inline int		nrCols( int row ) const	{ return nrSecond(row); }
    bool		hasInl( int inl ) const	{ return hasFirst(inl); }
    bool		hasCrl( int crl ) const	{ return hasSecond(crl); }
    inline bool		hasRow( int row ) const	{ return hasFirst(row); }
    inline bool		hasCol( int col ) const	{ return hasSecond(col); }
    Interval<int>	inlRange() const	{ return firstRange(); }
    Interval<int>	rowRange() const	{ return firstRange(); }
    Interval<int>	crlRange( int inl=-1 ) const
						{ return secondRange(crl); }
    Interval<int>	colRange( int row=-1 ) const
    						{ return secondRange(row); }

protected:

    const int			nrvals_;
    TypeSet<int>		frsts_;
    ObjectSet< TypeSet<int> >	scndsets_;
    ObjectSet< TypeSet<float> > valsets_;
    bool			allowdup_;

    void			addNew(Pos&,int,const float*);
    void			sortPart(TypeSet<int>&,TypeSet<float>&,
	    				 int,int,int,bool);

    void			removeLine(int idx);

    inline int			getInl( const Pos& pos ) const
					{ return inls_[pos.i]; }
    inline int			getCrl( const Pos& pos ) const
					{ return (*crlsets_[pos.i])[pos.j]; }
    inline TypeSet<int>&	getCrlSet( const Pos& pos )
					{ return *crlsets_[pos.i]; }
    inline const TypeSet<int>&	getCrlSet( const Pos& pos ) const
					{ return *crlsets_[pos.i]; }
    inline TypeSet<float>&	getValSet( const Pos& pos )
					{ return *valsets_[pos.i]; }
    inline const TypeSet<float>& getValSet( const Pos& pos ) const
					{ return *valsets_[pos.i]; }
    inline TypeSet<int>&	getCrlSet( int idx )
					{ return *crlsets_[idx]; }
    inline const TypeSet<int>&	getCrlSet( int idx ) const
					{ return *crlsets_[idx]; }
    inline TypeSet<float>&	getValSet( int idx )
					{ return *valsets_[idx]; }
    inline const TypeSet<float>& getValSet( int idx ) const
					{ return *valsets_[idx]; }

    friend class	DataPointSet;
    friend class	PosVecDataSet;

};


#endif

