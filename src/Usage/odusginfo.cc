/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Mar 2009
-*/

static const char* rcsID = "$Id: odusginfo.cc,v 1.2 2009-06-30 15:23:47 cvsbert Exp $";

#include "odusginfo.h"
#include <iostream>


Usage::Info::ID Usage::Info::newID()
{
    static Usage::Info::ID id = 0;
    return ++id;
}


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
