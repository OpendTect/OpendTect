#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/


/*!

For every platform, the following variables get set:

	__win64__	Windows
	__lux64__	Linux
	__mac__		macOS

Together with:
OS type:

	__win__		Windows
	__unix__	Unix
	__lux__		Linux
	__mac__		macOS

Platform:

	__win64__	Windows (x64)
	__lux64__	Linux (x86_64)
	__macarm__	macOS Aarch64 (arm64)
	__macintel__	macOS Intel (x86_64)

	__plfsubdir__	String like "win64", "lux64" etc.

Compiler type:

	__msvc__	MS Visual C++
	__gnuc__	GNU gcc
	__clang__	Apple clang

Language:

	__cpp__		C++ (else C)

Byte order:

	__little__	little-endian

Always defined:

	__islittle__	'true' if little endian machine, false otherwise
	__iswin__	'true' on Windows, 'false' otherwise
	__islinux__	'true' on Linux, 'false' otherwise
	__ismac__	'true' on macOS, 'false' otherwise

*/


/*____________________________________________________________________________*/
/* OS type */

#undef __unix__
#undef __win__

#if defined( _WIN32 ) || defined( WIN32 ) || \
    defined( __CYGWIN__ ) || defined( __MINGW32__ )
# define __win__ 1
# define __win64__ 1
# define __iswin__ true
#else
# define __iswin__ false
#endif

#if defined ( __linux__ )
# define __unix__ 1
# define __lux__ 1
# define __lux64__ 1
# define __islinux__ true
#else
# define __islinux__ false
#endif

#if defined( __APPLE__ )
# define __unix__ 1
# define __mac__ 1
# if defined( __aarch64__ )
# define __macarm__ 1
# elif defined( __x86_64__ )
# define __macintel__ 1
# else
#  error "APPLE platform not detected"
# endif
# define __ismac__ true
#else
# define __ismac__ false
#endif



#if !defined(__unix__) && !defined(__win__)
# error "Platform not detected."
#endif

/*____________________________________________________________________________*/
/* Machine type	*/


#undef __little__
// All platforms are little endian
# define __little__ 1
# define __islittle__ true

#ifdef __win64__
# define __plfsubdir__	"win64"
#endif
#ifdef __lux64__
# define __plfsubdir__	"lux64"
#endif
#ifdef __mac__
# define __plfsubdir__	"mac"
#endif
/*____________________________________________________________________________*/
/* Language type */

#undef __cpp__
#ifdef __cplusplus
# if __cplusplus >= 201103L
#  define __cpp11__ 1
# endif
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
#if defined( __win__ )
# ifndef __gnuc__
#  define __msvc__ 1
# endif
#endif

#if defined( __win__ ) && !defined( __msvc__ )
# define __cygwin__ 1
#endif

/*____________________________________________________________________________*/
/* Configuration type */
#ifndef NDEBUG
# define __debug__
#endif

#undef mUnusedVar
#if defined( __gnuc__ )
# define mUnusedVar __attribute__ ((unused))
#else
# define mUnusedVar
#endif

//C++14 fully supports the 'deprecated' tag along with a message

#define mDeprecated(msg) [[deprecated(msg)]]
#define mDeprecatedDef	mDeprecated("See header file for alternatives")
#define mDeprecatedObs	mDeprecated("This is obsolete now")

#if defined( __gnuc__ ) || defined( __clang__ )
#  define mStartAllowDeprecatedSection \
    _Pragma ( "GCC diagnostic push" ) \
    _Pragma ( "GCC diagnostic ignored \"-Wdeprecated-declarations\"" )
#  define mStopAllowDeprecatedSection \
    _Pragma ( "GCC diagnostic pop" )
#elif defined( __msvc__ )
#  define mStartAllowDeprecatedSection \
    _Pragma ( "warning( push )" ) \
    _Pragma ( "warning( disable : 4996 )" )
#  define mStopAllowDeprecatedSection \
    _Pragma ( "warning( pop )" )
#else
#  define mStartAllowDeprecatedSection
#  define mStopAllowDeprecatedSection
#endif

#ifdef __win__
# define mODRestrict __restrict
# else
# define mODRestrict __restrict__
#endif


/* And, probably unnecessary, for external header files: */
#ifndef NeedFunctionPrototypes
# define NeedFunctionPrototypes 1
#endif
#ifndef _ANSI_C_SOURCE
# define _ANSI_C_SOURCE 1
#endif
