#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"
#include "transform.h"
#include "bufstring.h"

class ArrayNDInfo;
template <class T> class ValueSeries;
template <class T> class ArrayND;


/*!
\brief Class to compute Hilbert Transform.
*/

mExpClass(Algo) HilbertTransform
{
public:
			HilbertTransform();
			~HilbertTransform();

    bool		setInputInfo(const ArrayNDInfo&);
    const ArrayNDInfo&	getInputInfo() const		{ return *info_; }

    bool		real2real() const		{ return true; }
    bool		real2complex() const		{ return false; }
    bool		complex2real() const		{ return false; }
    bool		complex2complex() const		{ return false; }

    bool		biDirectional() const		{ return false; }
    bool		setDir(bool fw)			{ return fw; }
    bool		getDir() const			{ return forward_; }

    bool		init();
    void		setHalfLen( int hl )		{ halflen_ = hl; }
    void		setCalcRange(int startidx,int convstartidx);

			/*<! Will handle some undefined values
			     BUT will be very slow if there are mostly
				 undefined values.
			     Returns unchanged output if all input values are
			     undefined
			 */
    bool		transform(const float*,int szin,float*,int szout) const;
    bool		transform(const ValueSeries<float>&,int szin,
				  ValueSeries<float>&,int szout) const;
    bool		transform(const ArrayND<float>&,ArrayND<float>&) const;
    bool		transform(const ArrayND<float_complex>&,
				  ArrayND<float_complex>&) const;
    bool		transform(const ArrayND<float>&,
				  ArrayND<float_complex>&) const;

    uiString		errMsg() const		{ return errmsg_; }

protected:

    float*		makeHilbWindow(int);
    bool		isPossible(int) const;
    bool		isFast( int ) const		{ return true; }

    bool		transform(const float*,int szin,float*,int szout,
				  const ValueSeries<float>* in) const;

    bool		forward_;
    int			nrsamples_;
    int			halflen_;
    float*		hilbwindow_;
    ArrayNDInfo*	info_;
    int			startidx_;
    int			convstartidx_;

    mutable uiString    errmsg_;
};
