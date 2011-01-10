#ifndef odcomplex_h
#define odcomplex_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
 RCS:		$Id: odcomplex.h,v 1.4 2011-01-10 12:31:46 cvsranojay Exp $
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
    static float_complex val()		{ return float_complex(1e30,1e30); }
    static bool	hasUdf() 		{ return true; }
    static bool	isUdf(float_complex f) 	{ return __mIsUndefined(f.real())
						 || __mIsUndefined(f.imag()); }
    static void setUdf(float_complex& f)
    {
#ifdef __msvc__
	f.real( (float)__mUndefValue ); f.imag( (float)__mUndefValue ); 
#else
	f.real = (float)__mUndefValue; f.imag = (float)__mUndefValue;
#endif
    }   
};



};//namespace Values

#endif

