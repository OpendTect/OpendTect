#ifndef samplfunc_h
#define samplfunc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id$
________________________________________________________________________

-*/

#include "mathfunc.h"
#include "periodicvalue.h"

/*!\brief make any sampled series comply with MathFunction.
If the sampled values are periodic (i.e. phase), set the periodic flag and let
period() return the period ( i.e. 2*pi for phase ).
*/

template <class RT,class T>
class SampledFunction : public MathFunction<RT,RT>
{
public:
				SampledFunction( bool periodic_= false )
				    : periodic( periodic_ ) {}

    virtual RT			operator[](od_int64)	const		= 0;

    virtual float		getDx() const				= 0;
    virtual float		getX0() const				= 0;

    virtual int			size() const				= 0;

    virtual float		period() const { return mUdf(float); } 
    void			setPeriodic( bool np ) { periodic = np; } 

    float			getIndex(float x) const
				    { return (x-getX0()) / getDx(); }

    int				getNearestIndex(float x) const
				    { return mNINT32(getIndex( x )); }

    RT				getValue( RT x ) const
				{ 
				    if ( !doInterpolate() )
				    {
					const int smpl = mNINT32( getIndex(x) );
					if ( smpl<0 || smpl>=size() )
					    return mUdf(RT);
					return (*this)[smpl];
				    }

				    if ( periodic )
				    {
					return 
					    IdxAble::interpolateYPeriodicReg(
						    *this, size(),
						    getIndex(x), period(),
						    extrapolate());
				    }

				    return hasUdfs() 
					? IdxAble::interpolateRegWithUdf( *this,
						size(), getIndex(x),
						extrapolate())
					: IdxAble::interpolateReg( *this,
						size(), getIndex(x),
						extrapolate());
				}

    RT				getValue( const RT* x ) const
				{ return getValue(*x); }

protected:
    bool			periodic;


    virtual bool		extrapolate() const { return false; }
    virtual bool		hasUdfs() const { return false; }
    virtual bool		doInterpolate() const { return true; }
};


/*!\brief implementation for array-type of SampledFunction */

template <class RT, class T>
class SampledFunctionImpl : public SampledFunction<RT,T>
{
public:
			SampledFunctionImpl(const T& idxabl,int sz,
			    float x0=0,float dx=1 )
			    : idxabl_( idxabl )
			    , sz_( sz )
			    , x0_( x0 )
			    , dx_( dx )
			    , period_ ( mUdf(float) )
			    , hasudfs_( false )
			    , interpolate_( true )
			{}

    RT			operator[](od_int64 idx) const	{ return idxabl_[idx];}

    float		getDx() const			{ return dx_; }
    float		getX0() const			{ return x0_; }

    int			size() const			{ return sz_; }

    float		period() const			{ return period_; }
    void		setPeriod(float np)		{ period_ = np; }

    bool		hasUdfs() const			{ return hasudfs_; }
    void		setHasUdfs(bool yn)		{ hasudfs_=yn; }

    bool		doInterpolate() const		{ return interpolate_; }
    void		setInterpolate( bool yn ) 	{ interpolate_=yn; }

protected:

    const T&		idxabl_;
    int			sz_;
    int			firstidx_;

    float		dx_;
    float		x0_;

    float		period_;
    bool		hasudfs_;
    bool		interpolate_;
};

#endif
