#ifndef Pmacros_h
#define Pmacros_h

/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Contents:	Macros that can be system or language dependent
 RCS:		$Id: plfdefs.h,v 1.2 2000-03-02 15:24:22 bert Exp $
________________________________________________________________________

For every platform, the HDIR variable should be put in a -D$HDIR by make.

HDIR can be:

	lux		Linux
	sun5		Sun Solaris 5.x
	ibm		IBM AIX 4.x
	sgi		SGI IRIX 5.x
	win		Windows 2000

OS type:

	__unix__	Unix
	__sysv__	---- System V
	__bsd__		---- Berkeley
	__win__		Windows

Machine:

	__sgi__		Silicon graphics
	__sun__		Sun
	__ibm__		IBM RS/6000
	__pc__		PC

Compiler type:

	__gnuc__	GNU gcc
	__mipsc__	MIPS c
	__sunc__	Sun c
	__ibmc__	Ibm C

Language:

	__cpp__		C++ (else C)

Bytes:

	__little__	little-endian (PC,DEC)

________________________________________________________________________________

*/


/*____________________________________________________________________________*/
/* OS type */

#undef __sysv__
#undef __bsd__
#ifdef SYSTYPE_SYSV
# define __sysv__ 1
#endif
#ifdef SVR4
# undef __sysv__
# define __sysv__ 1
#endif
#ifdef _SVR4
# undef __sysv__
# define __sysv__ 1
#endif
#ifdef SVR5
# undef __sysv__
# define __sysv__ 1
#endif
#ifdef SVR6
# undef __sysv__
# define __sysv__ 1
#endif

#undef __unix__
#undef __win__
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
# define __win__ 1
#endif

#ifndef __win__
# ifndef __sysv__
#  define __bsd__ 1
# endif
#endif


/*____________________________________________________________________________*/
/* Machine type	*/

#undef __sgi__
#undef __sun__
#undef __ibm__
#undef __pc__
#undef __little__
# define __little__ 1
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
# define __little__ 1
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
#ifdef lux
# define __gnuc__ 1
#endif
#ifdef __GNU_LIBRARY__
# undef __gnuc__
# define __gnuc__ 2
#endif
#ifdef sun5
# ifndef __gnuc__
#  define __sunc__ 1
# endif
#endif


/* Finally, support some common flags */
#ifndef NeedFunctionPrototypes
# define NeedFunctionPrototypes 1
#endif
#ifndef _ANSI_C_SOURCE
# define _ANSI_C_SOURCE 1
#endif


#define Pquote(token) #token
#define Ppaste(a,b) a##b
#define Ppaste3(a,b,c) a##b##c

		/* For calling C functions from FORTRAN */
#if defined(__sun__)
#	define Pfordecl(fun) Ppaste(fun,_)
#else
#	define Pfordecl(fun) fun
#endif


#endif
