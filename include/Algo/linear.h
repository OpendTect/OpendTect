#ifndef linear_h
#define linear_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Jan 2005
 RCS:		$Id: linear.h,v 1.5 2007-12-18 07:30:26 cvsnanne Exp $
________________________________________________________________________

-*/


#include "mathfunc.h"
#include "geometry.h"


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
    void	use(const Geom::Point2D<float>*,int nrpts);
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
				AxisLayout();
				AxisLayout(float start,float stop,float step);
				AxisLayout(const StepInterval<float>& rg);
				AxisLayout(const Interval<float>& dr);

    void			setDataRange(const Interval<float>&);

    float			findEnd(float datastop) const;

    SamplingData<float>		sd;
    float			stop;

};


#endif
