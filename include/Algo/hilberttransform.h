#ifndef hilberttransform_h
#define hilberttransform_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		December 2007
 RCS:		$Id: hilberttransform.h,v 1.1 2008-07-09 12:19:55 cvsnanne Exp $
________________________________________________________________________

-*/

#include "transform.h"

class ArrayNDInfo;
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

    bool		transform(const ArrayND<float>&,ArrayND<float>&) const;
    bool		transform(const ArrayND<float_complex>&,
				  ArrayND<float_complex>&) const;

protected:

    bool		isPossible(int) const;
    bool		isFast( int ) const		{ return true; }

    bool		forward_;
    ArrayNDInfo*	info_;
};


#endif
