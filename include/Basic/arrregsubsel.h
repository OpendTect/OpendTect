#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2018
________________________________________________________________________

-*/

#include "basicmod.h"
#include "rowcol.h"
#include "ranges.h"



/*!\brief data describing the regular subselection of an array.

  The subselection has 3 parts:
  * An offset from the start
  * A step
  * The total size of the subselection itself

 */

mExpClass(Basic) ArrRegSubSelData
{
public:

    typedef Index_Type			idx_type;
    typedef idx_type			size_type;

			ArrRegSubSelData( size_type sz )
			    : sz_(sz)		{}
			mImplSimpleEqOpers3Memb(ArrRegSubSelData,
						offs_,step_,sz_)

    inline bool		isEmpty() const		{ return sz_ < 1; }
    inline bool		hasOffset() const	{ return offs_ != 0; }
    inline bool		isSubSpaced() const	{ return step_ > 1; }

    inline idx_type	offset() const		{ return offs_; }
    inline idx_type	step() const		{ return step_; }
    inline size_type	size() const		{ return sz_; }

    inline idx_type&	offset()		{ return offs_; }
    inline idx_type&	step()			{ return step_; }
    inline size_type&	size()			{ return sz_; }

    inline idx_type	arrIdx( idx_type subselidx ) const
			{ return offs_ + step_*subselidx; }
    inline idx_type	subSelIdx( idx_type arridx ) const
			{ return (arridx-offs_) / step_; }

protected:

    idx_type		offs_			= 0;
    idx_type		step_			= 1;
    size_type		sz_;

};


/*!\brief base class for regular array subselectors */

mExpClass(Basic) ArrRegSubSel
{
public:

    typedef ArrRegSubSelData	Data;
    mUseType( Data,		idx_type );
    mUseType( Data,		size_type );
    typedef od_int64		totsz_type;

    virtual		~ArrRegSubSel()			{}

    inline bool		isEmpty() const;
    inline bool		hasOffset() const;
    inline bool		isSubSpaced() const;
    inline totsz_type	totalSize() const;
    inline Data&	data( idx_type idim )		{ return gtData(idim); }
    inline const Data&	data( idx_type idim ) const	{ return gtData(idim); }

    virtual size_type	nrDims() const			= 0;

protected:

    virtual Data&	gtData(idx_type) const		= 0;

};


/*!\brief base class for regular array subselection iterators.
  It will (may) become valid after the first next(). */

mExpClass(Basic) ArrRegSubSelIterator
{
public:

    mUseType( ArrRegSubSelData,	idx_type );
    mUseType( ArrRegSubSelData,	size_type );

    virtual void	toStart()			= 0;
    virtual void	startAt(idx_type,idx_type idim)	= 0;
    virtual bool	next()				= 0;
    virtual bool	isValid() const			= 0;
    virtual idx_type	arrIdx(idx_type idim) const	= 0;

};


/*!\brief base class for regular array subselectors in one dimension */

mExpClass(Basic) ArrRegSubSel1D : public ArrRegSubSel
{
public:

    inline bool		isAll( size_type inpsz ) const
			{ return size() == inpsz
			      && !hasOffset() && !isSubSpaced(); }

    inline size_type	nrDims() const override	{ return 1; }
    inline Data&	data()			{ return ArrRegSubSel::data(0);}
    inline const Data&	data() const		{ return ArrRegSubSel::data(0);}

    inline idx_type	offset() const		{ return data().offset(); }
    inline idx_type	step() const		{ return data().step(); }
    inline size_type	size() const		{ return data().size(); }

    inline idx_type&	offset()		{ return data().offset(); }
    inline idx_type&	step()			{ return data().step(); }
    inline size_type&	size()			{ return data().size(); }

    inline idx_type	arrIdx( idx_type i ) const
			{ return data().arrIdx(i); }
    inline idx_type	subSelIdx( idx_type i ) const
			{ return data().subSelIdx(i); }
    inline bool		validIdx( idx_type i ) const
			{ return i < size(); }

};


/*!\brief iterates a 1D regular array subselection */

mExpClass(Basic) ArrRegSubSel1DIterator : public ArrRegSubSelIterator
{
public:

		ArrRegSubSel1DIterator( const ArrRegSubSel1D& ss )
		    : subsel_(ss)
		    , sz_(ss.size())	    { toStart(); }

    const ArrRegSubSel1D&   subsel_;
    idx_type		    idx_;
    const size_type	    sz_;

    inline void		toStart() override  { idx_ = -1; }
    inline void		startAt(idx_type,idx_type idim=0) override;
    inline bool		next() override;
    inline bool		isValid() const override { return idx_ >= 0; }
    inline idx_type	arrIdx(idx_type idim=0) const override
			{ return subsel_.arrIdx(idx_); }

};


/*!\brief provides regular array subselection in one dimension */

mExpClass(Basic) PlainArrRegSubSel1D : public ArrRegSubSel1D
{
public:

		PlainArrRegSubSel1D( size_type sz )
		    : data_(sz)			{}
		mImplSimpleEqOpers1Memb(PlainArrRegSubSel1D,data_)

protected:

    Data	data_;

    Data&	gtData( idx_type ) const override
		{ return const_cast<Data&>( data_ ); }

};


/*!\brief base class for regular array subselectors in two dimensions */

mExpClass(Basic) ArrRegSubSel2D : public ArrRegSubSel
{
public:

    inline bool		isAll( size_type inpsz0, size_type inpsz1 ) const
			{ return size(0) == inpsz0 && size(1) == inpsz1
			      && !hasOffset() && !isSubSpaced(); }

    inline size_type	nrDims() const override	{ return 2; }

    inline idx_type	offset( idx_type idim ) const
			{ return data(idim).offset(); }
    inline idx_type	step( idx_type idim ) const
			{ return data(idim).step(); }
    inline idx_type	size( idx_type idim ) const
			{ return data(idim).size(); }

    inline idx_type&	offset( idx_type idim )	{ return data(idim).offset(); }
    inline idx_type&	step( idx_type idim )	{ return data(idim).step(); }
    inline size_type&	size( idx_type idim )	{ return data(idim).size(); }

    inline idx_type	arrIdx( idx_type idim, idx_type i ) const
			{ return data(idim).arrIdx(i); }
    inline idx_type	subSelIdx( idx_type idim, idx_type i ) const
			{ return data(idim).subSelIdx(i); }
    inline bool		validIdxs( idx_type i0, idx_type i1 ) const
			{ return i0 < size(0) && i1 < size(1); }

};


/*!\brief iterates a 2D regular array subselection */

mExpClass(Basic) ArrRegSubSel2DIterator : public ArrRegSubSelIterator
{
public:

		ArrRegSubSel2DIterator( const ArrRegSubSel2D& ss )
		    : subsel_(ss)
		    , sz0_(ss.size(0)), sz1_(ss.size(1))	{ toStart(); }

    const ArrRegSubSel2D&   subsel_;
    idx_type		    idx0_, idx1_;
    const size_type	    sz0_, sz1_;

    inline void		toStart() override	{ idx0_ = 0; idx1_ = -1; }
    inline void		startAt(idx_type,idx_type idim) override;
    inline bool		next();
    inline bool		isValid() const override { return idx1_ >= 0; }
    inline idx_type	arrIdx( idx_type idim ) const override
			{ return subsel_.arrIdx(idim,idim?idx1_:idx0_); }

};


/*!\brief provides regular array subselection in two dimensions */

mExpClass(Basic) PlainArrRegSubSel2D : public ArrRegSubSel2D
{
public:

		PlainArrRegSubSel2D( size_type sz0, size_type sz1 )
		    : data0_(sz0), data1_(sz1)	{}
		PlainArrRegSubSel2D( const Data& d0, const Data& d1 )
		    : data0_(d0), data1_(d1)	{}
		mImplSimpleEqOpers2Memb(PlainArrRegSubSel2D,data0_,data1_)

protected:

    Data	data0_;
    Data	data1_;

    Data&	gtData( idx_type idim ) const override
		{ return const_cast<Data&>( idim ? data1_ : data0_ ); }

};


/*!\brief base class for regular array subselectors in three dimensions */

mExpClass(Basic) ArrRegSubSel3D : public ArrRegSubSel
{
public:

    inline bool		isAll( size_type inpsz0, size_type inpsz1,
				size_type inpsz2 ) const
			{ return size(0) == inpsz0 && size(1) == inpsz1
			      && size(2) == inpsz2
			      && !hasOffset() && !isSubSpaced(); }

    inline size_type	nrDims() const override	{ return 3; }

    inline idx_type	offset( idx_type idim ) const
			{ return data(idim).offset(); }
    inline idx_type	step( idx_type idim ) const
			{ return data(idim).step(); }
    inline idx_type	size( idx_type idim ) const
			{ return data(idim).size(); }

    inline idx_type&	offset( idx_type idim )	{ return data(idim).offset(); }
    inline idx_type&	step( idx_type idim )	{ return data(idim).step(); }
    inline size_type&	size( idx_type idim )	{ return data(idim).size(); }

    inline idx_type	arrIdx( idx_type idim, idx_type i ) const
			{ return data(idim).arrIdx(i); }
    inline idx_type	subSelIdx( idx_type idim, idx_type i ) const
			{ return data(idim).subSelIdx(i); }

    inline size_type	size2D() const
			{ return size( 0 ) * size( 1 ); }
    inline bool		validIdxs( idx_type i0, idx_type i1, idx_type i2 ) const
			{ return i0 < size(0) && i1 < size(1) && i2 < size(2); }

};


/*!\brief iterates a 3D regular array subselection */

mExpClass(Basic) ArrRegSubSel3DIterator : public ArrRegSubSelIterator
{
public:

		ArrRegSubSel3DIterator( const ArrRegSubSel3D& ss )
		    : subsel_(ss)
		    , sz0_(ss.size(0)), sz1_(ss.size(1))
		    , sz2_(ss.size(2))			{ toStart(); }

    const ArrRegSubSel3D&   subsel_;
    idx_type		    idx0_, idx1_, idx2_;
    const size_type	    sz0_, sz1_, sz2_;

    inline void		toStart() override  { idx0_ = idx1_ = 0, idx2_ = -1; }
    inline void		startAt(idx_type,idx_type idim) override;
    inline bool		next();
    inline bool		isValid() const override { return idx2_ >= 0; }
    inline idx_type	arrIdx( idx_type idim ) const override
			{ return subsel_.arrIdx( idim,
				    !idim ? idx0_ : (idim==1?idx1_:idx2_) ); }

};


/*!\brief provides regular array subselection in three dimensions */

mExpClass(Basic) PlainArrRegSubSel3D : public ArrRegSubSel3D
{
public:

		PlainArrRegSubSel3D( size_type sz0, size_type sz1,
				     size_type sz2 )
		    : data0_(sz0), data1_(sz1), data2_(sz2)	{}
		PlainArrRegSubSel3D( const Data& d0, const Data& d1,
				     const Data& d2 )
		    : data0_(d0), data1_(d1), data2_(d2)	{}
		mImplSimpleEqOpers3Memb(PlainArrRegSubSel3D,
					data0_,data1_,data2_)

protected:

    Data	data0_;
    Data	data1_;
    Data	data2_;

    Data&	gtData( idx_type idim ) const override
		{ return const_cast<Data&>( !idim ? data0_
				: (idim==1 ? data1_ : data2_) ); }

};


inline bool ArrRegSubSel::isEmpty() const
{
    const auto nrdims = nrDims();
    for ( auto idx=0; idx<nrdims; idx++ )
	if ( !data(idx).isEmpty() )
	    return false;
    return true;
}


inline bool ArrRegSubSel::isSubSpaced() const
{
    const auto nrdims = nrDims();
    for ( auto idx=0; idx<nrdims; idx++ )
	if ( data(idx).isSubSpaced() )
	    return true;
    return false;
}


inline bool ArrRegSubSel::hasOffset() const
{
    const auto nrdims = nrDims();
    for ( auto idx=0; idx<nrdims; idx++ )
	if ( data(idx).hasOffset() )
	    return true;
    return false;
}


inline ArrRegSubSel::totsz_type ArrRegSubSel::totalSize() const
{
    const auto nrdims = nrDims();
    totsz_type ret = data(0).size();
    for ( auto idx=1; idx<nrdims; idx++ )
	ret *= data(idx).size();
    return ret;
}


inline void ArrRegSubSel1DIterator::startAt( idx_type idx, idx_type )
{
    idx_ = idx - 1;
}


inline bool ArrRegSubSel1DIterator::next()
{
    idx_++;
    if ( idx_ >= sz_ )
	{ idx_ = -1; return false; }
    return true;
}


inline void ArrRegSubSel2DIterator::startAt( idx_type idx, idx_type idim )
{
    if ( idim )
	idx1_ = idx - 1;
    else
	idx0_ = idx;
}


inline bool ArrRegSubSel2DIterator::next()
{
    idx1_++;
    if ( idx1_ >= sz1_ )
    {
	idx0_++; idx1_ = 0;
	if ( idx0_ < sz0_ )
	    idx1_ = 0;
	else
	    { idx1_ = -1; idx0_ = 0; return false; }
    }
    return true;
}


inline void ArrRegSubSel3DIterator::startAt( idx_type idx, idx_type idim )
{
    if ( idim > 1 )
	idx2_ = idx - 1;
    else
	(idim ? idx1_ : idx0_) = idx;
}


inline bool ArrRegSubSel3DIterator::next()
{
    idx2_++;
    if ( idx2_ >= sz2_ )
    {
	idx1_++;
	if ( idx1_ >= sz1_ )
	{
	    idx0_++;
	    if ( idx0_ >= sz0_ )
		{ idx0_ = idx1_ = 0; idx2_ = -1; return false; }
	    idx1_ = 0;
	}
	idx2_ = 0;
    }
    return true;
}
