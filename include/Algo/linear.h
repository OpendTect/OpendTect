#ifndef linear_h
#define linear_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Jan 2005
 RCS:		$Id: linear.h,v 1.2 2005-01-28 13:31:16 bert Exp $
________________________________________________________________________

-*/


#include "mathfunc.h"


/*!\brief steepness and intercept. */

class LinePars : public MathFunction<float,float>
{
public:
		LinePars( float i0=0, float i1=0 )
		: a0(i0), ax(i1)		{}
 
    float	getValue( float x ) const
		{ return (float)(a0 + ax * x); }
 
    float	a0, ax;
};


/*!\brief steepnesses and intercept. */

class PlanePars : public MathXYFunction<float,float>
{
public:
		PlanePars( float i0=0, float i1=0, float i2=0 )
		: a0(i0), ax(i1), ay(i2)	{}

    float	getValue( float x, float y ) const
		{ return a0 + ax * x + ay * y; }

    float	a0, ax, ay;

};


/*!\brief linear stats in 2D. */

class LinStats2D
{
public:
		LinStats2D() : corrcoeff(0)	{}

    LinePars	lp;		// Parameters
    LinePars	sd;		// Standard deviations in parameters
    float	corrcoeff;	// Correlation coefficient

    void	use(const float*,const float*,int nrpts);
    void	use(const DataPoint2D<float>*,int nrpts);
};


/*!\brief linear stats in 3D. */

class LinStats3D
{
public:
		LinStats3D() : corrcoeff(0)	{}

    PlanePars	pp;		// Parameters
    PlanePars	sd;		// Standard deviations in parameters
    float	corrcoeff;	// Correlation coefficient

};


/*!\brief helps making nice axes for graphs */

class AxisLayout
{
public:
				AxisLayout()
				    : sd(0,1), stop(1)	{}
				AxisLayout( Interval<float> dr )
							{ setDataRange(dr); }

    void			setDataRange(Interval<float>);

    float			findEnd(float datastop) const;

    SamplingData<float>		sd;
    float			stop;

};


#endif
