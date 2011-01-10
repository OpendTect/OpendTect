#ifndef odcomplex_h
#define odcomplex_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
 RCS:		$Id: odcomplex.h,v 1.6 2011-01-10 14:57:33 cvsbert Exp $
________________________________________________________________________

-*/

#include <complex>
#include "undefval.h"

typedef std::complex<float> float_complex;

namespace Values
{

template<>
class Undef<float_complex>
{
public:

    static bool			hasUdf() 		{ return true; }
    static float_complex	val()
				{
				    return float_complex( (float)__mUndefValue,
					    		  (float)__mUndefValue);
				}
    static bool			isUdf( float_complex f )
				{
				    const float r = f.real();
				    const float i = f.imag();
				    return __mIsUndefined(r)
					|| __mIsUndefined(i);
				}
    static void			setUdf( float_complex& f )
				{
				    f.real( (float)__mUndefValue );
				    f.imag( (float)__mUndefValue );
				}

};


};//namespace Values

#endif

