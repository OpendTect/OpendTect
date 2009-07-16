/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Mar 2009
-*/

static const char* rcsID = "$Id: odusginfo.cc,v 1.3 2009-07-16 07:55:07 cvsbert Exp $";

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
    str += " ID="; str += id_;
    str += " group="; str += group_;
    str += " action="; str += action_;
    str += " aux="; str += aux_;
    if ( withreply_ ) str += " (with reply)";
    return str;
}
