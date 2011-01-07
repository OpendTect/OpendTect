#ifndef odcomplex_h
#define odcomplex_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
 RCS:		$Id: odcomplex.h,v 1.3 2011-01-07 12:54:27 cvsbruno Exp $
________________________________________________________________________

-*/

#include <complex>
#include "undefval.h"

#ifndef __win__

typedef std::complex<float> float_complex;

#else

namespace std
{

template <class T>
class odcomplex : public complex<T>
{
public:
    		template <class U>
		odcomplex( const complex<U>& c )
		    : complex<T>(c.real(),c.imag())	{}

		odcomplex( const T& r = T(), const T& i = T() )
		    : complex<T>(r,i)	{}

    T&		real()	{ return (this->_Val[_RE]); }
    T&		imag()	{ return (this->_Val[_IM]); }
};

} // namespace std


typedef std::odcomplex<float> float_complex;

#endif

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
    static void	setUdf(float_complex& f) { f.real() = (float)__mUndefValue;
					   f.imag() = (float)__mUndefValue; }
};

};//namespace Values

#endif

