#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Sept 2011
________________________________________________________________________

-*/

#include "algomod.h"
#include "algomod.h"
#include "arrayndimpl.h"
#include "odcomplex.h"
#include "ranges.h"
#include "paralleltask.h"

namespace Fourier { class CC; };

/*!
\brief Base class for fourier interpolation.
*/

mExpClass(Algo) FourierInterpolBase
{
public:
    void	setTargetDomain(bool fourier);
				/*!<Default is time-domain */
protected:
				FourierInterpolBase();
				~FourierInterpolBase();

    Fourier::CC*                fft_;
};


/*!
\brief Fourier interpolation for 1D datasets.
*/

mExpClass(Algo) FourierInterpol1D : public ParallelTask
				  , public FourierInterpolBase
{
public:

    mStruct(Algo) Point
    {
				Point(float_complex v,float x)
				    : val_(v), pos_(x) {}

	float_complex		val_;
	float			pos_;

	inline bool operator	== (const Point& p) const
						{ return pos_ == p.pos_; }
    };

				FourierInterpol1D(const TypeSet<Point>& pts,
				    const StepInterval<float>& outsampling);

				~FourierInterpol1D();

    const Array1DImpl<float_complex>* getOutput() const
				{ return arrs_.isEmpty() ? 0 : arrs_[0]; }

protected:
    od_int64			nrIterations() const	{ return pts_.size(); }

    bool			doPrepare(int);
    bool			doWork(od_int64,od_int64,int);
    bool			doFinish(bool);

    const TypeSet<Point>&	pts_;

    int				sz_;
    const StepInterval<float>&	sampling_;

    ObjectSet< Array1DImpl<float_complex> > arrs_;
};


/*!
\brief Fourier interpolation for 2D datasets.
*/

mExpClass(Algo) FourierInterpol2D : public ParallelTask
				  , public FourierInterpolBase
{
public:

    mStruct(Algo) Point
    {
				Point(float_complex v,float x,float y)
				    : val_(v), xpos_(x) , ypos_(y) {}

	float_complex		val_;
	float			xpos_, ypos_;

	inline bool operator	== (const Point& p) const
				{ return xpos_ == p.xpos_ && ypos_ == p.ypos_; }
    };

				FourierInterpol2D(const TypeSet<Point>& pts,
				    const StepInterval<float>& xoutsampling,
				    const StepInterval<float>& youtsampling);

				~FourierInterpol2D();

    const Array2DImpl<float_complex>* getOutput() const
				{ return arrs_.isEmpty() ? 0 : arrs_[0]; }

protected:
    od_int64			nrIterations() const	{ return pts_.size(); }

    bool			doPrepare(int);
    bool			doWork(od_int64,od_int64,int);
    bool			doFinish(bool);

    const TypeSet<Point>&	pts_;

    int				szx_, szy_;
    const StepInterval<float>&	xsampling_, ysampling_;

    ObjectSet< Array2DImpl<float_complex> > arrs_;
};


/*!
\brief Fourier interpolation for 3D datasets.
*/

mExpClass(Algo) FourierInterpol3D : public ParallelTask
				  , public FourierInterpolBase
{
public:

    mStruct(Algo) Point
    {
				Point(float_complex v,float x,float y,float z)
				    : val_(v), xpos_(x) , ypos_(y) , zpos_(z) {}

	float_complex		val_;
	float			xpos_, ypos_, zpos_;

	inline bool operator	== (const Point& p) const
				{
				    return xpos_ == p.xpos_
					&& ypos_ == p.ypos_
					&& zpos_ == p.zpos_;
				}
    };

				FourierInterpol3D(const TypeSet<Point>& pts,
				    const StepInterval<float>& xoutsampling,
				    const StepInterval<float>& youtsampling,
				    const StepInterval<float>& zoutsampling);

				~FourierInterpol3D();

    const Array3DImpl<float_complex>* getOutput() const
				{ return arrs_.isEmpty() ? 0 : arrs_[0]; }

protected:
    od_int64			nrIterations() const	{ return pts_.size(); }

    bool			doPrepare(int);
    bool			doWork(od_int64,od_int64,int);
    bool			doFinish(bool);

    const TypeSet<Point>&	pts_;

    int				szx_, szy_, szz_;
    const StepInterval<float>&	xsampling_, ysampling_, zsampling_;

    ObjectSet< Array3DImpl<float_complex> > arrs_;
};


