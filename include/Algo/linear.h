#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2005
________________________________________________________________________

-*/


#include "algomod.h"
#include "coord.h"
#include "mathfunc.h"

class Plane3;

/*!
\brief Steepness and intercept.
*/

template <class T>
mClass(Algo) LineParameters : public MathFunction<T,T>
{
public:
		LineParameters( T a0=0, T ax=0 )
		: a0_(a0), ax_(ax)		{}

    inline T	getValue( T x ) const
			{ return a0_ + ax_ * x; }
    inline T	getXValue( T y ) const
			{ return ax_ ? (y - a0_) / ax_ : 0; }
    inline T	getProjectedX( T x, T y ) const
			{ return (x + ax_ * (y - a0_)) / (1 + ax_ * ax_); }
    inline T	getValue( const T* x ) const
			{ return getValue(*x); }

    T		a0_, ax_;
};

typedef LineParameters<float> LinePars;


/*!
\brief Steepnesses and intercept.
*/

template <class T>
mClass(Algo) PlaneParameters : public MathXYFunction<T,T>
{
public:
		PlaneParameters( T a0=0, T ax=0, T ay=0 )
		: a0_(a0), ax_(ax), ay_(ay)	{}

    inline T	getValue( T x, T y ) const
		{ return a0_ + ax_ * x + ay_ * y; }
    inline T	getValue( const T* x ) const
			{ return getValue(x[0],x[1]); }

    T		a0_, ax_, ay_;

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

/*! \brief Best fit plane based on 3D points */
mExpClass(Algo) Plane3DFit
{
public:
			Plane3DFit();

    bool		compute(const TypeSet<Coord3>& points,Plane3& result);

protected:

    void		setScatterMatrix(double scattermatrix[3][3],
					 int order[3]);
    void		tqli(double d[3],double e[3],double z[3][3]);
    void		tred2(double a[3][3],double d[3],double e[3]);

    TypeSet<Coord3>	points_;
    Coord3		centroid_;
};
