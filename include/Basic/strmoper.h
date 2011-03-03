#ifndef strmoper_h
#define strmoper_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		23-10-1996
 Contents:	Stream opening etc.
 RCS:		$Id: strmoper.h,v 1.16 2011-03-03 12:46:05 cvsbert Exp $
________________________________________________________________________

*/

#include "gendefs.h"
#include <iosfwd>
#include <iostream>
class BufferString;

/*!\brief Stream operations. operations will be retried on soft errors */

namespace StrmOper
{
    mGlobal bool	readBlock(std::istream&,void*,unsigned int nrbytes);
    mGlobal bool	writeBlock(std::ostream&,const void*,unsigned int);

    mGlobal bool	getNextChar(std::istream&,char&);
    mGlobal bool	wordFromLine(std::istream&,char*,int maxnrchars);

    mGlobal bool	readLine(std::istream&,BufferString* b=0);
    mGlobal bool	readFile(std::istream&,BufferString&);

    mGlobal void	seek(std::istream&,od_int64 pos);
    mGlobal od_int64	tell(std::istream&);
    mGlobal void	seek(std::istream&,od_int64 offset,std::ios::seekdir);
    mGlobal od_int64	tell(std::ostream&);

}


#endif
