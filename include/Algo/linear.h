#ifndef linear_h
#define linear_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Jan 2005
 RCS:		$Id: linear.h,v 1.1 2005-01-26 16:40:27 bert Exp $
________________________________________________________________________

-*/


#include "mathfunc.h"

/*!\brief 2D data point. */
struct DataPoint2D	{ float x, y; };

/*!\brief 3D data point. */
struct DataPoint3D	{ float x, y, z; };


/*!\brief steepness and intercept. */

class LinePars : public MathFunction<float>
{
public:
		LinePars( float i0=0, float i1=0 )
		: a0(i0), ax(i1)		{}
 
    float	getValue( double x ) const
		{ return (float)(a0 + ax * x); }
 
    float	a0, ax;
};


/*!\brief steepnesses and intercept. */

class PlanePars : public MathXYFunction<float>
{
public:
		PlanePars( double i0=0, double i1=0, double i2=0 )
		: a0(i0), ax(i1), ay(i2)	{}

    float	getValue( const Coord& c ) const
		{ return (float)(a0 + ax * c.x + ay * c.y); }

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
    void	use(const DataPoint2D*,int nrpts);
};


/*!\brief linear stats in 3D. */

class LinStats3D
{
public:
		LinStats3D() : corrcoeff(0)	{}

    PlanePars	pp;		// Parameters
    PlanePars	sd;		// Standard deviations in parameters
    float	corrcoeff;	// Correlation coefficient

    void	use(const float*,const float*,const float*,int nrpts); //!< NI
    void	use(const DataPoint3D*,int nrpts); //!< NI
};


#endif
