#ifndef mathfunc_H
#define mathfunc_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		17-11-1999
 Contents:	Mathematical Functions
 RCS:		$Id: mathfunc.h,v 1.6 2003-05-13 13:42:23 kristofer Exp $
________________________________________________________________________

-*/


#include <position.h>

/*!\brief Multidimensional Mathematical function

A MathFunctionND must deliver a value at any position: F(x*).
The positioning needs more precision than the outcome, hence
the doubles in the position.

*/


template <class T>
class MathFunctionND
{
public:
    virtual	~MathFunctionND() {}
    virtual T	getValue( const double* x) const= 0;
    virtual int	getNrDim() const 		= 0;
};


/*!\brief Mathematical function

A MathFunction must deliver a value at any position: F(x).
The positioning needs more precision than the outcome, hence
the doubles in the position.

*/

template <class T>
class MathFunction : public MathFunctionND<T>
{
public:

    virtual T	getValue(double) const	= 0;

    T		getValue( const double* pos ) const { return getValue(pos[0]);}
    int		getNrDim() const { return 1; }
};


/*!\brief a Math Function as in F(x,y). */

template <class T>
class MathXYFunction : public MathFunctionND<T>
{
public:

    virtual T	getValue(const Coord&) const	= 0;

    T		getValue( const double* pos ) const
    		        { return getValue(Coord(pos[0],pos[1]));}
    int		getNrDim() const { return 2; }

};


/*!\brief a Math Function as in F(x,y,z). */
template <class T>
class MathXYZFunction : public MathFunctionND<T>
{
public:

    virtual T	getValue(const Coord3&) const	= 0;

    T		getValue( const double* pos ) const
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

template <class T>
class AlongVectorFunction : public MathFunction<T>
{
public:
    			AlongVectorFunction( const MathFunctionND<T>& func_,
					     const double* P_, const double* N_)
			    : P( P_ )
			    , N( N_ )
			    , func( func_ )
			{}

    T			getValue( double lambda ) const
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
    const MathFunctionND<T>&	func;
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


#endif
