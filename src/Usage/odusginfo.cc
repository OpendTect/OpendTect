/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2009
-*/

static const char* rcsID = "$Id: odusginfo.cc,v 1.8 2010/10/14 09:58:06 cvsbert Exp $";

#include "odusginfo.h"
#include "hostdata.h"
#include <iostream>


static od_int64 usginfo_curid = 0;

Usage::Info::ID::ID()
    : pid_(GetPID())
    , hostname_(HostData::localHostName())
{
    nr_ = ++usginfo_curid;
}


Usage::Info::ID::ID( od_uint64 nr, const char* hnm, int pid )
    : nr_(nr)
    , pid_(pid)
    , hostname_(hnm)
{
}


bool Usage::Info::ID::operator ==( const Usage::Info::ID& id ) const
{
    return nr_ == id.nr_ && pid_ == id.pid_ && hostname_ == id.hostname_;
}


Usage::Info::ID& Usage::Info::ID::operator =( const Usage::Info::ID& id )
{
    if ( this != &id )
    {
	nr_ = id.nr_;
	hostname_ = id.hostname_;
	pid_ = id.pid_;
    }
    return *this;
}


bool Usage::Info::ID::isLocal() const
{
    return hostname_ == HostData::localHostName();
}


void Usage::Info::ID::putTo( BufferString& str ) const
{
    str += nr_; str += "@";
    str += hostname_; str += ":"; str += pid_;
}


bool Usage::Info::ID::getFrom( const char* str )
{
    BufferString buf( str );

    char* startptr = buf.buf();
    char* ptr = strchr( startptr, '@' );
    if ( !ptr ) return false;
    *ptr++ = '\0'; startptr = ptr;
    nr_ = atoll( startptr );

    ptr = strchr( startptr, ':' );
    if ( !ptr ) return false;
    *ptr = '\0';
    hostname_ = startptr;
    startptr = ptr + 1;

    pid_ = toInt( startptr );
    return true;
}


Usage::Info::Info( const char* grp, const char* act, const char* aux )
    : group_(grp)
    , action_(act)
    , aux_(aux)
    , delim_(Start)
    , withreply_(false)
{
}


void Usage::Info::prepareForSend()
{
    id_.nr_ = ++usginfo_curid;
}


std::ostream& Usage::Info::dump( std::ostream& strm ) const
{
    BufferString str; dump( str );
    strm << str << std::endl;
    return strm;
}


BufferString& Usage::Info::dump( BufferString& str ) const
{
    id_.putTo( str );
    str += delim_ == Start ? " START" : (delim_ == Stop ? " STOP" : " USG");
    str += " group="; str += group_;
    str += " action="; str += action_;
    if ( !aux_.isEmpty() )
	{ str += " aux="; str += aux_; }
    if ( withreply_ )
	str += " (reply requested)";
    return str;
}
