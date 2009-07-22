#ifndef strmoper_h
#define strmoper_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		23-10-1996
 Contents:	Stream opening etc.
 RCS:		$Id: strmoper.h,v 1.12 2009-07-22 16:01:14 cvsbert Exp $
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
