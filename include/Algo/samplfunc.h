#ifndef samplfunc_h
#define samplfunc_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: samplfunc.h,v 1.7 2003-11-07 12:21:50 bert Exp $
________________________________________________________________________

-*/

#include <mathfunc.h>
#include <periodicvalue.h>

/*!\brief make any sampled series comply with MathFunction.
If the sampled values are periodic (i.e. phase), set the periodic flag and let
period() return the period ( i.e. 2*pi for phase ).
*/

template <class RT,class T>
class SampledFunction : public MathFunction<RT>
{
public:
				SampledFunction( bool periodic_= false )
				    : periodic( periodic_ ) {}

    virtual RT			operator[](int)	const			= 0;

    virtual float		getDx() const				= 0;
    virtual float		getX0() const				= 0;

    virtual int			size() const				= 0;

    virtual float		period() const { return mUndefValue; } 
    void			setPeriodic( bool np ) { periodic = np; } 

    float			getIndex(float x) const
				    { return (x-getX0()) / getDx(); }

    int				getNearestIndex(float x) const
				    { return mNINT(getIndex( x )); }

    RT				getValue(double x) const
				{ 
				    return periodic 
					? interpolateYPeriodicSampled( *this,
							       size(),
							       getIndex(x),
							       period(),
							       extrapolate(),
							       getUndefVal())  
					: interpolateSampled( *this, size(), 
				     			       getIndex(x),
							       extrapolate(),
							       getUndefVal());
				}

protected:
    bool			periodic;


    virtual bool		extrapolate() const { return false; }
    virtual RT			getUndefVal() const { return mUndefValue; }

};


/*!\brief implementation for array-type of SampledFunction */

template <class RT, class T>
class SampledFunctionImpl : public SampledFunction<RT,T>
{
public:
			SampledFunctionImpl( const T& idxabl_, int sz_,
			    float x0_=0, float dx_=1 )
			    : idxabl( idxabl_ )
			    , sz( sz_ )
			    , x0( x0_ )
			    , dx( dx_ )
			    , period_ ( mUndefValue )
			{}

    RT			operator[](int idx) const { return idxabl[idx]; }

    float		getDx() const { return dx; }
    float		getX0() const { return x0; }

    int			size() const { return sz; }

    float		period() const { return period_; }
    void		setPeriod( float np ) { period_ = np; }


protected:

    const T&		idxabl;
    int			sz;
    int			firstidx;

    float		dx;
    float		x0;

    float		period_;
};

#endif
