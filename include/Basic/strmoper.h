#ifndef strmoper_H
#define strmoper_H

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		23-10-1996
 Contents:	Stream opening etc.
 RCS:		$Id: strmoper.h,v 1.8 2004-04-27 15:51:15 bert Exp $
________________________________________________________________________

*/

#include <gendefs.h>
#include <iosfwd>

std::ostream*	openOutputStream(const char*);
std::istream*	openInputStream(const char*);
void		closeIOStream(std::ostream*&);
void		closeIOStream(std::istream*&);
bool		readWithRetry(std::istream&,void*,unsigned int nrbytes,
			      unsigned int nrretries,unsigned int delay);
bool		writeWithRetry(std::ostream&,const void*,unsigned int nrbytes,
			       unsigned int nrretries,unsigned int delay);
bool		wordFromLine(std::istream&,char*,int maxnrchars);
bool		ignoreToEOL(std::istream&);


#endif
