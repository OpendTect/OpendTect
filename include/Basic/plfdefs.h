#ifndef Pmacros_h
#define Pmacros_h

/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Contents:	Macros that can be system or language dependent
 RCS:		$Id: plfdefs.h,v 1.1.1.1 1999-09-03 10:11:40 dgb Exp $
________________________________________________________________________

*/

#include <Pdefs.h>

/* Note that P also stands for Pre-Processor */

		/* Quote and paste */
#if defined (__stdc__)
#	define Pquote(token) #token
#	define Ppaste(a,b) a##b
#	define Ppaste3(a,b,c) a##b##c
#else
#	define Pquote(token) "token"
#	ifdef __sysv__
#		define Ppaste(a,b) a\
b
#		define Ppaste3(a,b,c) a\
b\
c
#	else
#		define Ppaste(a,b) a/**/b
#		define Ppaste3(a,b,c) a/**/b/**/c
#	endif
#endif

		/* Prototyping/Non-prototyping */
#ifdef __proto__
#	define	Pargs(args)	args
#	define	Pnoargs		void
#else
#	define	Pargs(args)	()
#	define	Pnoargs		/* nothing */
#endif


		/* For calling C functions from FORTRAN */
#if defined(__sun__)
#	define Pfordecl(fun) Ppaste(fun,_)
#else
#	define Pfordecl(fun) fun
#endif


#endif
