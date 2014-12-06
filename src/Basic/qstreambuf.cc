/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2014
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "qstreambuf.h"

#include <QIODevice>


qstreambuf::qstreambuf( QIODevice& p )
    : iodevice_( &p )
{}

qstreambuf::~qstreambuf()
{ }


std::streamsize qstreambuf::showmanyc()
{
    return iodevice_->bytesAvailable();
}


std::streamsize qstreambuf::xsgetn( char_type* s, std::streamsize n )
{
    return iodevice_->read( s, n );
}


std::streamsize qstreambuf::xsputn( const char_type* s, std::streamsize n )
{
    return iodevice_->write( s, n );
}
