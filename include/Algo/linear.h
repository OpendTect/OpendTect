#ifndef linear_h
#define linear_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Jan 2005
 RCS:		$Id: linear.h,v 1.11 2012-08-03 13:00:04 cvskris Exp $
________________________________________________________________________

-*/


#include "algomod.h"
#include "mathfunc.h"
namespace Geom { template <class T> class Point2D; }


/*!\brief steepness and intercept. */

template <class T>
class LineParameters : public MathFunction<T,T>
{
public:
		LineParameters( T i0=0, T i1=0 )
		: a0(i0), ax(i1)		{}
 
    inline T	getValue( T x ) const
			{ return a0 + ax * x; }
    inline T	getXValue( T y ) const
			{ return ax ? (y - a0) / ax : 0; }
    inline T	getProjectedX( T x, T y ) const
			{ return (x + ax * (y - a0)) / (1 + ax * ax); }
    inline T	getValue( const T* x ) const
			{ return getValue(*x); }
 
    T		a0, ax;
};

typedef LineParameters<float> LinePars;


/*!\brief steepnesses and intercept. */

template <class T>
class PlaneParameters : public MathXYFunction<T,T>
{
public:
		PlaneParameters( T i0=0, T i1=0, T i2=0 )
		: a0(i0), ax(i1), ay(i2)	{}

    inline T	getValue( T x, T y ) const
		{ return a0 + ax * x + ay * y; }
    inline T	getValue( const T* x ) const
			{ return getValue(x[0],x[1]); }

    T		a0, ax, ay;

};

typedef PlaneParameters<float> PlanePars;


/*!\brief linear stats in 2D. */

mClass(Algo) LinStats2D
{
public:
		LinStats2D() : corrcoeff(0)	{}

    LinePars	lp;		//!< Parameters
    LinePars	sd;		//!< Standard deviations in parameters
    float	corrcoeff;	//!< Correlation coefficient

    void	use(const float*,const float*,int nrpts);
    void	use(const Geom::Point2D<float>*,int nrpts);
};


/*!\brief linear stats in 3D. */

mClass(Algo) LinStats3D
{
public:
		LinStats3D() : corrcoeff(0)	{}

    PlanePars	pp;		//!< Parameters
    PlanePars	sd;		//!< Standard deviations in parameters
    float	corrcoeff;	//!< Correlation coefficient

};


#endif

