#ifndef strmoper_H
#define strmoper_H

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		23-10-1996
 Contents:	Stream opening etc.
 RCS:		$Id: strmoper.h,v 1.9 2008-01-08 11:53:52 cvsbert Exp $
________________________________________________________________________

*/

#include "gendefs.h"
#include <iosfwd>
class BufferString;

/*!\brief Stream operations. operations will be retried on soft errors */

namespace StrmOper
{

    bool	readBlock(std::istream&,void*,unsigned int nrbytes);
    bool	writeBlock(std::ostream&,const void*,unsigned int nrbytes);

    bool	getNextChar(std::istream&,char&);
    bool	wordFromLine(std::istream&,char*,int maxnrchars);
    bool	readLine(std::istream&,BufferString* b=0);

}


#endif
