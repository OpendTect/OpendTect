#ifndef strmoper_H
#define strmoper_H

/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		23-10-1996
 Contents:	Stream opening etc.
 RCS:		$Id: strmoper.h,v 1.4 2000-04-17 14:53:43 bert Exp $
________________________________________________________________________

*/

#include <gendefs.h>
class istream;
class ostream;

ostream*	openOutputStream(const char*);
istream*	openInputStream(const char*);
void		closeIOStream(ostream*&);
void		closeIOStream(istream*&);
bool		readWithRetry(istream&,void*,unsigned int nrbytes,
			      unsigned int nrretries,unsigned int delay);
bool		writeWithRetry(ostream&,const void*,unsigned int nrbytes,
			       unsigned int nrretries,unsigned int delay);
bool		wordFromLine(istream&,char*,int maxnrchars);


#endif
