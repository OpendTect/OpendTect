/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: tcpsocket.cc,v 1.3 2009-10-27 03:22:20 cvsnanne Exp $";

#include "tcpsocket.h"
#include "qtcpsocketcomm.h"


TcpSocket::TcpSocket()
    : qtcpsocket_(new QTcpSocket)
    , comm_(new QTcpSocketComm(qtcpsocket_,this))
    , connected(this)
    , disconnected(this)
    , hostFound(this)
    , readyRead(this)
    , error(this)
    , stateChanged(this)
{}


TcpSocket::~TcpSocket()
{
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
