#ifndef string2_H
#define string2_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		11-4-1994
 Contents:	Extra string functions
 RCS:		$Id: string2.h,v 1.2 2000-03-02 15:24:33 bert Exp $
________________________________________________________________________

String functions just outside the standard:. Some remarks:
* removeTrailingBlanks bluntly puts a '\0' on trailing white space.
* getStringFromInt returns the string for an int in a static buffer. You'll have
  to pass a C-style ('printf') format.
* getStringFromDouble is like getStringFromInt. Treats "%lg" special: it will
  do removal of trailing zeros and use %f in more cases than std.
* countCharacter counts occurrences of a char in string
* replaceCharacter replaces all occurrences of a char with another
* cleanupString cleans a string from non-alpha numeric by replacing with
  underscores.
* matchString checks whether a string is the start of another string.
* matchStringCI is a case insensitive version of matchString
* getPaddedString fills a string with another padded with blanks (to the right).
* getNextWord fills a buffer with the next word (delimited by whitespace) in
  string. It returns a ptr just after the word.
* getEqTokenValue returns the value for a value token `XXX=xxx'.
* prettyNumber removes unwanted zeros and dots from a floating point in string.

-*/

#include <ctype.h>
#include <gendefs.h>

#ifdef __cpp__
extern "C" {
#endif


void		removeTrailingBlanks(char*);
/* void		skipLeadingBlanks(char*&) */

int		caseInsensitiveEqual(const char*,const char*,
				     int nr_chars_to_match_0_is_all);
int		matchString(const char* bigstring,const char* start_with);
int		matchStringCI(const char*,const char*);

void		getPaddedString(const char*,char*,int max_nr_from_inp_0_is_all);

const char*	getNextWord(const char*,char*);
int		getEqTokenValue(const char*,const char*,float*);

int		countCharacter(const char*,char);
void		replaceCharacter(char*,char,char);
void		cleanupString(char*,int ,int,int);

const char*	getStringFromInt(const char* fmt,int);
const char*	getStringFromDouble(const char* fmt,double);
void		prettyNumber(char*);

const char*	getYesNoString(int);
int		yesNoFromString(const char*);


/*$-*/

/* macro advances given pointer to first non-whitespace. */
#define skipLeadingBlanks(ptr) \
    { while ( (ptr) && *(ptr) && isspace(*(ptr)) ) (ptr)++; }

#ifdef __cpp__
}
#endif


#endif
