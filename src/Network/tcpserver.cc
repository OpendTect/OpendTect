/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: tcpserver.cc,v 1.6 2010-05-20 09:46:53 cvsranojay Exp $";

#include "tcpserver.h"
#include "qtcpservercomm.h"
#include "tcpsocket.h"

#include <QTcpSocket>


TcpServer::TcpServer()
    : qtcpserver_(new QTcpServer)
    , comm_(new QTcpServerComm(qtcpserver_,this))
    , newConnection(this)
    , readyRead(this)
    , tcpsocket_(0)
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


bool TcpServer::listen( const char* host, int port )
{
    return qtcpserver_->listen(
	    host ? QHostAddress(host) : QHostAddress(QHostAddress::Any), port );
}


bool TcpServer::isListening() const
{ return qtcpserver_->isListening(); }

void TcpServer::close()
{ qtcpserver_->close(); }

const char* TcpServer::errorMsg() const
{
    errmsg_ = qtcpserver_->errorString().toAscii().constData();
    return errmsg_.buf();
}


bool TcpServer::hasPendingConnections() const
{ return qtcpserver_->hasPendingConnections(); }

QTcpSocket* TcpServer::nextPendingConnection()
{ return qtcpserver_->nextPendingConnection(); }


int TcpServer::write( const char* str )
{
    return tcpsocket_ ? tcpsocket_->write( str ) : 0;
}


void TcpServer::newConnectionCB( CallBacker* )
{
    if ( !hasPendingConnections() )
	return;

    if ( !tcpsocket_ )
    {
	tcpsocket_ = new TcpSocket( nextPendingConnection() );
	tcpsocket_->readyRead.notify( mCB(this,TcpServer,readyReadCB));
	tcpsocket_->disconnected.notify( mCB(this,TcpServer,disconnectCB) );
    }
}


void TcpServer::readyReadCB( CallBacker* )
{
    readyRead.trigger();
}


void TcpServer::disconnectCB( CallBacker* )
{
    if ( tcpsocket_ )
    {
	tcpsocket_->readyRead.remove( mCB(this,TcpServer,readyReadCB) );
	tcpsocket_->disconnected.remove( mCB(this,TcpServer,disconnectCB) );
    }
    sockets2bdeleted_ += tcpsocket_;
    tcpsocket_ = 0;
}


void TcpServer::read( BufferString& data ) const
{
    if ( tcpsocket_ )
	tcpsocket_->read( data );
}


void TcpServer::read( IOPar& par ) const
{
   tcpsocket_->read( par );
}


int TcpServer::write( const IOPar& par ) const
{
    return tcpsocket_->write( par );
}

