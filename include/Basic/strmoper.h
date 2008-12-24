#ifndef strmoper_H
#define strmoper_H

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		23-10-1996
 Contents:	Stream opening etc.
 RCS:		$Id: strmoper.h,v 1.10 2008-12-24 12:47:14 cvsranojay Exp $
________________________________________________________________________

*/

#include "gendefs.h"
#include <iosfwd>
class BufferString;

/*!\brief Stream operations. operations will be retried on soft errors */

namespace StrmOper
{

    mGlobal bool readBlock(std::istream&,void*,unsigned int nrbytes);
    mGlobal bool writeBlock(std::ostream&,const void*,unsigned int nrbytes);

    mGlobal bool getNextChar(std::istream&,char&);
    mGlobal bool wordFromLine(std::istream&,char*,int maxnrchars);
    mGlobal bool readLine(std::istream&,BufferString* b=0);

}


#endif
