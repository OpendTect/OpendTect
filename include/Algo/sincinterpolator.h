#ifndef sincinterpolator_h
#define sincinterpolator_h

#include "algomod.h"
#include "arrayndimpl.h"
#include "mathfunc.h"

class KaiserWindow;

/*!
\brief A manager used for constructing the table necessary for Sinc
 interpolations.
 The manager creates one table per design and provides its as necessary
 A table contains shifted, stretched KaiserWindow functions in both directions
*/

mClass(Algo) SincTableManager
{
public:
			SincTableManager()	{};

    static SincTableManager&	STM();

    struct Table /*Table of sinc interpolator coefficients.*/
    {
			Table(int lsinc,int nsinc,float emax,float fmax,
			      int lmax);

	bool		hasSameDesign(float fmax,int lmax) const;
	float		getMaximumError() const		{ return emax_; }
	float		getMaximumFrequency() const	{ return fmax_; }
	int		getMaximumLength() const	{ return lmax_; }
	long		getTableBytes() const;
	int		getLength() const;  // length of sinc approximations
	int		getNumbers() const; // number of sinc approximations
	int		getShift() const;
	inline float	get( int idx, int idy ) const
			{ return asinc_.get(idx,idy); }

	Array2DImpl<float>	asinc_;  // array of sinc approximations
	float		emax_;
	float		fmax_;
	int		lmax_;
    };

    const Table*	getTable(float fmax,int lmax);

protected:
    ObjectSet<const Table>	tables_;

			// Builds a table based on design parameters.
    void		makeTable(float fmax,int lmax);

			/*Builds a table of interpolators for a specified
			  Kaiser window.*/
    const Table*	makeTable(const KaiserWindow&,int nsinc,
				  float emax,float fmax,int lmax);
    int			getTableIdx(float fmax,int lmax) const;

    static float	sinc(float x);
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


mClass(Algo) SincInterpolator
{
public:

			/*!<\param fmax Maximum frequency in input dataset
			    \param lmax Maximum length of the interpolator*/
    virtual bool	initTable(float fmax,int lmax);

    inline long		getTableBytes() const;
    inline float	getMaximumError() const;
    inline float	getMaximumFrequency() const;
    inline int		getMaximumLength() const;

    enum Extrapolation	{ ZERO=0, CONSTANT=1 };
    Extrapolation	getExtrapolation()	{ return extrap_; }
    void		setExtrapolation(Extrapolation et) { extrap_ = et; }

protected:
			SincInterpolator();
    bool		init() { return initTable( 0.3f, 8 ); }
    inline bool		isTableOK() const;

    inline float	getTableVal(int idx,int idy) const;

    Extrapolation	extrap_;
    int			lsinc_;
    int			ishift_;

			/*Table of sinc interpolation coefficients.*/
    const SincTableManager::Table*		table_;
};


template <class RT, class PT>
mClass(Algo) SincInterpolator1D : public SincInterpolator,
				  public MathFunction<RT,PT>
{
public:
				SincInterpolator1D(const RT* = 0,int sz=-1);
    bool			setSize(int);
    void			setInput(const RT*);
    bool			isOK() const	{ return data_ && isTableOK(); }
    bool			initTable(float fmax,int lmax);

    RT				getValue(PT) const;

private:

    const RT*			data_;
    int				nx_;
    int				nxm_;
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

inline long SincInterpolator::getTableBytes() const
{ return table_->getTableBytes(); }

inline float SincInterpolator::getTableVal( int idx, int idy ) const
{ return table_ ? table_->get( idx, idy ) : mUdf(float); }



template <class RT, class PT>
SincInterpolator1D<RT,PT>::SincInterpolator1D( const RT* data, int nx )
    : SincInterpolator()
    , data_(data)
    , nx_(nx)
{
    init();
}

template <class RT, class PT>
void SincInterpolator1D<RT,PT>::setInput( const RT* data )
{
    data_ = data;
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


#define mValidPos(idx,ns)	( (idx > -1 && idx < ns) )
#define mKSinc(frac) ( mCast(int,frac*(table_->getNumbers()-1)+0.5) )
#define mValidIdx(idxin,idxout,ns) \
{ \
    idxout = idxin; \
    if ( needextrapol && !mValidPos(idxout,ns) ) \
    { \
	if ( extrap_ == ZERO ) \
	    continue; \
	else if ( extrap_ == CONSTANT ) \
	    idxout = idxout < 0 ? 0 : ns-1; \
    } \
}
#define mAddVal(val,weight,outval) \
{ \
    if ( mIsUdf(val) ) \
	continue; \
\
    outval += val * weight; \
}

template <class RT,class PT>
RT SincInterpolator1D<RT,PT>::getValue( PT x ) const
{
    const double xn = (double)x + (double)lsinc_;
    const int xnn = mNINT32(xn);
    const double xnnd = mCast(double,xnn);
    if ( mIsEqual(xn,xnnd,xn*1e-6) )
    {
	const int idx = xnn - lsinc_;
	if ( mValidPos(idx,nx_) && !mIsUdf(data_[idx]) )
	    return data_[idx];
    }

    const int ixn = (int)xn;
    int idx = ishift_ + ixn;

    double frac = xn-ixn;
    if ( frac < 0. )
	frac += 1.;

    const int ksinc = mKSinc(frac);

    const bool needextrapol = !mValidPos(idx,nxm_);
    double out = 0.;
    int idx3;
    for ( int isinc=0,idx2=idx; isinc<lsinc_; isinc++,idx2++ )
    {
	const float asincx = getTableVal( isinc, ksinc );
	if ( mIsZero(asincx,mDefEpsF) ) continue;
	mValidIdx(idx2,idx3,nx_);
	mAddVal(data_[idx3],asincx,out)
    }

    return (RT)out;
}



template <class RT, class PT>
SincInterpolator2D<RT,PT>::SincInterpolator2D( const RT* data, int nx, int ny )
    : SincInterpolator()
    , data_(data)
    , nx_(nx)
    , ny_(ny)
{
    init();
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
    const double xn = (double)x + (double)lsinc_;
    const double yn = (double)y + (double)lsinc_;
    const int xnn = mNINT32(xn);
    const int ynn = mNINT32(yn);
    const double xnnd = mCast(double,xnn);
    const double ynnd = mCast(double,ynn);
    if ( mIsEqual(xn,xnnd,xn*1e-6) && mIsEqual(yn,ynnd,yn*1e-6) )
    {
	const int idx = xnn - lsinc_;
	const int idy = ynn - lsinc_;
	if ( mValidPos(idx,nx_) && mValidPos(idy,ny_) &&
	     !mIsUdf(data_[idx*ny_+idy]) )
	{
	    return data_[idx*ny_+idy];
	}
    }

    const int ixn = (int)xn;
    const int iyn = (int)yn;
    int idx = ishift_ + ixn;
    int idy = ishift_ + iyn;

    double fracx = xn-ixn;
    double fracy = yn-iyn;
    if ( fracx < 0. )
	fracx += 1.;
    if ( fracy < 0. )
	fracy += 1.;

    const int ksincx = mKSinc(fracx);
    const int ksincy = mKSinc(fracy);

    const bool needextrapol = !mValidPos(idx,nxm_) || !mValidPos(idy,nym_);
    double out = 0.;
    double outx;
    int idx3, idy3;
    for ( int ixsinc=0,idx2=idx; ixsinc<lsinc_; ixsinc++,idx2++ )
    {
	outx = 0.;
	const float asincx = getTableVal( ixsinc, ksincx );
	if ( mIsZero(asincx,mDefEpsF) ) continue;
	mValidIdx(idx2,idx3,nx_);
	for ( int iysinc=0,idy2=idy; iysinc<lsinc_; iysinc++,idy2++ )
	{
	    const float asincy = getTableVal( iysinc, ksincy );
	    if ( mIsZero(asincy,mDefEpsF) ) continue;
	    mValidIdx(idy2,idy3,ny_);
	    mAddVal(data_[idx3*ny_+idy3],asincy,outx)
	}
	out += outx * asincx;
    }

    return (RT)out;
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
    const double xn = (double)x + (double)lsinc_;
    const double yn = (double)y + (double)lsinc_;
    const double zn = (double)z + (double)lsinc_;
    const int xnn = mNINT32(xn);
    const int ynn = mNINT32(yn);
    const int znn = mNINT32(zn);
    const double xnnd = mCast(double,xnn);
    const double ynnd = mCast(double,ynn);
    const double znnd = mCast(double,znn);
    if ( mIsEqual(xn,xnnd,xn*1e-6) && mIsEqual(yn,ynnd,yn*1e-6) &&
	 mIsEqual(zn,znnd,zn*1e-6) )
    { //Exactly on a sample
	const int idx = xnn - lsinc_;
	const int idy = ynn - lsinc_;
	const int idz = znn - lsinc_;
	if ( mValidPos(idx,nx_) && mValidPos(idy,ny_) &&
	     mValidPos(idz,nz_) && !mIsUdf(data_[mGetOffset(idx,idy,idz)]) )
	    return data_[mGetOffset(idx,idy,idz)];
    }

    const int ixn = (int)xn;
    const int iyn = (int)yn;
    const int izn = (int)zn;
    int idx = ishift_ + ixn;
    int idy = ishift_ + iyn;
    int idz = ishift_ + izn;

    double fracx = xn-ixn;
    double fracy = yn-iyn;
    double fracz = zn-izn;
    if ( fracx<0. )
	fracx += 1.;
    if ( fracy<0. )
	fracy += 1.;
    if ( fracz<0. )
      fracz += 1.;

    const int ksincx = mKSinc(fracx);
    const int ksincy = mKSinc(fracy);
    const int ksincz = mKSinc(fracz);

    const bool needextrapol = !mValidPos(idx,nxm_) || !mValidPos(idy,nym_) ||
			      !mValidPos(idz,nzm_);
    double out = 0.;
    double outx, outy;
    int idx3, idy3, idz3;
    for ( int ixsinc=0,idx2=idx; ixsinc<lsinc_; ixsinc++,idx2++ )
    {
	outx = 0.;
	const float asincx = getTableVal( ixsinc, ksincx );
	if ( mIsZero(asincx,mDefEpsF) ) continue;
	mValidIdx(idx2,idx3,nx_);
	for ( int iysinc=0,idy2=idy; iysinc<lsinc_; iysinc++,idy2++ )
	{
	    outy = 0.;
	    const float asincy = getTableVal( iysinc, ksincy );
	    if ( mIsZero(asincy,mDefEpsF) ) continue;
	    mValidIdx(idy2,idy3,ny_);
	    for ( int izsinc=0,idz2=idz; izsinc<lsinc_; izsinc++,idz2++ )
	    {
		const float asincz = getTableVal( izsinc, ksincz );
		if ( mIsZero(asincz,mDefEpsF) ) continue;
		mValidIdx(idz2,idz3,nz_);
		mAddVal(data_[mGetOffset(idx3,idy3,idz3)],asincz,outy)
	    }

	    outx += outy * asincy;
	}
	out += asincx * outx;
    }

    return (RT)out;
}

#endif
