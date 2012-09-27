#ifndef plfdefs_h
#define plfdefs_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Contents:	Defines that encapsulate system things
 RCS:		$Id$
________________________________________________________________________

*/


/*!

For every platform, one of the following variables must be set by cmake:

	__lux64__, __lux32__	Linux
	__win64__, __win32__	MS Windows
	__mac__			Apple Mac OSX

Then you get:
OS type:

	__unix__	Unix
	__lux__		Linux
	__win__		Windows

Platform:

	__win32__	Windows 32 bits (x86)
	__win64__	Windows 64 bits (AMD)
	__lux32__	Linux 32 bits (x86)
	__lux64__	Linux 64 bits (AMD)
	__mac__		Mac

	__plfsubdir__	String like "win32", "lux32" etc.
	__plfname__	String like "MS Windows 32 bits", "Linux 32 bits"

Compiler type:

	__gnuc__	GNU gcc
	__msvc__	MS Visual C++

Language:

	__cpp__		C++ (else C)

Byte order:

	__little__	little-endian
 
Always defined:

	__islittle__	'true' if little endian machine, false otherwise
	__islinux__	'true' on Linux, 'false' otherwise
	__is32bits__	'true' on 32 bits platforms, 'false' otherwise
	__ismac__	'true' on Mac, 'false' otherwise
	__iswin__	'true' on Windows, 'false' otherwise

*/


/*____________________________________________________________________________*/
/* OS type */

#undef __unix__
#undef __win__

#if defined( __win64__ ) || defined ( __win32__ )
# define __win__ 1
#endif

#if defined ( __lux32__ ) || defined ( __lux64__ )
# define __unix__ 1
# define __lux__ 1
#endif

#if defined( __mac__ )
# define __unix__ 1
#endif

#ifndef __unix__
#ifndef __win__
# error "Platform not detected."
#endif
#endif

#ifdef __lux__
# define __islinux__ true
#else
# define __islinux__ false
#endif
#ifdef __mac__
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


#undef __little__
// All platforms are little endian
# define __little__ 1
# define __islittle__ true

#ifdef __win32__
# define __plfsubdir__	"win32"
# define __plfname__	"MS Windows 32 bits"
# define __is32bits__	true
#endif
#ifdef __win64__
# define __plfsubdir__	"win64"
# define __plfname__	"MS Windows 64 bits"
# define __is32bits__	false
#endif
#ifdef __lux32__
# define __plfsubdir__	"lux32"
# define __plfname__	"Linux 32 bits"
# define __is32bits__	true
#endif
#ifdef __lux64__
# define __plfsubdir__	"lux64"
# define __plfname__	"Linux 64 bits"
# define __is32bits__	false
#endif
#ifdef __mac__
# define __plfsubdir__	"mac"
# define __plfname__	"Mac"
# define __is32bits__	false
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
#undef __msvc__
#ifdef __lux__
# define __gnuc__ 1
#endif
#ifdef __GNUC__
# undef __gnuc__
# define __gnuc__ 1
#endif
#if defined( __win__ ) || defined( WIN32 )
# ifndef __gnuc__
#  define __msvc__ 1
# endif
#endif

#if defined( __win__ ) && !defined( __msvc__ )
# define __cygwin__ 1
#endif


#undef mUnusedVar
#if defined( __gnuc__ )
# define mUnusedVar __attribute__ ((unused))
#else
# define mUnusedVar
#endif

/* And, probably unnecessary, for external header files: */
#ifndef NeedFunctionPrototypes
# define NeedFunctionPrototypes 1
#endif
#ifndef _ANSI_C_SOURCE
# define _ANSI_C_SOURCE 1
#endif


#endif
