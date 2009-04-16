#ifndef plfdefs_h
#define plfdefs_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Contents:	Defines that encapsulate system things
 RCS:		$Id: plfdefs.h,v 1.24 2009-04-16 10:03:39 cvsranojay Exp $
________________________________________________________________________

*/


/*!

For every platform, the HDIR variable should be put in a -D$HDIR by make.

HDIR can be:

	lux		Linux
	win		M$ Windows
	mac		Apple Mac OSX
	sun5		Sun Solaris 5.x

Also, PLFSUBDIR should be defined. It is identical to HDIR, except for:
Linux:   lux32 or lux64
Solaris: sol32

Then you get:
OS type:

	__unix__	Unix
	__win__		Windows

Machine:

	__sun__		Sun
	__pc__		PC
			(Mac is just __mac__ below)

Platform:

	__win__		Windows
	__mac__		Mac
	__lux32__	Linux 32 bits (x86)
	__lux64__	Linux 64 bits (AMD)
	__sol32__	Solaris (Sparc)

	__plfsubdir__	String like "win", "lux32" etc.
	__plfname__	String like "MS WIndows", "Linux 32 bits"

Compiler type:

	__gnuc__	GNU gcc
	__sunc__	Sun c
	__msvc__	M$ Visual C++

Language:

	__cpp__		C++ (else C)

Byte order:

	__little__	little-endian - PC's, not Mac

Always defined:

	__islittle__	'true' if little endian machine, false otherwise
	__islinux__	'true' on Linux, 'false' otherwise
	__ismac__	'true' on Mac, 'false' otherwise
	__iswin__	'true' on Windows, 'false' otherwise

*/


/*____________________________________________________________________________*/
/* OS type */

#undef __unix__
#undef __win__

#if defined( win ) || defined( WIN32 ) 
# define __win__ 1
#endif
#ifdef lux
# define __unix__ 1
# ifdef lux64
#  define __lux64__ 1
# else
#  define __lux32__ 1
# endif
#endif
#ifdef sun5
# define __unix__ 1
#endif
#ifdef mac
# define __unix__ 1
#endif
#ifndef __unix__
#ifndef __win__
# warning "Platform not detected. choosing windows"
# define __win__ 1
#endif
#endif

#ifdef lux
# define __islinux__ true
#else
# define __islinux__ false
#endif
#ifdef mac
# define __ismac__ true
#else
# define __ismac__ false
#endif
#ifdef __win__
# define __iswin__ true
#else
# define __iswin__ false
#endif


/*____________________________________________________________________________*/
/* Machine type	*/

#undef __sun__
#undef __pc__
#undef __little__

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
#ifdef __sun__
# define __sol32__ 1
#endif

#ifdef lux
# define __pc__ 1
#endif
#ifdef __win__
# define __pc__ 1
#endif
#ifdef mac
# define __mac__ 1
# ifndef macppc
#  define __pc__ 1
# endif
#endif

#ifdef __pc__
# define __little__ 1
# define __islittle__ true
#else
#  define __islittle__ false
#endif

#ifdef __win__
# define __plfsubdir__	"win"
# define __plfname__	"MS Windows"
#endif
#ifdef __lux32__
# define __plfsubdir__	"lux32"
# define __plfname__	"Linux 32 bits"
#endif
#ifdef __lux64__
# define __plfsubdir__	"lux64"
# define __plfname__	"Linux 64 bits"
#endif
#ifdef __mac__
# define __plfsubdir__	"mac"
# define __plfname__	"Mac"
#endif
#ifdef __sol32__
# define __plfsubdir__	"sol32"
# define __plfname__	"Solaris/Sparc"
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
#if defined( win ) || defined( WIN32 )
# ifndef __gnuc__
#  define __msvc__ 1
# endif
#endif

#if defined( __win__ ) && !defined( __msvc__ )
# define __cygwin__ 1
#endif


/* Finally, support some common flags */
#ifndef NeedFunctionPrototypes
# define NeedFunctionPrototypes 1
#endif
#ifndef _ANSI_C_SOURCE
# define _ANSI_C_SOURCE 1
#endif


#endif
