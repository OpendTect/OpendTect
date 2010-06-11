#ifndef odcomplex_h
#define odcomplex_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
 RCS:		$Id: odcomplex.h,v 1.1 2010-06-11 06:42:52 cvsnanne Exp $
________________________________________________________________________

-*/

#include <complex>

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

#ifdef __win__
    T&	real()	{ return (this->_Val[_RE]); }
    T&	imag()	{ return (this->_Val[_IM]); }
#endif
};

}

#endif

