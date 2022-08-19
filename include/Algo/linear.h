#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"
#include "mathfunc.h"
namespace Geom { template <class T> class Point2D; }


/*!
\brief Steepness and intercept.
*/

template <class T>
mClass(Algo) LineParameters : public MathFunction<T,T>
{
public:
		LineParameters( T i0=0, T i1=0 )
		: a0(i0), ax(i1)		{}

    inline T	getValue( T x ) const override
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


/*!
\brief Steepnesses and intercept.
*/

template <class T>
mClass(Algo) PlaneParameters : public MathXYFunction<T,T>
{
public:
		PlaneParameters( T i0=0, T i1=0, T i2=0 )
		: a0(i0), ax(i1), ay(i2)	{}

    inline T	getValue( T x, T y ) const override
		{ return a0 + ax * x + ay * y; }
    inline T	getValue( const T* x ) const
			{ return getValue(x[0],x[1]); }

    T		a0, ax, ay;

};

typedef PlaneParameters<float> PlanePars;


/*!
\brief linear stats in 2D.
*/

mExpClass(Algo) LinStats2D
{
public:
		LinStats2D() : corrcoeff(0)	{}

    LinePars	lp;		//!< Parameters
    LinePars	sd;		//!< Standard deviations in parameters
    float	corrcoeff;	//!< Correlation coefficient

    void	use(const float*,const float*,int nrpts);
    void	use(const Geom::Point2D<float>*,int nrpts);
};


/*!
\brief linear stats in 3D.
*/

mExpClass(Algo) LinStats3D
{
public:
		LinStats3D() : corrcoeff(0)	{}

    PlanePars	pp;		//!< Parameters
    PlanePars	sd;		//!< Standard deviations in parameters
    float	corrcoeff;	//!< Correlation coefficient

};
