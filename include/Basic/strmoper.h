#ifndef strmoper_H
#define strmoper_H

/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		23-10-1996
 Contents:	Stream opening etc.
 RCS:		$Id: strmoper.h,v 1.6 2001-06-07 09:42:38 bert Exp $
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
