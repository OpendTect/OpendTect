/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer
 Date:		Aug 2014
________________________________________________________________________

-*/

#include "netsocket.h"

#include "applicationdata.h"
#include "iopar.h"
#include "datainterp.h"
#include "netreqpacket.h"
#include "systeminfo.h"

#include <limits.h>

#include "qtcpsocketcomm.h"

# define mCheckThread \
    if ( thread_!=Threads::currentThread() ) \
    { \
      pErrMsg("Invalid Thread access" ); \
    }


Network::Socket::Socket( bool haveevloop )
    : qtcpsocket_(new QTcpSocket)
    , socketcomm_( 0 )
    , timeout_( 30000 )
    , noeventloop_( !haveevloop )
    , disconnected( this )
    , error( this )
    , readyRead( this )
    , ownssocket_( true )
    , thread_( Threads::currentThread() )
{
    socketcomm_ = new QTcpSocketComm( qtcpsocket_, this );
}


Network::Socket::Socket( QTcpSocket* s, bool haveevloop )
    : qtcpsocket_(s)
    , socketcomm_(0)
    , timeout_( 30000 )
    , noeventloop_( !haveevloop )
    , disconnected( this )
    , readyRead( this )
    , ownssocket_( false )
    , error( this )
    , thread_( Threads::currentThread() )
{
    socketcomm_ = new QTcpSocketComm( qtcpsocket_, this );
}


Network::Socket::~Socket()
{
    socketcomm_->disconnect();
    mCheckThread;
    socketcomm_->deleteLater();
    if ( ownssocket_ ) qtcpsocket_->deleteLater();
}


bool Network::Socket::connectToHost( const Authority& auth, bool wait )
{
    mCheckThread;

    if ( noeventloop_ )
	wait = true;

    const QAbstractSocket::SocketState state = qtcpsocket_->state();
    if ( state != QAbstractSocket::UnconnectedState )
    {
	errmsg_ = tr("Trying to connect already used connection.");
	return false;
    }

    const PortNr_Type port = auth.getPort();
    if ( auth.addressIsValid() )
	qtcpsocket_->connectToHost( auth.qhostaddr_, port );
    else
	qtcpsocket_->connectToHost( auth.qhost_, port );

    if ( wait )
	return waitForConnected();

    return true;
}


bool Network::Socket::disconnectFromHost( bool wait )
{
    mCheckThread;
    if ( !isConnected() )
	return true;

    if ( noeventloop_ )
	wait = true;

    bool res = true;

    qtcpsocket_->disconnectFromHost();
    const QAbstractSocket::SocketState state = qtcpsocket_->state();
    if ( wait && state != QAbstractSocket::UnconnectedState )
    {
	res = qtcpsocket_->waitForDisconnected( timeout_ );
	if ( !res )
	    errmsg_.setFrom( qtcpsocket_->errorString() );
    }

    if ( res )
	disconnected.trigger();

    return res;
}


bool Network::Socket::isBad() const
{
    const QAbstractSocket::SocketState state = qtcpsocket_->state();
    return state == QAbstractSocket::UnconnectedState
	|| state == QAbstractSocket::ClosingState;
}


bool Network::Socket::isConnected() const
{
    const QAbstractSocket::SocketState state = qtcpsocket_->state();
    return state == QAbstractSocket::ConnectedState;
}

od_int64 Network::Socket::bytesAvailable() const
{
    od_int64 res = qtcpsocket_->bytesAvailable();
    if ( res || !noeventloop_ )
	return res;

    //Force a trigger if noeventloop. Not really needed, but
    //Makes it safer. Could be any time, but 1ms will do.
    qtcpsocket_->waitForReadyRead( 1 );

    return qtcpsocket_->bytesAvailable();
}


void Network::Socket::abort()
{
    mCheckThread;
    qtcpsocket_->abort();
}

// 2000 is a margin. Qt has a margin of 32, but we don't wish to
// come near as it is an assertion.
static const od_int64 maxbuffersize = ((INT_MAX/2-2000));


bool Network::Socket::writeArray( const void* voidbuf, od_int64 sz, bool wait )
{
    mCheckThread;
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
}


bool Network::Socket::waitForWrite( bool forall ) const
{
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

    return true;
}


bool Network::Socket::write( const OD::String& str )
{
    Threads::Locker locker( lock_ );

    return writeInt32( str.size() ) &&
	   writeArray( str.buf(), str.size(), true );
}


bool Network::Socket::write( const IOPar& par )
{
    BufferString str;
    par.putTo( str );
    return write( str );
}


bool Network::Socket::write( const Network::RequestPacket& pkt, bool waitfor )
{
    if ( !pkt.isOK() )
	return false;

#ifdef __debug__
    const int maxsz = RequestPacket::systemSizeLimit();
    if ( maxsz > 0 && pkt.totalSize() > maxsz )
	{ pErrMsg( BufferString("Pkt exceeds max size: ",pkt.totalSize()) ); }
#endif

    Threads::Locker locker( lock_ );
    if ( !writeArray( pkt.getRawHeader(), pkt.headerSize(), waitfor ) ||
	   !writeArray( pkt.payload(), pkt.payloadSize(), waitfor ) )
	return false;

    if ( waitfor )
    {
	//TODO implement
    }
    return true;
}


Network::Socket::ReadStatus Network::Socket::readArray( void* voidbuf,
							od_int64 sz ) const
{
    mCheckThread;
    char* buf = (char*)voidbuf;

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
}


#define mReadWriteArrayImpl( Type, tp ) \
bool Network::Socket::write##Type##Array( const tp* arr,od_int64 sz,bool wait) \
{ return writeArray( (const void*) arr, sz*sizeof(tp), wait ); } \
\
bool Network::Socket::read##Type##Array( tp* arr, od_int64 sz ) const \
{ return readArray( (void*) arr, sz*sizeof(tp) )==Network::Socket::ReadOK; } \
\
bool Network::Socket::write##Type( tp val ) \
{ return write##Type##Array( &val, 1 ); } \
\
bool Network::Socket::read##Type( tp& val ) const \
{ return read##Type##Array( &val, 1 ); }

mReadWriteArrayImpl( Short, short );
mReadWriteArrayImpl( Int32, od_int32 );
mReadWriteArrayImpl( Int64, od_int64 );
mReadWriteArrayImpl( Float, float );
mReadWriteArrayImpl( Double, double );

bool Network::Socket::read( BufferString& res ) const
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

    if ( readArray( res.getCStr(), nrchars )!=Network::Socket::ReadOK )
	return false;

    res.getCStr()[nrchars] = 0;
    return true;
}


bool Network::Socket::read( IOPar& res ) const
{
    BufferString str;
    if ( !read( str ) )
	return false;

    res.getFrom( str.buf() );
    return true;
}


Network::Socket::ReadStatus Network::Socket::read(
				Network::RequestPacket& pkt ) const
{
    Threads::Locker locker( lock_ );

    ReadStatus res = readArray( pkt.getRawHeader(),
			       Network::RequestPacket::headerSize() );
    if ( res!=ReadOK )
	return res;

    if ( !pkt.isOK() )
    {
	errmsg_ = tr("Received packet is not OK");
	return ReadError;
    }

    const od_int32 payloadsize = pkt.payloadSize();
    if ( payloadsize > 0 )
    {
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

	pkt.setPayload( payload, payloadsize );
    }

    return ReadOK;
}



bool Network::Socket::waitForConnected() const
{
    if ( isConnected() )
	return true;

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
    return false;
}


bool Network::Socket::waitForNewData() const
{
    while ( !qtcpsocket_->bytesAvailable() )
    {
	qtcpsocket_->waitForReadyRead( timeout_ );

	if ( !qtcpsocket_->bytesAvailable() )
	{
	    errmsg_ = tr("Read timeout");
	    return false;
	}
    }

    return true;
}
