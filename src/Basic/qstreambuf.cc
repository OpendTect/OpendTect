/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2014
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "qstreambuf.h"

#ifndef OD_NO_QT
# include <QIODevice>
#endif


qstreambuf::qstreambuf( QIODevice& p )
    : iodevice_( &p )
{}

qstreambuf::~qstreambuf()
{ }


std::streamsize qstreambuf::showmanyc()
{
#ifndef OD_NO_QT
    return iodevice_->bytesAvailable();
#else
    return 0;
#endif
}


std::streamsize qstreambuf::xsgetn( char_type* s, std::streamsize n )
{
#ifndef OD_NO_QT
    return iodevice_->read( s, n );
#else
    return 0;
#endif
}


std::streamsize qstreambuf::xsputn( const char_type* s, std::streamsize n )
{
#ifndef OD_NO_QT
    return iodevice_->write( s, n );
#else
    return 0;
#endif
}
