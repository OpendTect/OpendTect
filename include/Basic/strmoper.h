#ifndef strmoper_H
#define strmoper_H

/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		23-10-1996
 Contents:	Stream opening etc.
 RCS:		$Id: strmoper.h,v 1.1.1.2 1999-09-16 09:19:20 arend Exp $
________________________________________________________________________

*/

#include <gendefs.h>

ostream*	openOutputStream(const char*);
istream*	openInputStream(const char*);
void		closeIOStream(ostream*&);
void		closeIOStream(istream*&);
int		wordFromLine(istream&,char*);

#endif
