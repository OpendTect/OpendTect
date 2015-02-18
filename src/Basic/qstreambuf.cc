/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2014
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "qstreambuf.h"

#ifndef OD_NO_QT
# include <QProcess>
#endif

#define mPUSH_BACK	100
#define mBUFFER_SIZE	1000

#define mTIMEOUT	100


qstreambuf::qstreambuf( QIODevice& p, bool isstderr )
    : iodevice_( &p )
    , isstderr_( isstderr )
    , buffer_( mPUSH_BACK+mBUFFER_SIZE )
{
    mDynamicCast( QProcess*, process_, iodevice_ );

    char* end = &buffer_.front()+buffer_.size();
    setg( end, end, end );
}


qstreambuf::~qstreambuf()
{ }


std::streambuf::int_type qstreambuf::underflow()
{
#ifndef OD_NO_QT
    if ( gptr()<egptr() )
	return traits_type::to_int_type( *gptr() );

    char *base = &buffer_.front();
    char *start = base;

    if ( eback()==base )
    {
	memmove( base, egptr() - mPUSH_BACK, mPUSH_BACK );
	start += mPUSH_BACK;
    }

    readAll();

    size_t n = buffer_.size() - (start - base);
    if ( readbuffer_.size()<n )
	n = readbuffer_.size();

    if ( !n )
	return traits_type::eof();

    memmove( start, readbuffer_.data(), n );
    readbuffer_.remove( 0, n );

    setg( base, start, start + n );

    return traits_type::to_int_type( *gptr() );
#else
    return traits_type::eof();
#endif
}


void qstreambuf::detachDevice( bool readall )
{
    if ( readall )
	readAll();

    iodevice_ = 0;
    process_ = 0;
}


void qstreambuf::readAll()
{
#ifndef OD_NO_QT
    if ( iodevice_ ) iodevice_->waitForReadyRead( mTIMEOUT );
    QByteArray newdata;
    if ( process_ )
    {
       newdata = isstderr_ ? process_->readAllStandardError()
			   : process_->readAllStandardOutput();
    }
    else if ( iodevice_ )
    {
	newdata  = iodevice_->readAll();
    }

    readbuffer_.append( newdata );
#endif
}


std::streamsize qstreambuf::xsputn( const char_type* s, std::streamsize n )
{
#ifndef OD_NO_QT
    if ( !iodevice_ )
	return 0;

    const std::streamsize res = iodevice_->write( s, n );
    return iodevice_->waitForBytesWritten( mTIMEOUT ) ? res : 0;

#else
    return 0;
#endif
}
