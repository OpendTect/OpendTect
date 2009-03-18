/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: tcpserver.cc,v 1.1 2009-03-18 04:24:39 cvsnanne Exp $";

#include "tcpserver.h"
#include "qtcpservercomm.h"


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


bool TcpServer::listen( int port )
{ return qtcpserver_->listen( QHostAddress::Any, port ); }

bool TcpServer::isListening() const
{ return qtcpserver_->isListening(); }

void TcpServer::close()
{ qtcpserver_->close(); }

const char* TcpServer::errorMsg() const
{
    errmsg_ = qtcpserver_->errorString().toAscii().constData();
    return errmsg_.buf();
}
