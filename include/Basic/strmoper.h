#ifndef strmoper_H
#define strmoper_H

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		23-10-1996
 Contents:	Stream opening etc.
 RCS:		$Id: strmoper.h,v 1.7 2003-11-07 12:21:51 bert Exp $
________________________________________________________________________

*/

#include <gendefs.h>
#include <iosfwd>

ostream*	openOutputStream(const char*);
istream*	openInputStream(const char*);
void		closeIOStream(ostream*&);
void		closeIOStream(istream*&);
bool		readWithRetry(istream&,void*,unsigned int nrbytes,
			      unsigned int nrretries,unsigned int delay);
bool		writeWithRetry(ostream&,const void*,unsigned int nrbytes,
			       unsigned int nrretries,unsigned int delay);
bool		wordFromLine(istream&,char*,int maxnrchars);
bool		ignoreToEOL(istream&);


#endif
