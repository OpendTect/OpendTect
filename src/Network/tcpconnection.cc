/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer
 Date:		Aug 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "tcpconnection.h"

#include "applicationdata.h"
#include "iopar.h"
#include "datainterp.h"
#include "netreqpacket.h"

#include <limits.h>

#ifndef OD_NO_QT
#include <QTcpSocket>
#endif

/*

   From the manual:

   QAbstractSocket::UnconnectedState
	0	The socket is not connected.
   QAbstractSocket::HostLookupState
	1	The socket is performing a host name lookup.
   QAbstractSocket::ConnectingState
	2	The socket has started establishing a connection.
   QAbstractSocket::ConnectedState
	3	A connection is established.
   QAbstractSocket::BoundState
	4	The socket is bound to an address and port.
   QAbstractSocket::ClosingState
	6	The socket is about to close (data may still be waiting to
		be written).
   QAbstractSocket::ListeningState
	5	For internal use only.

   */

TcpConnection::TcpConnection( bool haveevloop )
#ifndef OD_NO_QT
    : qtcpsocket_(new QTcpSocket)
#else
    : qtcpsocket_(0)
#endif
    , timeout_( 30000 )
    , noeventloop_( !haveevloop )
    , shortinterpreter_( 0 )
    , od_int32interpreter_( 0 )
    , od_int64interpreter_( 0 )
    , floatinterpreter_( 0 )
    , doubleinterpreter_( 0 )
    , Closed(this)
{
}


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


bool TcpConnection::connectToHost( const char* host, int port, bool wait )
{
#ifdef OD_NO_QT
    return false;
#else
    if ( noeventloop_ )
	wait = true;

    const QAbstractSocket::SocketState state = qtcpsocket_->state();
    if ( state != QAbstractSocket::UnconnectedState )
    {
	errmsg_ = tr("Trying to connect already used connection.");
	return false;
    }

    qtcpsocket_->connectToHost( QString(host), port );

    if ( wait )
	return waitForConnected();

    return true;
#endif
}


bool TcpConnection::disconnectFromHost( bool wait )
{
    if ( noeventloop_ )
	wait = true;

    bool res = true;

#ifndef OD_NO_QT

    qtcpsocket_->disconnectFromHost();

    if ( wait )
    {
	res = qtcpsocket_->waitForDisconnected( timeout_ );
	if ( !res )
	    errmsg_.setFrom( qtcpsocket_->errorString() );
    }

#endif

    if ( res )
	Closed.trigger();

    return res;
}


bool TcpConnection::isBad() const
{
#ifdef OD_NO_QT
    return false;
#else
    const QAbstractSocket::SocketState state = qtcpsocket_->state();
    return state == QAbstractSocket::UnconnectedState
	|| state == QAbstractSocket::ClosingState;
#endif
}


bool TcpConnection::isConnected() const
{
#ifdef OD_NO_QT
    return false;
#else
    const QAbstractSocket::SocketState state = qtcpsocket_->state();
    return state == QAbstractSocket::ConnectedState;
#endif
}

bool TcpConnection::anythingToRead() const
{
#ifdef OD_NO_QT
    return false;
#else
    return qtcpsocket_->bytesAvailable();
#endif
}


void TcpConnection::abort()
{
#ifndef OD_NO_QT
    qtcpsocket_->abort();
#endif
}


static const od_int64 maxbuffersize = INT_MAX/2;


bool TcpConnection::writeArray( const void* voidbuf, od_int64 sz, bool wait )
{
#ifndef OD_NO_QT
    const char* buf = (const char*) voidbuf;
    if ( noeventloop_ )
	wait = true;

    if ( !waitForConnected() )
	return false;

    Threads::Locker locker( lock_ );
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

	qtcpsocket_->waitForBytesWritten( timeout_ );

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
    Threads::Locker locker( lock_ );

    return writeInt32( str.size() ) &&
	   writeArray( str.buf(), str.size(), true );
}


bool TcpConnection::write( const Network::RequestPacket& packet )
{
    if ( !packet.isOK() )
	return false;

    Threads::Locker locker( lock_ );
    return writeArray( packet.getRawHeader(), packet.headerSize(), true ) &&
	   writeArray( packet.payload(), packet.payloadSize(), true );

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

bool TcpConnection::readArray( void* voidbuf, od_int64 sz )
{
#ifndef OD_NO_QT
    char* buf = (char*) voidbuf;

    if ( !waitForConnected() )
	return false;

    Threads::Locker locker( lock_ );

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
    int nrchars;
    if ( !readInt32( nrchars ) )
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


bool TcpConnection::read( Network::RequestPacket& packet )
{
    Threads::Locker locker( lock_ );

    if ( !readArray( packet.getRawHeader(),
		     Network::RequestPacket::headerSize() ) )
	return false;

    if ( !packet.isOK() )
    {
	errmsg_ = tr("Received packet is not OK");
	return false;
    }

    const od_int32 payloadsize = packet.payloadSize();
    mDeclareAndTryAlloc( char*, payload, char[payloadsize] );
    packet.setPayload( payload );

    if ( !payload )
    {
	errmsg_ = tr("Out of memory");
	return false;
    }

    if ( !readArray( payload, payloadsize ) )
    {
	delete [] payload;
	return false;
    }

    return true;
}



bool TcpConnection::waitForConnected()
{
    if ( isConnected() )
	return true;

#ifndef OD_NO_QT

    //Have we started at something?
    QAbstractSocket::SocketState state = qtcpsocket_->state();
    if ( state > QAbstractSocket::UnconnectedState )
    {
	qtcpsocket_->waitForConnected( timeout_ );
	state = qtcpsocket_->state();
	if ( state == QAbstractSocket::ConnectedState )
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
	qtcpsocket_->waitForReadyRead( timeout_ );

	if ( !qtcpsocket_->bytesAvailable() )
	{
	    errmsg_ = tr("Read timeout");
	    return false;
	}
    }
#endif

    return true;
}
