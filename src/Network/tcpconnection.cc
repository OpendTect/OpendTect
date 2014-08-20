/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		March 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "tcpconnection.h"

#include "applicationdata.h"
#include "iopar.h"
#include "datainterp.h"

#include <limits.h>

#ifndef OD_NO_QT
#include <QTcpSocket>
#endif

TcpConnection::TcpConnection()
#ifndef OD_NO_QT
    : qtcpsocket_(new QTcpSocket)
#else
    : qtcpsocket_(0)
#endif
    , timeout_( 30000 )
    , noeventloop_( false )
    , shortinterpreter_( 0 )
    , od_int32interpreter_( 0 )
    , od_int64interpreter_( 0 )
    , floatinterpreter_( 0 )
    , doubleinterpreter_( 0 )
{}


TcpConnection::~TcpConnection()
{
#ifndef OD_NO_QT
    delete qtcpsocket_;
#endif
    deleteAndZeroPtr( shortinterpreter_ );
    deleteAndZeroPtr( od_int32interpreter_ );
    deleteAndZeroPtr( od_int64interpreter_ );
    deleteAndZeroPtr( floatinterpreter_ );
    deleteAndZeroPtr( doubleinterpreter_ );
}


void TcpConnection::setNoEventLoop( bool yn )
{ noeventloop_ = yn; }


bool TcpConnection::connectToHost( const char* host, int port, bool wait )
{
#ifndef OD_NO_QT
    if ( noeventloop_ ) wait = true;

    if ( qtcpsocket_->state()!=QAbstractSocket::UnconnectedState )
    {
	errmsg_ = tr("Trying to connect used connection.");
	return false;
    }

    qtcpsocket_->connectToHost( QString(host), port );

    if ( wait )
	return waitForConnected();
#endif

    return true;
}


bool TcpConnection::disconnectFromHost( bool wait )
{
#ifndef OD_NO_QT
    if ( noeventloop_ ) wait = true;

    qtcpsocket_->disconnectFromHost();

    if ( wait )
    {
	const bool res = qtcpsocket_->waitForDisconnected( timeout_ );
	if ( !res )
	    errmsg_.setFrom( qtcpsocket_->errorString() );

	return res;
    }
#endif

    return true;
}


void TcpConnection::abort()
{
#ifndef OD_NO_QT
    qtcpsocket_->abort();
#endif
}


static const od_int64 maxbuffersize = INT_MAX/2;


bool TcpConnection::writeArray( const char* buf, od_int64 sz, bool wait )
{
#ifndef OD_NO_QT
    if ( noeventloop_ ) wait = true;

    if ( !waitForConnected() )
	return false;

    Threads::MutexLocker locker( lock_ );
    od_int64 bytestowrite = sz;

    while ( bytestowrite )
    {
	od_int64 writesize = bytestowrite;
	bool forcewait = wait;
	if ( bytestowrite+qtcpsocket_->bytesToWrite()>maxbuffersize )
	{
	    writesize = maxbuffersize-qtcpsocket_->bytesToWrite();
	    forcewait = true;
	}

	const od_int64 byteswritten = qtcpsocket_->write( buf, writesize );
	if ( byteswritten == -1 )
	{
	    errmsg_.setFrom( qtcpsocket_->errorString() );
	    return false;
	}

	buf += byteswritten;
	bytestowrite -= byteswritten;

	if ( forcewait )
	{
	    if ( !waitForWrite( false ) )
		return false;
	}
    }

    if ( wait )
    {
	if ( !waitForWrite( true ) )
	    return false;
    }

    return true;

#else
    return false;
#endif
}


bool TcpConnection::waitForWrite( bool forall )
{
#ifndef OD_NO_QT
    while ( qtcpsocket_->bytesToWrite() )
    {
	od_int64 oldpayload = qtcpsocket_->bytesToWrite();

	lock_.unLock();
	qtcpsocket_->waitForBytesWritten( timeout_ );
	lock_.lock();

	if ( oldpayload==qtcpsocket_->bytesToWrite() )
	{
	    errmsg_ = tr("Write timeout");
	    return false;
	}

	if ( !forall )
	    break;
    }
#endif

    return true;
}


bool TcpConnection::write( const OD::String& str )
{
    return writeShort( str.size() ) &&
	   writeArray( str.buf(), str.size(), false );
}


template <class T>
bool TcpConnection::writeArray( const DataInterpreter<T>* interpreter,
				const T* arr, od_int64 sz, bool wait )
{
    const char* usedarr = (char*) const_cast<T*>(arr);
    const od_int64 nrbytes = sz*sizeof(T);
    ArrPtrMan<char> writearr = 0;

    if ( interpreter )
    {
	writearr = new char[nrbytes];
	if ( !writearr )
	{
	    errmsg_ = tr("Out of memory");
	    return false;
	}

	for ( od_int64 idx=0; idx<sz; idx++ )
	    interpreter->put( writearr, idx, arr[idx] );

	usedarr = writearr;
    }

    return writeArray( usedarr, nrbytes, wait );
}

bool TcpConnection::readArray( char* buf, od_int64 sz )
{
#ifndef OD_NO_QT
    if ( !waitForConnected() )
	return false;

    Threads::MutexLocker locker( lock_ );

    od_int64 bytestoread = sz;

    while ( bytestoread )
    {
	od_int64 readsize = bytestoread;
	if ( readsize>maxbuffersize )
	    readsize = maxbuffersize;

	const od_int64 bytesread = qtcpsocket_->read( buf, readsize );
	if ( bytesread == -1 )
	{
	    errmsg_.setFrom( qtcpsocket_->errorString() );
	    return false;
	}

	buf += bytesread;
	bytestoread -= bytesread;

	if ( (!bytesread) || (bytestoread && noeventloop_) )
	{
	    if ( !waitForNewData() )
		return false;
	}
    }

    return true;

#else
    return false;
#endif
}


template <class T>
bool TcpConnection::readArray( const DataInterpreter<T>* interpreter,
			       T* arr, od_int64 sz )
{
    const od_int64 nrbytes = sz*sizeof(T);
    if ( !readArray( (char*) arr, nrbytes ) )
	return false;

    if ( interpreter )
    {
	for ( od_int64 idx=0; idx<sz; idx++ )
	    arr[idx] = interpreter->get( arr, idx );
    }

    return true;
}


#define mReadWriteArrayImpl( Type, tp ) \
bool TcpConnection::write##Type##Array( const tp* arr, od_int64 sz,bool wait) \
{ return writeArray<tp>( tp##interpreter_, arr, sz, wait ); } \
\
bool TcpConnection::read##Type##Array( tp* arr, od_int64 sz ) \
{ return readArray<tp>( tp##interpreter_, arr, sz ); } \
\
bool TcpConnection::write##Type( tp val ) \
{ return write##Type##Array( &val, 1 ); } \
\
bool TcpConnection::read##Type( tp& val ) \
{ return read##Type##Array( &val, 1 ); }

mReadWriteArrayImpl( Short, short );
mReadWriteArrayImpl( Int32, od_int32 );
mReadWriteArrayImpl( Int64, od_int64 );
mReadWriteArrayImpl( Float, float );
mReadWriteArrayImpl( Double, double );

bool TcpConnection::read( BufferString& res )
{
    short nrchars;
    if ( !readShort( nrchars ) )
	return false;

    if ( nrchars<0 )
	return true;

    if ( !res.setBufSize( nrchars+1 ) )
    {
	errmsg_ = tr("Out of memory");
	return false;
    }

    if ( !readArray( res.getCStr(), nrchars ) )
	return false;

    res.getCStr()[nrchars] = 0;
    return true;
}


bool TcpConnection::waitForConnected()
{
#ifndef OD_NO_QT
    if ( qtcpsocket_->state()==QAbstractSocket::ConnectedState )
	return true;

    //Have we started at something?
    if ( qtcpsocket_->state()>QAbstractSocket::UnconnectedState )
    {
	qtcpsocket_->waitForConnected( timeout_ );
	if ( qtcpsocket_->state()==QAbstractSocket::ConnectedState )
	    return true;

	errmsg_.setFrom( qtcpsocket_->errorString() );

	return false;
    }

    errmsg_ = tr("No connection");
#endif
    return false;
}


bool TcpConnection::waitForNewData()
{
#ifndef OD_NO_QT
    while ( !qtcpsocket_->bytesAvailable() )
    {
	lock_.unLock();
	qtcpsocket_->waitForReadyRead( timeout_ );
	lock_.lock();

	if ( !qtcpsocket_->bytesAvailable() )
	{
	    errmsg_ = tr("Read timeout");
	    return false;
	}
    }
#endif

    return true;
}
