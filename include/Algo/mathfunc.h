#ifndef mathfunc_H
#define mathfunc_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		17-11-1999
 Contents:	Mathematical Functions
 RCS:		$Id $
________________________________________________________________________

-*/


#include <position.h>


/*!\brief Mathematical function

A MathFunction must deliver a value at any position: F(x).
The positioning needs more precision than the outcome, hence
the doubles in the position.

*/

template <class T>
class MathFunction
{
public:

    virtual T		getValue(double) const	= 0;

};


/*!\brief a Math Function as in F(x,y). */

template <class T>
class MathXYFunction
{
public:

    virtual T		getValue(const Coord&) const	= 0;

};


/*!\brief a Math Function as in F(x,y,z). */
template <class T>
class MathXYZFunction
{
public:

    virtual T		getValue(const Coord3&) const	= 0;

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
