#ifndef mathfunc_H
#define mathfunc_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		17-11-1999
 Contents:	Mathematical Functions
 RCS:		$Id: mathfunc.h,v 1.12 2005-01-28 13:31:16 bert Exp $
________________________________________________________________________

-*/


#include "position.h"
#include "samplingdata.h"
#include <math.h>
class LinePars;


/*!\brief 2D data point. */
template <class T>
struct DataPoint2D	{ T x, y; };

/*!\brief 3D data point. */
template <class T>
struct DataPoint3D	{ T x, y, z; };


/*!\brief Multidimensional Mathematical function

A MathFunctionND must deliver a value at any position: F(x*).
The positioning may need a different precision than the outcome, hence
the two types.

*/

template <class RT,class PT>
class MathFunctionND
{
public:
    virtual	~MathFunctionND() {}
    template <class IDXABL>
    RT		getValue( const IDXABL& x ) const
    		{
		    const int nrdim = getNrDim();
		    PT pos[nrdim];
		    for ( int idx=0; idx<nrdim; idx++ )
			pos[idx] = x[idx];
		    return getValue( pos );
		}

    virtual RT	getValue( const PT* x) const	= 0;
    virtual int	getNrDim() const 		= 0;
};

typedef MathFunctionND<float,float> FloatMathFunctionND;


/*!\brief Mathematical function

A MathFunction must deliver a value at any position: F(x).
The positioning may need a different precision than the outcome, hence
the two types.

*/

template <class RT,class PT>
class MathFunction : public MathFunctionND<RT,PT>
{
public:

    virtual RT	getValue(PT) const	= 0;

    RT		getValue( const PT* pos ) const { return getValue(pos[0]);}
    int		getNrDim() const { return 1; }
};

typedef MathFunction<float,float> FloatMathFunction;


/*! \brief Makes a MathFunction indexable through an operator[].
*/

template <class RT,class PT>
class MathFunctionSampler
{
public:
			MathFunctionSampler( const MathFunction<RT,PT>& f )
			    : func( f )
			{}
    RT			operator[](int idx) const
			{ return func.getValue( sd.atIndex(idx) ); }

    SamplingData<PT>	sd;

protected:

    const MathFunction<RT,PT>&	func;

};


/*!\brief a Math Function as in F(x,y). */

template <class RT,class PT>
class MathXYFunction : public MathFunctionND<RT,PT>
{
public:

    virtual RT	getValue(PT,PT) const		= 0;

    RT		getValue( const PT* pos ) const
    		        { return getValue(pos[0],pos[1]);}
    int		getNrDim() const { return 2; }

};


/*!\brief a Math Function as in F(x,y,z). */
template <class RT,class PT>
class MathXYZFunction : public MathFunctionND<RT,PT>
{
public:

    virtual RT	getValue(PT,PT,PT) const	= 0;

    RT		getValue( const PT* pos ) const
    		        { return getValue(pos[0],pos[1],pos[2]);}
    int		getNrDim() const { return 3; }

};


/*! \brief A MathFunction that cuts through another mathfunctioin with
           higher number of dimensions.

A starting point (P) and a vector (N) is used to project a line through
a MathFunctionND (func). The value returned is:

f(x) = func(P+N*x)
*/

template <class RT,class PT>
class AlongVectorFunction : public MathFunction<RT,PT>
{
public:
    			AlongVectorFunction( const MathFunctionND<RT,PT>& func_,
					     const PT* P_, const PT* N_)
			    : P( P_ )
			    , N( N_ )
			    , func( func_ )
			{}

    RT			getValue( PT lambda ) const
			{
			    const int ndim = func.getNrDim();
			    PT pos[ndim];
			    for ( int idx=0; idx<ndim; idx++ )
				pos[idx] = P[idx]+N[idx]*lambda;

			    return func.getValue( pos );
			}
protected:

    const PT*				P;
    const PT*				N;
    const MathFunctionND<RT,PT>&	func;
};


/*! A class for 2nd order polynomials on the form:

    a x^2 + b x + c
*/

class SecondOrderPoly : public FloatMathFunction
{
public:
    			SecondOrderPoly( float a_=0, float b_=0, float c_=0 )
			    : a( a_ ), b( b_ ), c( c_ )
			{}

			/*!Fits the polynomial through the points. The points
			   are considered to be sampled with y0 at x=-1 and
			   y2 at x=1
		        */
    void		setFromSamples(float y0,float y1,float y2)
			{
			    c = y1;
			    a = ( (y0+y2) / 2 ) - y1;
			    b = ( y2-y0 ) / 2;
			}

    float		getValue( float pos ) const
			{
			    if ( mIsUndefined(pos) ) return mUndefValue;
			    return pos*pos * a + pos * b + c;
			}

    float		getExtremePos() const
			{
			    if ( mIsZero(a,mDefEps) ) return mUndefValue;
			    return -b / (2*a);
			}

    LinePars*		createDerivative() const;

    float		a, b, c;
};


/*! A class for 3rd order polynomials on the form:

    a x^3 + b x^2 + c x + d
*/

class ThirdOrderPoly : public FloatMathFunction
{
public:
    			ThirdOrderPoly( float a_=0, float b_=0,
					float c_=0, float d_=0 )
			    : a( a_ ), b( b_ ), c( c_ ), d( d_ )
			{}

			/*!Fits the polynomial through the points. The points
			   are considered to be sampled with y0 at x=-1 and
			   y3 at x=2
		        */
    void		setFromSamples(float y0,float y1,float y2,float y3)
			{
			    b = ( (y2+y0) / 2 ) - y1;
			    c = y2 - ( ( 2*y0 + 3*y1 + y3 ) / 6 );
			    a = ( (y2-y0) / 2 ) - c;
			    d = y1;
			}

    float		getValue( float pos ) const
			{
			    if ( mIsUndefined(pos) ) return mUndefValue;
			    const float possq = pos * pos;
			    return possq * pos * a + possq * b + pos * c + d;
			}

    SecondOrderPoly*	createDerivative() const
			{ return new SecondOrderPoly( a*3, b*2, c ); }

    void		getExtremePos( float& pos0, float& pos1 ) const
			{
			    pos0 = pos1 = mUndefValue;
			    if ( mIsZero(a,mDefEps) && mIsZero(b,mDefEps) )
				return;
			    else if ( mIsZero(a,mDefEps) )
			    {
				pos0 = -c / ( 2*b );
				return;
			    }

			    const float det = 4*b*b - 12*a*c;
			    if ( det < 0 ) return;
			    pos0 = ( -2*b + sqrt(det) ) / ( 6*a );
			    if ( !mIsZero(det,mDefEps) )
				pos1 = ( -2*b - sqrt(det) ) / ( 6*a );
			}

    float		a, b, c, d;
};


#endif
