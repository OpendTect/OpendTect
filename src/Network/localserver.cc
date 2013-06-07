/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "localserver.h"
#include "qlocalservercomm.h"

#include <QLocalSocket>


LocalServer::LocalServer()
    : qlocalserver_(new QLocalServer)
    , comm_(new QLocalServerComm(qlocalserver_,this))
    , newConnection(this)
{}


LocalServer::~LocalServer()
{
    if ( isListening() )
	close();

    delete qlocalserver_;
    delete comm_;
}


bool LocalServer::listen( const char* nm )
{ return qlocalserver_->listen( nm ); }

bool LocalServer::isListening() const
{ return qlocalserver_->isListening(); }

void LocalServer::close()
{ qlocalserver_->close(); }

const char* LocalServer::errorMsg() const
{
    errmsg_ = qlocalserver_->errorString().toAscii().constData();
    return errmsg_.buf();
}

bool LocalServer::waitForNewConnection( int msec, bool& timedout )
{ return qlocalserver_->waitForNewConnection( msec, &timedout ); }

bool LocalServer::hasPendingConnections() const
{ return qlocalserver_->hasPendingConnections(); }

QLocalSocket* LocalServer::nextPendingConnection()
{ return qlocalserver_->nextPendingConnection(); }


int LocalServer::write( const char* str )
{
    QLocalSocket* qsocket = nextPendingConnection();
    if ( !qsocket ) return 0;

    int res = qsocket->write( str );
    qsocket->flush();
    qsocket->disconnectFromServer();
    return res;
}
