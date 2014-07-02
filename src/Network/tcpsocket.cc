/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "tcpsocket.h"
#ifndef OD_NO_QT
#include "qtcpsocketcomm.h"
#endif
#include "iopar.h"

#define mInit \
    , connected(this) \
    , disconnected(this) \
    , hostFound(this) \
    , readyRead(this) \
    , error(this) \
    , stateChanged(this)

TcpSocket::TcpSocket()
#ifndef OD_NO_QT
    : qtcpsocket_(new QTcpSocket)
    , comm_(new QTcpSocketComm(qtcpsocket_,this))
#else
    : qtcpsocket_(0)
    , comm_(0)
#endif
    , id_(0)
    , isownsocket_(true)
    mInit
{}


TcpSocket::TcpSocket( QTcpSocket* qsocket, int id )
#ifndef OD_NO_QT
    : qtcpsocket_(qsocket)
    , comm_(new QTcpSocketComm(qtcpsocket_,this))
#else
    : qtcpsocket_(0)
    , comm_(0)
#endif
    , id_(id)
    , isownsocket_(false)
    mInit
{}


TcpSocket::~TcpSocket()
{
#ifndef OD_NO_QT
    if ( isownsocket_ )
	delete qtcpsocket_;

    delete comm_;
#endif
}


const char* TcpSocket::errorMsg() const
{
#ifndef OD_NO_QT
    errmsg_ = qtcpsocket_->errorString().toLatin1().constData();
#endif
    return errmsg_.buf();
}


void TcpSocket::connectToHost( const char* host, int port )
{
#ifndef OD_NO_QT
    qtcpsocket_->connectToHost( QString(host), port );
#else
    return;
#endif
}

void TcpSocket::disconnectFromHost()
{
#ifndef OD_NO_QT
    qtcpsocket_->disconnectFromHost();
#else
    return;
#endif
}

void TcpSocket::abort()
{
#ifndef OD_NO_QT
    qtcpsocket_->abort();
#else
    return;
#endif
}

bool TcpSocket::waitForConnected( int msec )
{
#ifndef OD_NO_QT
    return qtcpsocket_->waitForConnected( msec );
#else
    return false;
#endif
}

bool TcpSocket::waitForReadyRead( int msec )
{
#ifndef OD_NO_QT
    return qtcpsocket_->waitForReadyRead( msec );
#else
    return false;
#endif
}

void TcpSocket::readdata( char*& data, int len ) const
{
#ifndef OD_NO_QT
    qtcpsocket_->read( data, len );
#else
    return;
#endif
}


void TcpSocket::read( BufferString& str ) const
{
#ifndef OD_NO_QT
    QByteArray ba = qtcpsocket_->readAll();
    str = ba.data();
    ba.size();
#else
    return;
#endif
}


void TcpSocket::read( IOPar& par ) const
{
    BufferString buf;
    read( buf );
    par.getFrom( buf );
}


void TcpSocket::read( int& val ) const
{
    BufferString buf;
    read( buf );
    getFromString( val, buf.buf() );
}


void TcpSocket::read( bool& yn ) const
{
    BufferString buf;
    read( buf );
    getFromString( yn, buf.buf() );
}


int TcpSocket::write( const char* str )
{
#ifndef OD_NO_QT
    int ret = qtcpsocket_->write( str );
    qtcpsocket_->flush();
    return ret;
#else
    return 0;
#endif
}


int TcpSocket::write( const IOPar& par ) 
{
    BufferString buf;
    par.putTo( buf );
    return write( buf.buf() ); 
}


int TcpSocket::write( const int& val ) 
{
    BufferString buf;
    buf = val;
    return write( buf.buf() ); 
}


int TcpSocket::write( bool yn ) 
{
    BufferString buf;
    buf = toString(yn);
    return write( buf.buf() ); 
}


int TcpSocket::writedata( const char* data, int nr ) 
{
#ifndef OD_NO_QT
    int ret = qtcpsocket_->write( data, nr );
    qtcpsocket_->flush();
    return ret;
#else
    return 0;
#endif
}


