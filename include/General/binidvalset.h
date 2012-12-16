#ifndef binidvalset_h
#define binidvalset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		July 2004
 RCS:		$Id$
________________________________________________________________________

-*/

#include "generalmod.h"
#include "position.h"
#include "sets.h"
#include "ranges.h"
class IOPar;
namespace PosInfo { class CubeData; }
class HorSampling;


/*!\brief A sorted set of BinIDs and values
 
  The set is sorted on both inline and crossline. This has a cost when creating
  the set, and it will be slower than a normal TypeSet for small sets. Further,
  the order in which you add positions will not be preserved.

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
      BinIDValueSet::Pos pos = bivs.getPos( idx );
      // etc.
  }
  \endcode
  but that will be very wasteful.

  When you construct a BinIDValueSet, you must provide the number of values
  (which can be changed later) and whether duplicate BinIDs are allowed.
  In the set, new uninitialised values will be set to mUdf(float).

  if you also want the values sorted (for the same BinID) use
  sortDuplicateBids().

  Note: if one of the values is Z, make it the first value. Not that this
  is enforced by the set, but it will be assumed sooner or later.
 
 */


mClass(General) BinIDValueSet
{
public:

    			BinIDValueSet(int nr_vals,bool allow_duplucate_bids);
			BinIDValueSet(const BinIDValueSet&);
    virtual		~BinIDValueSet();
    BinIDValueSet&	operator =(const BinIDValueSet&);

    inline void		allowDuplicateBids( bool yn )
			{ allowdup_ = yn; if ( !yn ) removeDuplicateBids(); }
    void		setEmpty();
    bool		append(const BinIDValueSet&);
    void		remove(const BinIDValueSet&);
    void		copyStructureFrom(const BinIDValueSet&);
    			//!< will also empty this set

    /*!\brief position in BinIDValueSet. an iterator.

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
	inline bool	valid() const		{ return i > -1 && j > -1; }

	int		i, j;
    };

    Pos			findFirst(const BinID&) const;
    			//!< not found: j < 0.
    			//!< still, i can be >= 0 , then the inline is present
    			//!< Then, next(pos) will return the first on that inl.
    bool		next(Pos&,bool skip_duplcate_bids=false) const;
    bool		prev(Pos&,bool skip_duplcate_bids=false) const;
    bool		valid(const BinID&) const;

    void		get(const Pos&,BinID&,float* v=0,int mxnrvals=-1) const;
    BinID		getBinID(const Pos&) const;
    Pos			getPos(od_int64 global_idx) const;
    			//!< Slow. And 0 < global_idx < totalNr() is not checked
    Pos			add(const BinID&,const float* vs=0);
    			//!< Either pass sufficient data or pass null
    Pos			add(const BinIDValues&);
    			//!< Wrong-sized will be handled correctly
    void		set(Pos,const float* vs=0); //!< null = set to undef
    			//!< no checks on data size (how could there be?)
    			//!< and also not whether Pos is actually in set!
    			//!< in doubt, use valid(Pos) .

    			// more get, set and add: see below

    inline int		nrVals() const		{ return nrvals_; }
    inline int		nrInls() const		{ return inls_.size(); }
    int			nrCrls(int inl) const;
    inline bool		isEmpty() const		{ return !nrInls(); }
    inline bool		includes( const BinID& b ) const
    						{ return findFirst(b).j > -1; }
    int			nrPos(int inlidx) const;
    od_int64		totalSize() const;
    bool		hasInl(int) const;
    bool		hasCrl(int) const;
    BinID		firstPos() const; //!< if empty, returns BinID(0,0)
    Interval<int>	inlRange() const;
    Interval<int>	crlRange(int inl=-1) const;
    Interval<float>	valRange(int) const;

    void		remove(const Pos&);
    			//!< afterwards, Pos may be invalid
    void		remove(const TypeSet<Pos>&);
    			//!< You cannot remove while iterating
    			//!< Collect the to-be-removed and use this instead
    void		removeVal(int); // Will remove entire 'column'
    bool		insertVal(int);
    bool		setNrVals(int,bool kp_data=true);
    int			nrDuplicateBinIDs() const;
    void		sortDuplicateBids(int value_nr,bool ascending=true);
    void		removeDuplicateBids();
    void		randomSubselect(od_int64 maxnr);

    void		extend(const BinID& stepout,const BinID& stepoutstep);
    			//!< Adds only BinID postions not yet in set
    void		removeRange(int valnr,const Interval<float>&,
				    bool inside=true);
    			//!< Removes vectors with value for column valnr
    			//!< in- or outside interval
    void		remove(const HorSampling& hrg,bool inside);

    			// Convenience stuff
    Pos			add(const BinIDValue&);
    Pos			add(const BinID&,float);
    Pos			add(const BinID&,double);
    Pos			add(const BinID&,float,float);
    Pos			add(const BinID&,const TypeSet<float>&);
    void		add(const PosInfo::CubeData&);
    void		get(const Pos&,BinIDValues&) const;
    void		get(const Pos&,BinIDValue&) const;
    void		get(const Pos&,BinID&,float&) const;
    void		get(const Pos&,BinID&,float&,float&) const;
    void		get(const Pos&,BinID&,TypeSet<float>&,
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
    				//!< considers only 'reasonable' lines to add
    bool		putTo(std::ostream&) const;
    bool		areBinidValuesThere(const BinIDValues&) const;

    inline float*	getVals( const Pos& pos )
			{ return valsets_[pos.i]->arr() + nrvals_*pos.j; }
    			//!< Direct access to value arrays. No check on valid()!
    inline const float*	getVals( const Pos& pos ) const
			{ return valsets_[pos.i]->arr() + nrvals_*pos.j; }
    			//!< Direct access to value arrays. No check on valid()!
    inline float	getVal( const Pos& pos, int valnr ) const
    			//!< Direct access to value arrays. No check on valid()!
			{ return getVals(pos)[valnr]; }
    bool		hasDuplicateBinIDs() const;
protected:

    const int			nrvals_;
    TypeSet<int>		inls_;
    ObjectSet< TypeSet<int> >	crlsets_;
    ObjectSet< TypeSet<float> > valsets_;
    bool			allowdup_;

    void		addNew(Pos&,int,const float*);
    void		sortPart(TypeSet<int>&,TypeSet<float>&,
	    			 int,int,int,bool);

    void		removeLine(int idx);

    inline int		getInl( const Pos& pos ) const
    			{ return inls_[pos.i]; }
    inline int		getCrl( const Pos& pos ) const
    			{ return (*crlsets_[pos.i])[pos.j]; }
    inline TypeSet<int>& getCrlSet( const Pos& pos )
			{ return *crlsets_[pos.i]; }
    inline const TypeSet<int>& getCrlSet( const Pos& pos ) const
			{ return *crlsets_[pos.i]; }
    inline TypeSet<float>& getValSet( const Pos& pos )
			{ return *valsets_[pos.i]; }
    inline const TypeSet<float>& getValSet( const Pos& pos ) const
			{ return *valsets_[pos.i]; }
    inline TypeSet<int>& getCrlSet( int idx )
			{ return *crlsets_[idx]; }
    inline const TypeSet<int>& getCrlSet( int idx ) const
			{ return *crlsets_[idx]; }
    inline TypeSet<float>& getValSet( int idx )
			{ return *valsets_[idx]; }
    inline const TypeSet<float>& getValSet( int idx ) const
			{ return *valsets_[idx]; }

    friend class	DataPointSet;
    friend class	PosVecDataSet;
};


#endif

