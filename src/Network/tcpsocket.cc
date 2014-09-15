/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer
 Date:		Aug 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "tcpsocket.h"

#include "applicationdata.h"
#include "iopar.h"
#include "datainterp.h"
#include "netreqpacket.h"

#include <limits.h>

#ifndef OD_NO_QT
#include <QTcpSocket>
#include "qtcpsocketcomm.h"
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

TcpSocket::TcpSocket( bool haveevloop )
#ifndef OD_NO_QT
    : qtcpsocket_(new QTcpSocket)
#else
    : qtcpsocket_(0)
#endif
    , socketcomm_( 0 )
    , timeout_( 30000 )
    , noeventloop_( !haveevloop )
    , disconnected( this )
    , readyRead( this )
    , ownssocket_( true )
{
#ifndef OD_NO_QT
    socketcomm_ = new QTcpSocketComm( qtcpsocket_, this );
#endif
}


TcpSocket::TcpSocket( QTcpSocket* s, bool haveevloop )
#ifndef OD_NO_QT
    : qtcpsocket_(s)
#else
    : qtcpsocket_(0)
#endif
    , socketcomm_(0)
    , timeout_( 30000 )
    , noeventloop_( !haveevloop )
    , disconnected( this )
    , readyRead( this )
    , ownssocket_( false )
{
#ifndef OD_NO_QT
    socketcomm_ = new QTcpSocketComm( qtcpsocket_, this );
#endif
}


TcpSocket::~TcpSocket()
{
#ifndef OD_NO_QT
    delete socketcomm_;
    if ( ownssocket_ ) delete qtcpsocket_;
#endif
}


bool TcpSocket::connectToHost( const char* host, int port, bool wait )
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


bool TcpSocket::disconnectFromHost( bool wait )
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
	disconnected.trigger();

    return res;
}


bool TcpSocket::isBad() const
{
#ifdef OD_NO_QT
    return false;
#else
    const QAbstractSocket::SocketState state = qtcpsocket_->state();
    return state == QAbstractSocket::UnconnectedState
	|| state == QAbstractSocket::ClosingState;
#endif
}


bool TcpSocket::isConnected() const
{
#ifdef OD_NO_QT
    return false;
#else
    const QAbstractSocket::SocketState state = qtcpsocket_->state();
    return state == QAbstractSocket::ConnectedState;
#endif
}

od_int64 TcpSocket::bytesAvailable() const
{
#ifdef OD_NO_QT
    return false;
#else
    return qtcpsocket_->bytesAvailable();
#endif
}


void TcpSocket::abort()
{
#ifndef OD_NO_QT
    qtcpsocket_->abort();
#endif
}


static const od_int64 maxbuffersize = INT_MAX/2;


bool TcpSocket::writeArray( const void* voidbuf, od_int64 sz, bool wait )
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


bool TcpSocket::waitForWrite( bool forall ) const
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


bool TcpSocket::write( const OD::String& str )
{
    Threads::Locker locker( lock_ );

    return writeInt32( str.size() ) &&
	   writeArray( str.buf(), str.size(), true );
}


bool TcpSocket::write( const IOPar& par )
{
    BufferString str;
    par.dumpPretty( str );
    return write( str );
}


bool TcpSocket::write( const Network::RequestPacket& packet )
{
    if ( !packet.isOK() )
	return false;

    Threads::Locker locker( lock_ );
    return writeArray( packet.getRawHeader(), packet.headerSize(), true ) &&
	   writeArray( packet.payload(), packet.payloadSize(), true );

}


TcpSocket::ReadStatus TcpSocket::readArray( void* voidbuf, od_int64 sz ) const
{
#ifndef OD_NO_QT
    char* buf = (char*) voidbuf;

    if ( !waitForConnected() )
	return Timeout;

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
	    return ReadError;
	}

	buf += bytesread;
	bytestoread -= bytesread;

	if ( (!bytesread) || (bytestoread && noeventloop_) )
	{
	    if ( !waitForNewData() )
		return Timeout;
	}
    }

    return ReadOK;

#else
    return ReadError;
#endif
}


#define mReadWriteArrayImpl( Type, tp ) \
bool TcpSocket::write##Type##Array( const tp* arr, od_int64 sz,bool wait) \
{ return writeArray( (const void*) arr, sz*sizeof(tp), wait ); } \
\
bool TcpSocket::read##Type##Array( tp* arr, od_int64 sz ) const \
{ return readArray( (void*) arr, sz*sizeof(tp) ); } \
\
bool TcpSocket::write##Type( tp val ) \
{ return write##Type##Array( &val, 1 ); } \
\
bool TcpSocket::read##Type( tp& val ) const \
{ return read##Type##Array( &val, 1 ); }

mReadWriteArrayImpl( Short, short );
mReadWriteArrayImpl( Int32, od_int32 );
mReadWriteArrayImpl( Int64, od_int64 );
mReadWriteArrayImpl( Float, float );
mReadWriteArrayImpl( Double, double );

bool TcpSocket::read( BufferString& res ) const
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


bool TcpSocket::read( IOPar& res ) const
{
    BufferString str;
    if ( !read( str ) )
	return false;

    res.getFrom( str.buf() );
    return true;
}


TcpSocket::ReadStatus TcpSocket::read( Network::RequestPacket& packet ) const
{
    Threads::Locker locker( lock_ );

    ReadStatus res = readArray( packet.getRawHeader(),
			       Network::RequestPacket::headerSize() );
    if ( res!=ReadOK )
	return res;

    if ( !packet.isOK() )
    {
	errmsg_ = tr("Received packet is not OK");
	return ReadError;
    }

    const od_int32 payloadsize = packet.payloadSize();
    mDeclareAndTryAlloc( char*, payload, char[payloadsize] );

    if ( !payload )
    {
	errmsg_ = tr("Out of memory");
	return ReadError;
    }


    res = readArray( payload, payloadsize );

    if ( res!=ReadOK )
    {
	delete [] payload;
	return ReadError;
    }

    packet.setPayload( payload );

    return ReadOK;
}



bool TcpSocket::waitForConnected() const
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


bool TcpSocket::waitForNewData() const
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
