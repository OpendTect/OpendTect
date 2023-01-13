#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"

#include "arrayndimpl.h"
#include "mathfunc.h"
#include "objectset.h"
#include "thread.h"

/*!\brief A manager used for constructing the table necessary for Sinc
 interpolations.

 The manager creates one table per design and provides its as necessary
 A table contains shifted, stretched KaiserWindow functions in both directions
*/

mExpClass(Algo) SincTableManager
{
public:
			SincTableManager()	{};
			~SincTableManager();

    static SincTableManager&	STM();

    mExpClass(Algo) Table /*Table of sinc interpolator coefficients.*/
    {
	public:
			Table(int lsinc,int nsinc,float emax,float fmax,
			      int lmax);
			~Table()		{}

	bool		isOK() const	{ return asinc2_.get2DData(); }

	bool		hasSameDesign(float fmax,int lmax) const;
	float		getMaximumError() const		{ return emax_; }
	float		getMaximumFrequency() const	{ return fmax_; }
	int		getMaximumLength() const	{ return lmax_; }
	od_int64	getTableBytes() const;
	int		getLength() const;  // length of sinc approximations
	int		getNumbers() const; // number of sinc approximations
	int		getShift() const;
	inline void	setValue( int idx, int idy, float val )
			{ asinc2_.set( idx, idy, val ); }
	inline float	getValue( int idx, int idy ) const
			{ return asinc2_.get( idx, idy ); }

	protected:

	Array2DImpl<float>	asinc2_;	// array of sinc approximations

	private:

	float**		asinc_;  //will be removed
	float		emax_;
	float		fmax_;
	int		lmax_;
	int		nsinc_; //will be removed
	int		lsinc_; //will be removed

	friend class	SincInterpolator;
	friend class	RotatorExecutor;

    };

    const Table*	getTable(float fmax,int lmax);

protected:
    ObjectSet<const Table>	tables_;
    Threads::Mutex	lock_;
			//Protects tables_
    int			getTableIdx(float fmax,int lmax) const;

			// Builds a table based on design parameters.
    static const Table* makeTable(float fmax,int lmax);
    static float	sinc(float x);

			//will be removed after 6.0
    static float**	makeArray(int n1,int n2)	{ return 0; }
    static void		deleteArray(float**,int n1)	{}

};



/*!
\brief A sinc interpolator for bandlimited uniformly-sampled functions y(x).

 * Interpolators can be designed for any two of three parameters: maximum
 * error (emax), maximum frequency (fmax) and maximum length (lmax). The
 * parameter not specified is computed when an interpolator is designed.
 *
 * Below the specified (or computed) maximum frequency fmax, the maximum
 * interpolation error should be less than the specified (or computed)
 * maximum error emax. For frequencies above fmax, interpolation error
 * may be much greater. Therefore, sequences to be interpolated should
 * be bandlimited to frequencies less than fmax.
 *
 * The maximum length lmax of an interpolator is an even positive integer.
 * It is the number of uniform samples required to interpolate a single
 * value y(x). Ideally, the weights applied to each uniform sample are
 * values of a sinc function. Although the ideal sinc function yields zero
 * interpolation error for all frequencies up to the Nyquist frequency
 * (0.5 cycles/sample), it has infinite length.
 *
 * With recursive filtering, infinite-length approximations to the sinc
 * function are feasible and, in some applications, most efficient. When
 * the number of interpolated values is large relative to the number of
 * uniform samples, the cost of recursive filtering is amortized over those
 * many interpolated values, and can be negligible. However, this cost
 * becomes significant when only a few values are interpolated for each
 * sequence of uniform samples.
 *
 * This interpolator is based on a <em>finite-length</em> approximation
 * to the sinc function. The efficiency of finite-length interpolators
 * like this one does not depend on the number of samples interpolated.
 * Also, this interpolator is robust in the presence of noise spikes,
 * which affect only nearby samples.
 *
 * Finite-length interpolators present a tradeoff between cost and accuracy.
 * Interpolators with small maximum lengths are most efficient, and those
 * with high maximum frequencies and small maximum errors are most accurate.
 *
 * When interpolating multiple values of y(x) from a single sequence of
 * uniformly sampled values, efficiency may be improved by using one of the
 * methods that enables specification of multiple x values at which to
 * interpolate.
 *
 * author: Dave Hale, Colorado School of Mines
 * author: Bill Harlan, Landmark Graphics
 * version 2012.12.21
 */


mExpClass(Algo) SincInterpolator
{
public:
			~SincInterpolator();

			/*!\param fmax Maximum frequency in input dataset
			   \param lmax Maximum length of the interpolator*/
    virtual bool	initTable(float fmax,int lmax);
    inline bool		isTableOK() const;

    inline od_int64	getTableBytes() const;
    inline float	getMaximumError() const;
    inline float	getMaximumFrequency() const;
    inline int		getMaximumLength() const;

    enum Extrapolation	{ NONE=0, ZERO=1, CONSTANT=2 };
    Extrapolation	getExtrapolation();
    void		setExtrapolation(Extrapolation);

protected:
			SincInterpolator();
    bool		init() { return initTable( 0.3f, 8 ); }

    inline float	getTableVal(int idx,int idy) const;

    template <class RT>
    bool		initUndefTable(const RT*,od_int64 sz);

    bool*		isudfarr_;
    int			lsinc_;
    int			nsincm1_;
    int			ishift_;
    bool		extrapcst_;
    bool		extrapzero_;	// NOT used, will be removed after 6.0

    static const float	snapdist;
			//!< relative distance from a sample below which no
			//!< interpolation is done. 99.9% chance default is OK.

			/*Table of sinc interpolation coefficients.*/
    const SincTableManager::Table*		table_;
    const float**	asinc_;		// only a cached shortcut
};


template <class RT, class PT>
mClass(Algo) SincInterpolator1D : public SincInterpolator,
				  public MathFunction<RT,PT>
{
public:
			SincInterpolator1D(const RT* =0,int sz=-1);

    bool		setSize(int);
    void		setInput(const RT*,bool datahasnoudf=false);
    bool		isOK() const	{ return data_ && isTableOK(); }
    bool		initTable(float fmax,int lmax);

    RT			getValue(PT) const;

private:

    const RT*		data_;
    int			nx_;
    int			nxm_;
};


template <class RT, class PT>
mClass(Algo) SincInterpolator2D : public SincInterpolator,
				  public MathXYFunction<RT,PT>
{
public:
			SincInterpolator2D(const RT*,int nx,int ny);
    bool		isOK() const		{ return isTableOK(); }
    bool		initTable(float fmax,int lmax);

    RT			getValue(PT,PT) const;

private:

    const RT*		data_;
    int			nx_;
    int			ny_;
    int			nxm_;
    int			nym_;

};


template <class RT, class PT>
mClass(Algo) SincInterpolator3D : public SincInterpolator,
				  public MathXYZFunction<RT,PT>
{
public:
			SincInterpolator3D(const RT* data,int nx,int ny,int nz);
    bool		isOK() const		{ return isTableOK(); }
    bool		initTable(float fmax,int lmax);

    RT			getValue(PT,PT,PT) const;

private:

    const RT*		data_;
    int			nx_;
    int			ny_;
    int			nz_;
    int			nxm_;
    int			nym_;
    int			nzm_;

};



inline bool SincInterpolator::isTableOK() const
{ return table_; }

inline float SincInterpolator::getMaximumError() const
{ return table_->getMaximumError(); }

inline float SincInterpolator::getMaximumFrequency() const
{ return table_->getMaximumFrequency(); }

inline int SincInterpolator::getMaximumLength() const
{ return table_->getMaximumLength(); }

inline od_int64 SincInterpolator::getTableBytes() const
{ return table_->getTableBytes(); }

inline float SincInterpolator::getTableVal( int idx, int idy ) const
{ return table_ ? table_->getValue( idx, idy ) : mUdf(float); }


template <class RT>
bool SincInterpolator::initUndefTable( const RT* vals, od_int64 sz )
{
    deleteAndNullArrPtr( isudfarr_ );
    if ( !vals )
	return true;

    mTryAlloc( isudfarr_, bool[sz] );
    if ( !isudfarr_ )
	return false;

    bool hasudfs = false;
    for ( int idx=0; idx<sz; idx++ )
    {
	isudfarr_[idx] = mIsUdf(vals[idx]);
	if ( isudfarr_[idx] )
	    hasudfs = true;
    }

    if ( !hasudfs )
	deleteAndNullArrPtr( isudfarr_ );

    return true;
}



template <class RT, class PT>
SincInterpolator1D<RT,PT>::SincInterpolator1D( const RT* data, int nx )
    : SincInterpolator()
    , data_( 0 )
    , nx_(nx)
{
    init();
    setInput( data );
}


template <class RT, class PT>
void SincInterpolator1D<RT,PT>::setInput( const RT* data, bool datahasnoudf )
{
    data_ = data;
    if ( datahasnoudf )
	deleteAndNullArrPtr( isudfarr_ );
    else
	initUndefTable( data_, nx_ );
}


template <class RT, class PT>
bool SincInterpolator1D<RT,PT>::setSize( int nx )
{
    if ( nx < 1 )
	return false;

    nx_ = nx;
    return init();
}


template <class RT, class PT>
bool SincInterpolator1D<RT,PT>::initTable( float fmax, int lmax )
{
    if ( !SincInterpolator::initTable(fmax,lmax) )
	return false;

    nxm_ = nx_ - lsinc_;
    return true;
}

#define mKSinc(frac) ( mCast(int,frac*nsincm1_+0.5) )
#define mValidPos(is,ns)	( (is > -1 && is < ns) )
#define mCheckUdf(totidx) \
{ \
    if ( isudfarr_ && isudfarr_[totidx] ) \
	continue; \
}
#define mAddVal(val,weight,totidx,outval) \
{ \
    mCheckUdf(totidx) \
    outval += val * weight; \
}
#define mAddValW(val,totidx,weight,outval,sumweights) \
{ \
    mCheckUdf(totidx) \
    outval += val * weight; \
    sumweights += weight; \
}

template <class RT,class PT>
RT SincInterpolator1D<RT,PT>::getValue( PT x ) const
{
    int idx0 = mNINT32(x);
    PT fracx = x - idx0;
    if ( fracx > -snapdist && fracx < snapdist && mValidPos(idx0,nx_) &&
	 ( !isudfarr_ || (isudfarr_ && !isudfarr_[idx0]) ) )
	return data_[idx0];

    const PT floorx = floor(x);
    fracx = x - floorx;
    const float* asinc = asinc_[mKSinc(fracx)];
    idx0 = mCast(int,floorx) + ishift_;

    float out = 0.f;
    if ( mValidPos(idx0,nxm_) )
    {
	for ( int isinc=0,idx=idx0; isinc<lsinc_; isinc++,idx++ )
	    mAddVal(data_[idx],asinc[isinc],idx,out)
    }
    else if ( extrapcst_ )
    {
	for ( int isinc=0,idx=idx0; isinc<lsinc_; isinc++,idx++ )
	{
	    const int idx1 = idx < 0 ? 0 : idx >= nx_ ? nx_-1 : idx;
	    mAddVal(data_[idx1],asinc[isinc],idx,out)
	}
    }
    else
    {
	float sumweights = 0.f;
	for ( int isinc=0,idx=idx0; isinc<lsinc_; isinc++,idx++ )
	{
	    if ( !mValidPos(idx,nx_) )
		continue;

	    mAddValW(data_[idx],idx,asinc[isinc],out,sumweights)
	}
	if ( !mIsZero(sumweights,mDefEpsF) ) out /= sumweights;
    }

    return mCast(RT,out);
}



template <class RT, class PT>
SincInterpolator2D<RT,PT>::SincInterpolator2D( const RT* data, int nx, int ny )
    : SincInterpolator()
    , data_(data)
    , nx_(nx)
    , ny_(ny)
{
    init();
    initUndefTable( data_, nx_*ny_ );
}


template <class RT, class PT>
bool SincInterpolator2D<RT,PT>::initTable( float fmax, int lmax )
{
    if ( !SincInterpolator::initTable(fmax,lmax) )
	return false;

    nxm_ = nx_ - lsinc_;
    nym_ = ny_ - lsinc_;
    return true;
}


template <class RT, class PT>
RT SincInterpolator2D<RT,PT>::getValue( PT x, PT y ) const
{
    int idx0 =	mNINT32(x);
    int idy0 =	mNINT32(y);
    PT fracx = x - idx0;
    PT fracy = y - idy0;
    if ( fracx > -snapdist && fracx < snapdist && mValidPos(idx0,nx_) &&
	 fracy > -snapdist && fracy < snapdist && mValidPos(idy0,ny_) &&
	 ( !isudfarr_ || (isudfarr_ && !isudfarr_[idx0*ny_+idy0]) ) )
	return data_[idx0*ny_+idy0];

    const PT floorx = floor(x);
    const PT floory = floor(y);
    fracx = x - floorx;
    fracy = y - floory;
    const float* asincx = asinc_[mKSinc(fracx)];
    const float* asincy = asinc_[mKSinc(fracy)];
    idx0 = mCast(int,floorx) + ishift_;
    idy0 = mCast(int,floory) + ishift_;

    double out = 0., outx;
    if ( mValidPos(idx0,nxm_) && mValidPos(idy0,nym_) )
    {
	for ( int ixsinc=0,idx=idx0; ixsinc<lsinc_; ixsinc++,idx++ )
	{
	    outx = 0.;
	    const float asincxval = asincx[ixsinc];
	    if ( mIsZero(asincxval,mDefEpsF) )
		continue;

	    for ( int iysinc=0,idy=idy0; iysinc<lsinc_; iysinc++,idy++ )
	    {
		const od_int64 off = idx*ny_+idy;
		mAddVal(data_[off],asincy[iysinc],off,outx)
	    }
	    out += outx * asincxval;
	}
    }
    else if ( extrapcst_ )
    {
	for ( int ixsinc=0,idx=idx0; ixsinc<lsinc_; ixsinc++,idx++ )
	{
	    outx = 0.;
	    const float asincxval = asincx[ixsinc];
	    if ( mIsZero(asincxval,mDefEpsF) )
		continue;

	    const int idx1 = idx < 0 ? 0 : idx >= nx_ ? nx_-1 : idx;
	    for ( int iysinc=0,idy=idy0; iysinc<lsinc_; iysinc++,idy++ )
	    {
		const int idy1 = idy < 0 ? 0 : idy >= ny_ ? ny_-1 : idy;
		const od_int64 off = idx1*ny_+idy1;
		mAddVal(data_[off],asincy[iysinc],off,outx)
	    }
	    out += outx * asincxval;
	}
    }
    else
    {
	double sumweights = 0., sumx;
	for ( int ixsinc=0,idx=idx0; ixsinc<lsinc_; ixsinc++,idx++ )
	{
	    outx = 0.; sumx = 0.;
	    const float asincxval = asincx[ixsinc];
	    if ( mIsZero(asincxval,mDefEpsF) || !mValidPos(idx,nx_) )
		continue;

	    for ( int iysinc=0,idy=idy0; iysinc<lsinc_; iysinc++,idy++ )
	    {
		if ( !mValidPos(idy,ny_) )
		    continue;

		const od_int64 off = idx*ny_+idy;
		mAddValW(data_[off],off,asincy[iysinc],outx,sumx)
	    }
	    out += outx * asincxval;
	    sumweights += sumx;
	}
	if ( !mIsZero(sumweights,mDefEps) ) out /= sumweights;
    }

    return mCast(RT,out);
}



template <class RT, class PT>
SincInterpolator3D<RT,PT>::SincInterpolator3D( const RT* data, int nx, int ny,
					       int nz )
    : SincInterpolator()
    , data_(data)
    , nx_(nx)
    , ny_(ny)
    , nz_(nz)
{
    init();
    initUndefTable( data_, nx_*ny_*nz_ );
}


template <class RT, class PT>
bool SincInterpolator3D<RT,PT>::initTable( float fmax, int lmax )
{
    if ( !SincInterpolator::initTable(fmax,lmax) )
	return false;

    nxm_ = nx_ - lsinc_;
    nym_ = ny_ - lsinc_;
    nzm_ = nz_ - lsinc_;
    return true;
}


#undef mGetOffset
#define mGetOffset(idx,idy,idz) ( idz + nz_*( idy + ny_*idx ) )
template <class RT, class PT>
RT SincInterpolator3D<RT,PT>::getValue( PT x, PT y, PT z ) const
{
    int idx0 =	mNINT32(x);
    int idy0 =	mNINT32(y);
    int idz0 =	mNINT32(z);
    PT fracx = x - idx0;
    PT fracy = y - idy0;
    PT fracz = z - idz0;
    if ( fracx > -snapdist && fracx < snapdist && mValidPos(idx0,nx_) &&
	 fracy > -snapdist && fracy < snapdist && mValidPos(idy0,ny_) &&
	 fracz > -snapdist && fracz < snapdist && mValidPos(idz0,nz_) &&
	 ( !isudfarr_ || (isudfarr_ && !isudfarr_[mGetOffset(idx0,idy0,idz0)])))
	return data_[mGetOffset(idx0,idy0,idz0)];

    const PT floorx = floor(x);
    const PT floory = floor(y);
    const PT floorz = floor(z);
    fracx = x - floorx;
    fracy = y - floory;
    fracz = z - floorz;
    const float* asincx = asinc_[mKSinc(fracx)];
    const float* asincy = asinc_[mKSinc(fracy)];
    const float* asincz = asinc_[mKSinc(fracz)];
    idx0 = mCast(int,floorx) + ishift_;
    idy0 = mCast(int,floory) + ishift_;
    idz0 = mCast(int,floorz) + ishift_;

    double out = 0.;
    double outx, outy;
    if ( mValidPos(idx0,nxm_) && mValidPos(idy0,nym_) && mValidPos(idz0,nzm_) )
    {
	for ( int ixsinc=0,idx=idx0; ixsinc<lsinc_; ixsinc++,idx++ )
	{
	    outx = 0.;
	    const float asincxval = asincx[ixsinc];
	    if ( mIsZero(asincxval,mDefEpsF) )
		continue;

	    for ( int iysinc=0,idy=idy0; iysinc<lsinc_; iysinc++,idy++ )
	    {
		outy = 0.;
		const float asincyval = asincy[iysinc];
		if ( mIsZero(asincyval,mDefEpsF) )
		    continue;

		for ( int izsinc=0,idz=idz0; izsinc<lsinc_; izsinc++,idz++ )
		{
		    const od_int64 off = mGetOffset(idx,idy,idz);
		    mAddVal(data_[off],asincz[izsinc],off,outy)
		}
		outx += outy * asincyval;
	    }
	    out += asincxval * outx;
	}
    }
    else if ( extrapcst_ )
    {
	for ( int ixsinc=0,idx=idx0; ixsinc<lsinc_; ixsinc++,idx++ )
	{
	    outx = 0.;
	    const float asincxval = asincx[ixsinc];
	    if ( mIsZero(asincxval,mDefEpsF) )
		continue;

	    const int idx1 = idx < 0 ? 0 : idx >= nx_ ? nx_-1 : idx;
	    for ( int iysinc=0,idy=idy0; iysinc<lsinc_; iysinc++,idy++ )
	    {
		outy = 0.;
		const float asincyval = asincy[iysinc];
		if ( mIsZero(asincyval,mDefEpsF) )
		    continue;

		const int idy1 = idy < 0 ? 0 : idy >= ny_ ? ny_-1 : idy;
		for ( int izsinc=0,idz=idz0; izsinc<lsinc_; izsinc++,idz++ )
		{
		    const int idz1 = idz < 0 ? 0 : idz >= nz_ ? nz_-1 : idz;
		    const od_int64 off = mGetOffset(idx1,idy1,idz1);
		    mAddVal(data_[off],asincz[izsinc],off,outy)
		}
		outx += outy * asincyval;
	    }
	    out += asincxval * outx;
	}
    }
    else
    {
	double sumweights = 0., sumx, sumy;
	for ( int ixsinc=0,idx=idx0; ixsinc<lsinc_; ixsinc++,idx++ )
	{
	    outx = 0.; sumx = 0.;
	    const float asincxval = asincx[ixsinc];
	    if ( mIsZero(asincxval,mDefEpsF) || !mValidPos(idx,nx_) )
		continue;

	    for ( int iysinc=0,idy=idy0; iysinc<lsinc_; iysinc++,idy++ )
	    {
		outy = 0.; sumy = 0.;
		const float asincyval = asincy[iysinc];
		if ( mIsZero(asincyval,mDefEpsF) || !mValidPos(idy,ny_) )
		    continue;

		for ( int izsinc=0,idz=idz0; izsinc<lsinc_; izsinc++,idz++ )
		{
		    if ( !mValidPos(idz,nz_) ) continue;
		    const od_int64 off = mGetOffset(idx,idy,idz);
		    mAddValW(data_[off],off,asincz[izsinc],outy,sumy)
		}
		outx += outy * asincyval;
		sumx += sumy;
	    }
	    out += asincxval * outx;
	    sumweights += sumx;
	}
	if ( !mIsZero(sumweights,mDefEps) ) out /= sumweights;
    }

    return mCast(RT,out);
}
