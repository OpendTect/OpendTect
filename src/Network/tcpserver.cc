/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "tcpserver.h"

#include "qtcpservercomm.h"
#include "tcpsocket.h"

#include <QTcpSocket>

static int sockid = 0;

static int getNewID()
{
    return ++sockid;
}


TcpServer::TcpServer()
    : qtcpserver_(new QTcpServer)
    , comm_(new QTcpServerComm(qtcpserver_,this))
    , newConnection(this)
    , readyRead(this)
{
    newConnection.notify( mCB(this,TcpServer,newConnectionCB) );
}


TcpServer::~TcpServer()
{
    if ( isListening() )
	close();

    delete qtcpserver_;
    delete comm_;
    deepErase( sockets2bdeleted_ );
}


bool TcpServer::listen( const char* host, int prt )
{
    return qtcpserver_->listen(
	    host ? QHostAddress(host) : QHostAddress(QHostAddress::Any), prt );
}


bool TcpServer::isListening() const
{ return qtcpserver_->isListening(); }

int TcpServer::port() const
{ return qtcpserver_->serverPort(); }

void TcpServer::close()
{ qtcpserver_->close(); }

const char* TcpServer::errorMsg() const
{
    errmsg_ = qtcpserver_->errorString().toLatin1().constData();
    return errmsg_.buf();
}


bool TcpServer::hasPendingConnections() const
{ return qtcpserver_->hasPendingConnections(); }

QTcpSocket* TcpServer::nextPendingConnection()
{ return qtcpserver_->nextPendingConnection(); }


void TcpServer::newConnectionCB( CallBacker* )
{
    if ( !hasPendingConnections() )
	return;

    TcpSocket* tcpsocket = new TcpSocket( nextPendingConnection(), getNewID() );
    tcpsocket->readyRead.notify( mCB(this,TcpServer,readyReadCB));
    tcpsocket->disconnected.notify( mCB(this,TcpServer,disconnectCB) );
    sockets_ += tcpsocket;
}


void TcpServer::readyReadCB( CallBacker* cb )
{
    mDynamicCastGet(TcpSocket*,socket,cb);
    if ( !socket ) return;

    readyRead.trigger( socket->getID() );
}


void TcpServer::disconnectCB( CallBacker* cb )
{
    mDynamicCastGet(TcpSocket*,socket,cb);
    if ( !socket ) return;

    socket->readyRead.remove( mCB(this,TcpServer,readyReadCB) );
    socket->disconnected.remove( mCB(this,TcpServer,disconnectCB) );
    sockets_ -= socket;
    sockets2bdeleted_ += socket;
}


void TcpServer::read( int id, BufferString& data ) const
{
    TcpSocket* socket = getSocket( id );
    if ( !socket ) return;
	socket->read( data );
}


void TcpServer::read( int id, IOPar& par ) const
{
    TcpSocket* socket = getSocket( id );
    if ( !socket ) return;
	socket->read( par );
}


int TcpServer::write( int id, const IOPar& par )
{
    TcpSocket* socket = getSocket( id );
    return socket ? socket->write( par ) : 0;
}


int TcpServer::write( int id, const char* str )
{
    TcpSocket* socket = getSocket( id );
    return socket ? socket->write( str ) : 0;
}


TcpSocket* TcpServer::getSocket( int id ) const
{
    for ( int idx=0; idx<sockets_.size(); idx++ )
    {
	if ( sockets_[idx]->getID() == id )
	    return (TcpSocket*)sockets_[idx];
    }

    return 0;
}
