#ifndef mathfunc_H
#define mathfunc_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		17-11-1999
 Contents:	Mathematical Functions
 RCS:		$Id: mathfunc.h,v 1.9 2004-10-06 10:08:01 nanne Exp $
________________________________________________________________________

-*/


#include "position.h"

#include <math.h>

/*!\brief Multidimensional Mathematical function

A MathFunctionND must deliver a value at any position: F(x*).
The positioning needs more precision than the outcome, hence
the doubles in the position.

*/


template <class RT>
class MathFunctionND
{
public:
    virtual	~MathFunctionND() {}
    template <class IDXABL>
    RT		getValue( const IDXABL& x ) const
    		{
		    const int nrdim = getNrDim();
		    double pos[nrdim];
		    for ( int idx=0; idx<nrdim; idx++ )
			pos[idx] = x[idx];
		    return getValue( pos );
		}

    virtual RT	getValue( const double* x) const= 0;
    virtual int	getNrDim() const 		= 0;
};


/*!\brief Mathematical function

A MathFunction must deliver a value at any position: F(x).
The positioning needs more precision than the outcome, hence
the doubles in the position.

*/

template <class RT>
class MathFunction : public MathFunctionND<RT>
{
public:

    virtual RT	getValue(double) const	= 0;

    RT		getValue( const double* pos ) const { return getValue(pos[0]);}
    int		getNrDim() const { return 1; }
};


/*!\brief a Math Function as in F(x,y). */

template <class RT>
class MathXYFunction : public MathFunctionND<RT>
{
public:

    virtual RT	getValue(const Coord&) const	= 0;

    RT		getValue( const double* pos ) const
    		        { return getValue(Coord(pos[0],pos[1]));}
    int		getNrDim() const { return 2; }

};


/*!\brief a Math Function as in F(x,y,z). */
template <class RT>
class MathXYZFunction : public MathFunctionND<RT>
{
public:

    virtual RT	getValue(const Coord3&) const	= 0;

    RT		getValue( const double* pos ) const
    		        { return getValue(Coord3(pos[0],pos[1],pos[2]));}
    int		getNrDim() const { return 3; }

};


/*!\brief Line parameters: steepness and intercept. */

class LinePars : public MathFunction<float>
{
public:
		LinePars( double i0=0, double i1=0 )
		: a0(i0), ax(i1)		{}
 
    float	getValue( double x ) const
		{ return (float)(a0 + ax * x); }
 
    double      a0, ax;
};


/*!\brief Palne parameters: steepnesses and intercept. */

class PlanePars : public MathXYFunction<float>
{
public:
		PlanePars( double i0=0, double i1=0, double i2=0 )
		: a0(i0), ax(i1), ay(i2)	{}

    float	getValue( const Coord& c ) const
		{ return (float)(a0 + ax * c.x + ay * c.y); }

    double	a0, ax, ay;

};


/*! \brief A MathFunction that cuts through another mathfunctioin with
           higher number of dimensions.

A starting point (P) and a vector (N) is used to project a line through
a MathFunctionND (func). The value returned is:

f(x) = func(P+N*x)
*/

template <class RT>
class AlongVectorFunction : public MathFunction<RT>
{
public:
    			AlongVectorFunction( const MathFunctionND<RT>& func_,
					     const double* P_, const double* N_)
			    : P( P_ )
			    , N( N_ )
			    , func( func_ )
			{}

    RT			getValue( double lambda ) const
			{
			    const int ndim = func.getNrDim();
			    double pos[ndim];
			    for ( int idx=0; idx<ndim; idx++ )
				pos[idx] = P[idx]+N[idx]*lambda;

			    return func.getValue( pos );
			}
protected:

    const double*		P;
    const double*		N;
    const MathFunctionND<RT>&	func;
};


/*!\brief Linear statistics.

Linear statistics have Line parameters for the regression line, the errors
and a correlation coefficient. There is no 2D variant yet.

*/

class LinStats
{
public:
		LinStats() : corrcoeff(0)	{}

    LinePars	lp;		// Parameters
    LinePars	sd;		// Standard deviations in parameters
    double	corrcoeff;	// Correlation coefficient
};


/*! A class for 3rd order polynomials on the form:

    a x^3 + b x^2 + c x + d
*/

class ThirdOrderPoly : public MathFunction<float>
{
public:
    			ThirdOrderPoly(float y0_,float y1_,float y2_,float y3_)
			{
			    b = ( (y2_+y0_) / 2 ) - y1_;
			    c = y2_ - ( ( 2*y0_ + 3*y1_ + y3_ ) / 6 );
			    a = ( (y2_-y0_) / 2 ) - c;
			    d = y1_;
			}

    float		getValue( double pos ) const
			{
			    if ( mIsUndefined(pos) ) return mUndefValue;
			    const float possq = pos * pos;
			    return possq * pos * a + possq * b + pos * c + d;
			}

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


/*! A class for 2nd order polynomials on the form:

    a x^2 + b x + c
*/

class SecondOrderPoly : public MathFunction<float>
{
public:
    			SecondOrderPoly(float y0_,float y1_,float y2_)
			{
			    c = y1_;
			    a = ( (y0_+y2_) / 2 ) - y1_;
			    b = ( y2_-y0_ ) / 2;
			}

    float		getValue( double pos ) const
			{
			    if ( mIsUndefined(pos) ) return mUndefValue;
			    return pos *pos * a + pos * b + c;
			}

    float		getExtremePos() const
			{
			    if ( mIsZero(a,mDefEps) ) return mUndefValue;
			    return -b / (2*a);
			}

    float		a, b, c;
};


#endif
