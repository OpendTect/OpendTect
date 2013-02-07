#ifndef odcomplex_h
#define odcomplex_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include <complex>
#include "undefval.h"

typedef std::complex<float> float_complex;

#ifdef __win__
# define mSetComplexReal(compl_numb,val) compl_numb.real(val)
# define mSetComplexImag(compl_numb,val) compl_numb.imag(val)
#else
# define mSetComplexReal(compl_numb,val) compl_numb.real() = val
# define mSetComplexImag(compl_numb,val) compl_numb.imag() = val
#endif


namespace Values
{

/*!
\brief Undefined float_complex.
*/

template<>
mClass(Basic) Undef<float_complex>
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
				    return __mIsUndefinedF(r)
					|| __mIsUndefinedF(i);
				}
    static void			setUdf( float_complex& f )
				{
#ifdef __msvc__
				    f.real( (float)__mUndefValue );
				    f.imag( (float)__mUndefValue );
#else
				    f.real() = f.imag() = (float)__mUndefValue;
#endif
				}

};


};//namespace Values

mGlobal(Basic) bool dbgIsUdf(float_complex);

#endif

