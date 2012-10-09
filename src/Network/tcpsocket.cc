/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "tcpsocket.h"
#include "qtcpsocketcomm.h"
#include "iopar.h"

#define mInit \
    , comm_(new QTcpSocketComm(qtcpsocket_,this)) \
    , connected(this) \
    , disconnected(this) \
    , hostFound(this) \
    , readyRead(this) \
    , error(this) \
    , stateChanged(this)

TcpSocket::TcpSocket()
    : qtcpsocket_(new QTcpSocket)
    , id_(0)
    , isownsocket_(true)
    mInit
{}


TcpSocket::TcpSocket( QTcpSocket* qsocket, int id )
    : qtcpsocket_(qsocket)
    , id_(id)
    , isownsocket_(false)
    mInit
{}


TcpSocket::~TcpSocket()
{
    if ( isownsocket_ )
	delete qtcpsocket_;
    delete comm_;
}


const char* TcpSocket::errorMsg() const
{
    errmsg_ = qtcpsocket_->errorString().toAscii().constData();
    return errmsg_.buf();
}


void TcpSocket::connectToHost( const char* host, int port )
{ qtcpsocket_->connectToHost( QString(host), port ); }

void TcpSocket::disconnectFromHost()
{ qtcpsocket_->disconnectFromHost(); }

void TcpSocket::abort()
{ qtcpsocket_->abort(); }

bool TcpSocket::waitForConnected( int msec )
{ return qtcpsocket_->waitForConnected( msec ); }

bool TcpSocket::waitForReadyRead( int msec )
{ return qtcpsocket_->waitForReadyRead( msec ); }

void TcpSocket::readdata( char*& data, int len ) const
{ qtcpsocket_->read( data, len ); }


void TcpSocket::read( BufferString& str ) const
{
    QByteArray ba = qtcpsocket_->readAll();
    str = ba.data();
    ba.size();
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
    int ret = qtcpsocket_->write( str );
    qtcpsocket_->flush();
    return ret;
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
    int ret = qtcpsocket_->write( data, nr );
    qtcpsocket_->flush();
    return ret;
}


