/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Jul 2006
-*/

static const char* rcsID = "$Id: tableascio.cc,v 1.1 2006-11-06 16:04:27 cvsbert Exp $";

#include "tableascio.h"
#include "tabledef.h"
#include <iostream>


bool Table::AscIO::getHdrVals( std::istream& strm ) const
{
    errmsg_ = "TODO: Table::AscIO::getHdrVals not implemented";
    return false;
}


bool Table::AscIO::getNextBodyVals( std::istream& strm ) const
{
    errmsg_ = "TODO: Table::AscIO::getNextBodyVals not implemented";
    return false;
}


bool Table::AscIO::putHdrVals( std::ostream& strm ) const
{
    errmsg_ = "TODO: Table::AscIO::putHdrVals not implemented";
    return false;
}


bool Table::AscIO::putNextBodyVals( std::ostream& strm ) const
{
    errmsg_ = "TODO: Table::AscIO::putNextBodyVals not implemented";
    return false;
}
