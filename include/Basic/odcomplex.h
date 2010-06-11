#ifndef odcomplex_h
#define odcomplex_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
 RCS:		$Id: odcomplex.h,v 1.2 2010-06-11 07:06:44 cvsnanne Exp $
________________________________________________________________________

-*/

#include <complex>

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

#endif

