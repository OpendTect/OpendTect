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

#include "basicmod.h"
#include "gendefs.h"
#include <iosfwd>
#include <iostream>
class BufferString;

/*!\brief Stream operations. operations will be retried on soft errors */

namespace StrmOper
{
    mGlobal(Basic) bool	readBlock(std::istream&,void*,unsigned int nrbytes);
    mGlobal(Basic) bool	writeBlock(std::ostream&,const void*,unsigned int);

    mGlobal(Basic) bool	getNextChar(std::istream&,char&);
    mGlobal(Basic) bool	wordFromLine(std::istream&,char*,int maxnrchars);

    mGlobal(Basic) bool	readLine(std::istream&,BufferString* b=0);
    mGlobal(Basic) bool	readFile(std::istream&,BufferString&);

    mGlobal(Basic) od_int64	tell(std::istream&);
    mGlobal(Basic) od_int64	tell(std::ostream&);
    mGlobal(Basic) void	seek(std::istream&,od_int64 pos);
    mGlobal(Basic) void	seek(std::istream&,od_int64 offset,std::ios::seekdir);
    mGlobal(Basic) void	seek(std::ostream&,od_int64 pos);
    mGlobal(Basic) void	seek(std::ostream&,od_int64 offset,std::ios::seekdir);
   

}


#endif

