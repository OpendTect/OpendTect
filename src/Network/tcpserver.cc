/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: tcpserver.cc,v 1.2 2009-07-22 16:01:34 cvsbert Exp $";

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
