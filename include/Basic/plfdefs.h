#ifndef plfdefs_h
#define plfdefs_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Contents:	Defines that encapsulate system things
 RCS:		$Id: plfdefs.h,v 1.11 2004-04-01 13:39:50 bert Exp $
________________________________________________________________________

*/


/*!

For every platform, the HDIR variable should be put in a -D$HDIR by make.

HDIR can be:

	lux		Linux
	sun5		Sun Solaris 5.x
	sgi		SGI IRIX 6.x
	win		M$ Windows

Then you get:
OS type:

	__unix__	Unix
	__win__		Windows

Machine:

	__sgi__		Silicon graphics
	__sun__		Sun
	__ibm__		IBM RS/6000
	__pc__		PC

Compiler type:

	__gnuc__	GNU gcc
	__sunc__	Sun c
	__msvc__	M$ Visual C++

Language:

	__cpp__		C++ (else C)

Byte order:

	__little__	little-endian (PC,DEC)

Always defined:

	__islittle__	'true' if __little__ defined, else 'false'
	__iswin__	'true' on windows, 'false' otherwise

*/


/*____________________________________________________________________________*/
/* OS type */

#undef __unix__
#undef __win__

#ifdef win
# define __win__ 1
#endif
#ifdef lux
# define __unix__ 1
#endif
#ifdef sun5
# define __unix__ 1
#endif
#ifdef ibm
# define __unix__ 1
#endif
#ifdef sgi
# define __unix__ 1
#endif
#ifndef __unix__
#ifndef __win__
# warning "Platform not detected. choosing windows"
# define __win__ 1
#endif
#endif

#ifdef __win__
# define __iswin__ true
#else
# define __iswin__ false
#endif


/*____________________________________________________________________________*/
/* Machine type	*/

#undef __sgi__
#undef __sun__
#undef __ibm__
#undef __pc__
#undef __little__
#ifdef sgi
# define __sgi__ 1
#endif
#ifdef __sgi
# undef __sgi__
# define __sgi__ 2
#endif
#ifdef sun5
# define __sun__ 1
#endif
#ifdef sparc
# undef __sun__
# define __sun__ 2
#endif
#ifdef sun
# undef __sun__
# define __sun__ 3
#endif
#ifdef _AIX
# define __ibm__ 1
#endif
#ifdef ibm
# undef __ibm__
# define __ibm__ 2
#endif
#ifdef lux
# define __pc__ 1
#endif
#ifdef __win__
# define __pc__ 1
#endif

#ifdef __pc__
# define __little__ 1
# define __islittle__ true
#else
# define __islittle__ false
#endif


/*____________________________________________________________________________*/
/* Language type */

#undef __cpp__
#ifdef __cplusplus
# define __cpp__ 1
#endif
#ifdef _LANGUAGE_C_PLUS_PLUS
# undef __cpp__
# define __cpp__ 2
#endif
#ifdef c_plusplus
# undef __cpp__
# define __cpp__ 3
#endif


/*____________________________________________________________________________*/
/* Compiler type */

#undef __gnu__
#undef __sunc__
#undef __msvc__
#ifdef lux
# define __gnuc__ 1
#endif
#ifdef __GNUC__
# undef __gnuc__
# define __gnuc__ 1
#endif
#ifdef sun5
# ifndef __gnuc__
#  define __sunc__ 1
# endif
#endif
#ifdef win
# ifndef __gnuc__
#  define __msvc__ 1
# endif
#endif


/* Finally, support some common flags */
#ifndef NeedFunctionPrototypes
# define NeedFunctionPrototypes 1
#endif
#ifndef _ANSI_C_SOURCE
# define _ANSI_C_SOURCE 1
#endif


#endif
