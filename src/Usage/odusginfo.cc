/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Mar 2009
-*/

static const char* rcsID = "$Id: odusginfo.cc,v 1.1 2009-03-17 12:53:18 cvsbert Exp $";

#include "odusginfo.h"
#include <iostream>


std::ostream& Usage::Info::dump( std::ostream& strm ) const
{
    BufferString str; dump( str );
    strm << str << std::endl;
    return strm;
}


BufferString& Usage::Info::dump( BufferString& str ) const
{
    str += start_ ? "START" : "STOP";
    str += " group="; str += group_; str += " action="; str += action_;
    str += " aux="; str += aux_;
    return str;
}
