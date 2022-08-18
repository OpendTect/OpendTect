#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"

#include "gendefs.h"
#include "uistring.h"
#include <iostream>
class StreamData;

/*!\brief Stream operations. operations will be retried on soft errors. */

namespace StrmOper
{
    mGlobal(Basic) bool		readBlock(std::istream&,void*,od_uint64 nrbyts);
    mGlobal(Basic) bool		writeBlock(std::ostream&,const void*,od_uint64);

    mGlobal(Basic) bool		peekChar(std::istream&,char&);
    mGlobal(Basic) bool		readChar(std::istream&,char&,
					    bool allowreadingnewlines=false);
    mGlobal(Basic) bool		readWord(std::istream&,bool maycrossnewline,
					BufferString* b=0);
    mGlobal(Basic) bool		readLine(std::istream&,BufferString* b=0,
					 bool* newline_found=0);
    mGlobal(Basic) bool		readFile(std::istream&,BufferString&);
    mGlobal(Basic) bool		skipWhiteSpace(std::istream&);

    mGlobal(Basic) od_int64	tell(std::istream&);
    mGlobal(Basic) od_int64	tell(std::ostream&);
    mGlobal(Basic) void		seek(std::istream&,od_int64 pos);
    mGlobal(Basic) void		seek(std::istream&,od_int64 offset,
					std::ios::seekdir);
    mGlobal(Basic) void		seek(std::ostream&,od_int64 pos);
    mGlobal(Basic) void		seek(std::ostream&,od_int64 offset,
					std::ios::seekdir);
    mGlobal(Basic) od_int64	lastNrBytesRead(std::istream&);

    mGlobal(Basic) bool		resetSoftError(std::istream&,int& retrycount);
    mGlobal(Basic) bool		resetSoftError(std::ostream&,int& retrycount);
    mGlobal(Basic) void		clear(std::ios&);
    mGlobal(Basic) uiString	getErrorMessage(std::ios&);
    mGlobal(Basic) uiString	getErrorMessage(const StreamData&);

}
