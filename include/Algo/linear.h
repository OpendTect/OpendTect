#ifndef linear_h
#define linear_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Jan 2005
 RCS:		$Id: linear.h,v 1.8 2009-07-22 16:01:12 cvsbert Exp $
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
		{ return a0 + ax * x; }
    float	getXValue( float y ) const
		{ return ax ? (y - a0) / ax : 0; }
 
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

mClass LinStats2D
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

mClass LinStats3D
{
public:
		LinStats3D() : corrcoeff(0)	{}

    PlanePars	pp;		// Parameters
    PlanePars	sd;		// Standard deviations in parameters
    float	corrcoeff;	// Correlation coefficient

};


/*!\brief helps making nice axes for graphs */

mClass AxisLayout
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
