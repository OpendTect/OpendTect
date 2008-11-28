#ifndef hilberttransform_h
#define hilberttransform_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		December 2007
 RCS:		$Id: hilberttransform.h,v 1.3 2008-11-28 09:19:15 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "transform.h"
#include "bufstring.h"

class ArrayNDInfo;
template <class T> class ValueSeries;
template <class T> class ArrayND;

typedef std::complex<float> float_complex;

class HilbertTransform : public TransformND
{
public:
    			HilbertTransform();
			~HilbertTransform();

    bool		setInputInfo(const ArrayNDInfo&);
    const ArrayNDInfo&	getInputInfo() const		{ return *info_; }

    bool		isReal() const			{ return true; }
    bool		isCplx() const			{ return false; }

    bool		bidirectional() const		{ return false; }
    bool		setDir( bool fw )		{ return fw; }
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

    const char*		errMsg() const		{ return errmsg_.buf(); }

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
