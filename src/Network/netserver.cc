/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		March 2009
________________________________________________________________________

-*/

#include "netserver.h"
#include "netsocket.h"

#ifndef OD_NO_QT
#include "qtcpservercomm.h"
#endif

static int sockid = 0;

static int getNewID()
{
    return ++sockid;
}


Network::Server::Server()
#ifndef OD_NO_QT
    : qtcpserver_(new QTcpServer)
    , comm_(new QTcpServerComm(qtcpserver_,this))
#else
    : qtcpserver_(0)
    , comm_(0)
#endif
    , newConnection(this)
    , readyRead(this)
{ }


Network::Server::~Server()
{
    detachAllNotifiers();
    if ( isListening() )
	close();
#ifndef OD_NO_QT
    delete qtcpserver_;
    delete comm_;
#endif
    deepErase( sockets2bdeleted_ );
}


bool Network::Server::listen( const char* host, int prt )
{
#ifndef OD_NO_QT
    return qtcpserver_->listen(
	    host ? QHostAddress(host) : QHostAddress(QHostAddress::Any), prt );
#else
    return false;
#endif
}


bool Network::Server::isListening() const
{
#ifndef OD_NO_QT
    return qtcpserver_->isListening();
#else
    return false;
#endif
}

int Network::Server::port() const
{
#ifndef OD_NO_QT
    return qtcpserver_->serverPort();
#else
    return 0;
#endif
}

void Network::Server::close()
{
#ifndef OD_NO_QT
    qtcpserver_->close();
#endif
}

const char* Network::Server::errorMsg() const
{
#ifndef OD_NO_QT
    errmsg_ = qtcpserver_->errorString().toLatin1().constData();
    return errmsg_.buf();
#else
    return 0;
#endif
}


bool Network::Server::hasPendingConnections() const
{
#ifndef OD_NO_QT
    return qtcpserver_->hasPendingConnections();
#else
    return false;
#endif
}

QTcpSocket* Network::Server::nextPendingConnection()
{
#ifndef OD_NO_QT
    return qtcpserver_->nextPendingConnection();
#else
    return 0;
#endif
}


void Network::Server::notifyNewConnection()
{
    if ( !hasPendingConnections() )
	return;

    Network::Socket* tcpsocket = new Network::Socket( nextPendingConnection() );
    tcpsocket->readyRead.notify( mCB(this,Server,readyReadCB));
    mAttachCB( tcpsocket->disconnected, Network::Server::disconnectCB);
    const int id = getNewID();
    sockets_ += tcpsocket;
    ids_ += id;

    newConnection.trigger( id );
}


void Network::Server::readyReadCB( CallBacker* cb )
{
    mDynamicCastGet(Network::Socket*,socket,cb);
    if ( !socket ) return;

    const int idx = sockets_.indexOf( socket );
    if ( idx<0 )
	return;

    readyRead.trigger( ids_[idx] );
}


void Network::Server::disconnectCB( CallBacker* cb )
{
    mDynamicCastGet(Network::Socket*,socket,cb);
    if ( !socket ) return;

    //socket->readyRead.remove( mCB(this,Server,readyReadCB) );
    const int idx = sockets_.indexOf( socket );
    if ( idx>=0 )
    {
	sockets_.removeSingle( idx );
	ids_.removeSingle( idx );
    }

    sockets2bdeleted_ += socket;
}


void Network::Server::read( int id, BufferString& data ) const
{
    const Network::Socket* socket = getSocket( id );
    if ( !socket ) return;
	socket->read( data );
}


void Network::Server::read( int id, IOPar& par ) const
{
    const Network::Socket* socket = getSocket( id );
    if ( !socket ) return;
	socket->read( par );
}


int Network::Server::write( int id, const IOPar& par )
{
    Network::Socket* socket = getSocket( id );
    return socket ? socket->write( par ) : 0;
}


int Network::Server::write( int id, const char* str )
{
    Network::Socket* socket = getSocket( id );
    return socket ? socket->write( FixedString(str) ) : 0;
}


Network::Socket* Network::Server::getSocket( int id )
{
    const int idx = ids_.indexOf( id );
    if ( idx<0 )
	return 0;

    return sockets_[idx];
}


const Network::Socket* Network::Server::getSocket( int id ) const
{ return const_cast<Server*>( this )->getSocket( id ); }


bool Network::Server::waitForNewConnection( int msec )
{
#ifndef OD_NO_QT
    return qtcpserver_->waitForNewConnection( msec );
#else
    return false;
#endif
}
