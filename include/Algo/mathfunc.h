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


/*! \brief Mathematical functions

A MathFunction must deliver a value at any position: F(x), a MathXYFunction
is F(x,y). The positioning needs more precision than the outcome, hence
the doubles in the position.
Linear functions have two parameters, ax (steepness, for 2d also ay)
and a0 (intercept).
Linear statistics have Line parameters for the regression line, the errors
and a correlation coefficient. There is no 2D variant yet.

*/


#include <position.h>


class MathFunction
{
public:

    virtual float	getValue(double) const	= 0;

};


class MathXYFunction
{
public:

    virtual float	getValue(const Coord&) const	= 0;

};


class LinePars : public MathFunction
{
public:
		LinePars( double i0=0, double i1=0 )
		: a0(i0), ax(i1)		{}
 
    float	getValue( double x ) const
		{ return (float)(a0 + ax * x); }
 
    double      a0, ax;
};


class PlanePars : public MathXYFunction
{
public:
		PlanePars( double i0=0, double i1=0, double i2=0 )
		: a0(i0), ax(i1), ay(i2)	{}

    float	getValue( const Coord& c ) const
		{ return (float)(a0 + ax * c.x + ay * c.y); }

    double	a0, ax, ay;

};



class LinStats
{
public:
		LinStats() : corrcoeff(0)	{}

    LinePars	lp;		// Parameters
    LinePars	sd;		// Standard deviations in parameters
    double	corrcoeff;	// Correlation coefficient
};


#endif
