#ifndef hilberttransform_h
#define hilberttransform_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		December 2007
 RCS:		$Id$
________________________________________________________________________

-*/

#include "algomod.h"
#include "transform.h"
#include "bufstring.h"

class ArrayNDInfo;
template <class T> class ValueSeries;
template <class T> class ArrayND;

typedef std::complex<float> float_complex;


/*!
\ingroup Algo
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
    void		setCalcRange(int, int, int);

    bool		transform(const ValueSeries<float>&,int szin,
	    			  ValueSeries<float>&,int szout) const;
    bool		transform(const ArrayND<float>&,ArrayND<float>&) const;
    bool		transform(const ArrayND<float_complex>&,
				  ArrayND<float_complex>&) const;
    bool		transform(const ArrayND<float>&,
				  ArrayND<float_complex>&) const;

    const char*		errMsg() const		{ return errmsg_.str(); }

protected:

    float*		makeHilbWindow(int);
    bool		isPossible(int) const;
    bool		isFast( int ) const		{ return true; }

    bool		forward_;
    int			nrsamples_;
    int			halflen_;
    float*		hilbwindow_;
    ArrayNDInfo*	info_;
    int			startidx_;
    int			convstartidx_;
    int			arrminnrsamp_;

    mutable BufferString errmsg_;
};


#endif

