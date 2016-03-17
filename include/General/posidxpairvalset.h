#ifndef posidxpairvalset_h
#define posidxpairvalset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2004/Oct 2013/Mar 2016
 RCS:		$Id$
________________________________________________________________________

-*/

#include "generalmod.h"
#include "posidxpairdataset.h"


namespace Pos
{
template <class IPT,class FT> class ValueIdxPair;
template <class IPT,class FT> class IdxPairValues;


/*!\brief uses an IdxPairDataSet to hold arrays of floats

  When you construct a IdxPairValueSet, you must provide the number of values
  (which can be changed later) and whether duplicate IdxPairs are allowed.
  In the set, new uninitialized values will be set to mUdf(float).

  Note: if one of the values is Z, make it the first value. Not that this
  is enforced by the set, but it will be assumed sooner or later.

 */


mExpClass(General) IdxPairValueSet
{
public:

    typedef IdxPair::IdxType			IdxType;
    typedef ValueIdxPair<IdxPair,float>		PairVal;
    typedef IdxPairValues<IdxPair,float>	DataRow;
    typedef IdxPairDataSet::SPos		SPos;
    typedef IdxPairDataSet::ArrIdxType		ArrIdxType;
    typedef IdxPairDataSet::GlobIdxType		GlobIdxType;

			IdxPairValueSet(int nr_vals,bool allow_dupl_idxpairs);
    virtual		~IdxPairValueSet()		{}
    IdxPairValueSet&	operator =(const IdxPairValueSet&);

    inline void		allowDuplicateIdxPairs( bool yn )
			{ return data_.allowDuplicateIdxPairs(yn); }
    bool		allowsDuplicateIdxPairs() const
			{ return data_.allowsDuplicateIdxPairs(); }
    void		setEmpty()
			{ data_.setEmpty(); }
    bool		append( const IdxPairValueSet& oth )
			{ return data_.append( oth.data_ ); }
    void		remove( const IdxPairValueSet& oth )
			{ data_.remove( oth.data_ ); }
    void		copyStructureFrom(const IdxPairValueSet&);
			//!< will also empty this set

    inline SPos		find( const IdxPair& ip ) const
			{ return data_.find( ip ); }
    SPos		findOccurrence( const IdxPair& ip, int occ=0 ) const
			{ return data_.findOccurrence( ip, occ ); }
    bool		next( SPos& spos, bool skip_dupl_idxpairs=false ) const
			{ return data_.next( spos, skip_dupl_idxpairs ); }
    bool		prev( SPos& spos, bool skip_dupl_idxpairs=false ) const
			{ return data_.prev( spos, skip_dupl_idxpairs ); }
    bool		isValid( const IdxPair& spos ) const
			{ return data_.isValid( spos ); }

    void		get(const SPos&,float* v=0,int mxnrvals=-1) const;
    void		get(const SPos&,IdxPair&,float* v=0,int mxnrv=-1) const;
    IdxPair		getIdxPair( const SPos& spos ) const
			{ return data_.getIdxPair( spos ); }
    SPos		getPos(GlobIdxType global_idx) const
			{ return data_.getPos( global_idx ); }
    SPos		add(const IdxPair&,const float* vs=0);
			    //!< Either pass sufficient data or pass null
    SPos		add(const DataRow&);
			    //!< Wrong-sized will be handled correctly
    void		set(SPos,const float* vs=0); //!< null = set to undef
			    //!< no checks on data size (how could there be?)
			    //!< and also not whether SPos is actually in set!
			    //!< in doubt, use isValid(SPos) .

			// more get, set and add: see below

    inline int		nrVals() const
			{ return nrvals_; }
    inline IdxType	nrFirst() const
			{ return data_.nrFirst(); }
    inline IdxType	nrSecond( IdxType firstidx ) const
			{ return data_.nrSecond(firstidx); }
    inline bool		isEmpty() const
			{ return data_.isEmpty(); }
    inline bool		includes( const IdxPair& ip ) const
			{ return data_.includes( ip ); }
    inline bool		hasFirst( IdxType inl ) const
			{ return data_.hasFirst( inl ); }
    inline bool		hasSecond( IdxType crl ) const
			{ return data_.hasSecond( crl ); }
    inline IdxPair	firstIdxPair() const
			{ return data_.firstIdxPair(); }
    inline GlobIdxType	totalSize() const
			{ return data_.totalSize(); }
    inline Interval<IdxType> firstRange() const
			{ return data_.firstRange(); }
    inline Interval<IdxType> secondRange( IdxType frstidx=-1 ) const
			{ return data_.secondRange( frstidx ); }
    Interval<float>	valRange(int valnr) const;

    bool		insertVal(int); //<! Will add a 'column'
    bool		setNrVals(int);
    inline bool		hasDuplicateIdxPairs() const
			{ return data_.hasDuplicateIdxPairs(); }
    inline ArrIdxType	nrDuplicateIdxPairs() const
			{ return data_.nrDuplicateIdxPairs(); }
    inline void		removeDuplicateIdxPairs()
			{ data_.removeDuplicateIdxPairs(); }
    inline void		randomSubselect( od_int64 maxsz )
			{ data_.randomSubselect( maxsz ); }

    void		extend(const IdxPairDelta& stepout,const IdxPairStep&);
    void		add(const PosInfo::CubeData&);
    inline void		remove( const SPos& spos )
			{ data_.remove( spos ); }
    inline void		remove( const TypeSet<SPos>& torem )
			{ data_.remove( torem ); }
    void		removeRange(int valnr,const Interval<float>&,
				    bool inside=true);
    inline void		remove( const TrcKeySampling& hrg, bool inside )
			{ data_.remove( hrg, inside ); }
    void		removeVal(int); //!< Will remove entire 'column'

			// Convenience stuff
    SPos		add(const PairVal&);
    SPos		add(const IdxPair&,float);
    SPos		add(const IdxPair&,double);
    SPos		add(const IdxPair&,float,float);
    SPos		add(const IdxPair&,const TypeSet<float>&);
    void		get(const SPos&,DataRow&) const;
    void		get(const SPos&,PairVal&) const;
    void		get(const SPos&,IdxPair&,float&) const;
    void		get(const SPos&,IdxPair&,float&,float&) const;
    void		get(const SPos&,IdxPair&,TypeSet<float>&,
	    			int mxnrvals=-1) const;
    void		get(const SPos&,TypeSet<float>&,int maxnrvals=-1) const;
    void		set(const SPos&,float);
    void		set(const SPos&,float,float);
    void		set(const SPos&,const TypeSet<float>&);
    void		getColumn(int valnr,TypeSet<float>&,bool incudf) const;

			// Slow! Can still come in handly for small sets
    void		fillPar(IOPar&,const char* key) const;
    void		usePar(const IOPar&,const char* key);

			// Fast
    bool		getFrom(od_istream&,Pos::GeomID=mUdf(Pos::GeomID));
				//!< detects/converts coords if geomid passed
    bool		putTo(od_ostream&) const;

    inline ArrIdxType	nrPos( ArrIdxType lineidx ) const
			{ return data_.nrPos(lineidx); }
    float*		getVals(const SPos&);
			    //!< Direct access to value arrays.
    const float*	getVals(const SPos&) const;
			    //!< Direct access to value arrays.
    float		getVal(const SPos& pos,int valnr) const;
			    //!< Direct access to value arrays.
    bool		haveDataRow(const DataRow&) const;

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

protected:

    const int			nrvals_;
    IdxPairDataSet		data_;

    inline float*		gtVals( const SPos& spos )
				{ return (float*)data_.getObj( spos ); }
    inline const float*		gtVals( const SPos& spos ) const
				{ return (const float*)data_.getObj( spos ); }

    friend class		DataPointSet;
    friend class		PosVecDataSet;

};


} // namespace Pos


#endif
