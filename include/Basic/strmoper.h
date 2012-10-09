#ifndef strmoper_h
#define strmoper_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		23-10-1996
 Contents:	Stream opening etc.
 RCS:		$Id$
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

    mGlobal od_int64	tell(std::istream&);
    mGlobal od_int64	tell(std::ostream&);
    mGlobal void	seek(std::istream&,od_int64 pos);
    mGlobal void	seek(std::istream&,od_int64 offset,std::ios::seekdir);
    mGlobal void	seek(std::ostream&,od_int64 pos);
    mGlobal void	seek(std::ostream&,od_int64 offset,std::ios::seekdir);
   

}


#endif
