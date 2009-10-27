/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: tcpserver.cc,v 1.3 2009-10-27 03:22:20 cvsnanne Exp $";

#include "tcpserver.h"
#include "qtcpservercomm.h"

#include <QTcpSocket>


TcpServer::TcpServer()
    : qtcpserver_(new QTcpServer)
    , comm_(new QTcpServerComm(qtcpserver_,this))
    , newConnection(this)
{}


TcpServer::~TcpServer()
{
    if ( isListening() )
	close();

    delete qtcpserver_;
    delete comm_;
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


int TcpServer::write( const char* str )
{
    QTcpSocket* qsocket = nextPendingConnection();
    return qsocket ? qsocket->write( str, 1024 ) : 0;
}


bool TcpServer::hasPendingConnections() const
{ return qtcpserver_->hasPendingConnections(); }

QTcpSocket* TcpServer::nextPendingConnection()
{ return qtcpserver_->nextPendingConnection(); }
