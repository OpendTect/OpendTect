#ifndef string2_H
#define string2_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		11-4-1994
 Contents:	Extra string functions
 RCS:		$Id: string2.h,v 1.1.1.1 1999-09-03 10:11:41 dgb Exp $
________________________________________________________________________

@$*/

#include <ctype.h>
#include <gendefs.h>

#ifdef __cpp__
extern "C" {
#endif

/* any int >= 0 will do for caseInsensitiveEqual, here's some help: */
#define mCompareAll             0
#define mCompareFirstChar       1
#define mCompare3Char           3

void		removeTrailingBlanks	Pargs( (char*) );
/* void		skipLeadingBlanks	(char*&) */

int		caseInsensitiveEqual	Pargs( (const char*,const char*,int) );
int		matchStringCI		Pargs( (const char*,const char*) );
int		matchString		Pargs( (const char*,const char*) );

void		getPaddedString		Pargs( (const char*,char*,int) );

const char*	getNextWord		Pargs( (const char*,char*) );
int		getEqTokenValue		Pargs( (const char*,const char*,float*) );

int		countCharacter		Pargs( (const char*,char) );
void		replaceCharacter	Pargs( (char*,char,char) );
void		cleanupString		Pargs( (char*,int,int,int) );

const char*	getStringFromInt	Pargs( (const char*,int) );
const char*	getStringFromDouble	Pargs( (const char*,double) );
void		prettyNumber		Pargs( (char*) );

const char*	getYesNoString		Pargs( (int) );
int		yesNoFromString		Pargs( (const char*) );


/*$-*/

/*--------------------------------------------------@+skipLeadingBlanks
 macro advances given pointer to first non-whitespace.
\list{{\b Input}}
        \list{str}a pointer to a string
-----------------------------------------------------------------@$*/
#define skipLeadingBlanks(ptr) \
    { while ( (ptr) && *(ptr) && isspace(*(ptr)) ) (ptr)++; }

#ifdef __cpp__
}
#endif

/*$-*/
#endif
