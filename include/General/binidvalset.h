#ifndef binidvalset_h
#define binidvalset_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		July 2004
 RCS:		$Id: binidvalset.h,v 1.4 2005-01-17 16:34:41 bert Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "sets.h"
#include "ranges.h"
class IOPar;


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
  const int sz = totalSize();
  for ( int idx=0; idx<sz; idx++ ) 
  {
      BinIDValueSet::Pos pos = bivs.getPos( idx );
      // etc.
  }
  \endcode
  but that will be very wasteful.

  When you construct a BinIDValueSet, you must provide the number of values
  (which can be changed later) and whether duplicate BinIDs are allowed.
  In the set, new uninitialised values will be set to mUndefValue.

  if you also want the values sorted (for the same BinID) use
  sortDuplicateBids().

  Note: if one of the values is Z, make it the first value. Not that this
  is enforced by the set, but it will be assumed sooner or later.
 
 */


class BinIDValueSet
{
public:

    			BinIDValueSet(int nr_vals,bool allow_duplucate_bids);
			BinIDValueSet(const BinIDValueSet&);
    virtual		~BinIDValueSet();
    BinIDValueSet&	operator =(const BinIDValueSet&);

    inline void		allowDuplicateBids( bool yn )
			{ allowdup = yn; if ( !yn ) removeDuplicateBids(); }
    void		empty();
    void		append(const BinIDValueSet&);
    void		copyStructureFrom(const BinIDValueSet&);
    			//!< will also empty this set

    /*!\brief position in BinIDValueSet. an iterator. */
    struct Pos
    {
			Pos( int ii=-1, int jj=-1 ) : i(ii), j(jj)	{}
	inline bool	operator ==( const Pos& p ) const
						{ return i == p.i && j == p.j; }
	inline bool	operator !=( const Pos& p ) const
						{ return i != p.i || j != p.j; }
	inline bool	valid() const		{ return i > -1 && j > -1; }

	int		i, j;
    };

    Pos			findFirst(const BinID&) const;
    			//!< not found: j < 0.
    			//!< still, i can be >= 0 , then the inline is present
    			//!< Then, next(pos) will return the first on that inl.
    bool		next(Pos&,bool skip_duplcate_bids=false) const;
    bool		prev(Pos&,bool skip_duplcate_bids=false) const;
    bool		valid(const Pos&) const;

    void		get(const Pos&,BinID&,float* v=0) const;
    Pos			getPos(int global_idx) const;
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

    inline int		nrVals() const		{ return nrvals; }
    inline int		nrInls() const		{ return inls.size(); }
    inline bool		isEmpty() const		{ return !nrInls(); }
    inline bool		includes( const BinID& b ) const
    						{ return findFirst(b).j > -1; }
    int			nrPos(int inlidx) const;
    int			totalSize() const;
    bool		hasInl(int) const;
    bool		hasCrl(int) const;
    BinID		firstPos() const; //!< if empty, returns BinID(0,0)
    Interval<int>	inlRange() const;
    Interval<int>	crlRange() const;
    Interval<float>	valRange(int) const;

    void		remove(const Pos&);
    void		removeVal(int); // Will remove entire 'column'
    void		setNrVals(int,bool kp_data=true);
    void		sortDuplicateBids(int value_nr,bool ascending=true);
    void		removeDuplicateBids();

    void		extend(const BinID& stepout,const BinID& stepoutstep);
    			//!< Adds only BinID postions not yet in set

    			// Convenience stuff
    Pos			add(const BinIDValue&);
    Pos			add(const BinID&,float);
    Pos			add(const BinID&,float,float);
    Pos			add(const BinID&,const TypeSet<float>&);
    void		get(const Pos&,BinIDValues&) const;
    void		get(const Pos&,BinIDValue&) const;
    void		get(const Pos&,BinID&,float&) const;
    void		get(const Pos&,BinID&,float&,float&) const;
    void		get(const Pos&,BinID&,TypeSet<float>&) const;
    void		set(const Pos&,float);
    void		set(const Pos&,float,float);
    void		set(const Pos&,const TypeSet<float>&);

    			// Slow! Can still come in handly for small sets
    void		fillPar(IOPar&,const char* key) const;
    void		usePar(const IOPar&,const char* key);

    			// Fast
    bool		getFrom(std::istream&);
				//!< re-structures but keeps allowdup
    bool		putTo(std::ostream&) const;

    			// Direct access to value arrays
    inline float*	getVals( const Pos& p )		{ return gtVals(p); }
    inline const float*	getVals( const Pos& p ) const	{ return gtVals(p); }

protected:

    const int			nrvals;
    TypeSet<int>		inls;
    ObjectSet< TypeSet<int> >	crlsets;
    ObjectSet< TypeSet<float*> > valsets;
    bool			allowdup;

    void	addNew(Pos&,int,const float*);
    void	sortPart(TypeSet<int>&,TypeSet<float*>&,int,int,int,bool);
    float*	gtVals(const Pos&) const;

};


#endif
