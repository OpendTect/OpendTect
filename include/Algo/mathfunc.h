#ifndef mathfunc_H
#define mathfunc_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		17-11-1999
 Contents:	Mathimatical Functions
 RCS:		$Id $
________________________________________________________________________

A MathFunction must deliver a value at any position: F(x). The positioning
needs more precision than the outcome, hence the double for the position.
Linear functions have two parameters, ax (steepness) and a0 (intercept).
Linear statistics have Line parameters for the regression line, the errors and
a correlation coefficient.

*/


#include <gendefs.h>


class MathFunction
{
public:

    virtual float	getValue(double) const	= 0;

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


struct LinStats
{
		LinStats() : corrcoeff(0)	{}

    LinePars	lp;		// Parameters
    LinePars	sd;		// Standard deviations in parameters
    double	corrcoeff;	// Correlation coefficient
};


#endif
