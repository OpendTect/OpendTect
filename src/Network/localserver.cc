/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "localserver.h"
#ifndef OD_NO_QT
#include "qlocalservercomm.h"

#include <QLocalSocket>
#endif


LocalServer::LocalServer()
#ifndef OD_NO_QT
    : qlocalserver_(new QLocalServer)
    , comm_(new QLocalServerComm(qlocalserver_,this))
#else
    : qlocalserver_(0)
    , comm_(0)
#endif
    , newConnection(this)
{}


LocalServer::~LocalServer()
{
    if ( isListening() )
	close();

#ifndef OD_NO_QT
    delete qlocalserver_;
    delete comm_;
#endif
}


bool LocalServer::listen( const char* nm )
{
#ifndef OD_NO_QT
    return qlocalserver_->listen( nm );
#else
    return false;
#endif
}

bool LocalServer::isListening() const
{
#ifndef OD_NO_QT
    return qlocalserver_->isListening();
#else
    return false;
#endif
}

void LocalServer::close()
{
#ifndef OD_NO_QT
    qlocalserver_->close();
#else
    return;
#endif
}

const char* LocalServer::errorMsg() const
{
#ifndef OD_NO_QT
    errmsg_ = qlocalserver_->errorString().toLatin1().constData();
    return errmsg_.buf();
#else
    return 0;
#endif
}

bool LocalServer::waitForNewConnection( int msec, bool& timedout )
{
#ifndef OD_NO_QT
    return qlocalserver_->waitForNewConnection( msec, &timedout );
#else
    return false;
#endif
}

bool LocalServer::hasPendingConnections() const
{
#ifndef OD_NO_QT
    return qlocalserver_->hasPendingConnections();
#else
    return false;
#endif
}

QLocalSocket* LocalServer::nextPendingConnection()
{
#ifndef OD_NO_QT
    return qlocalserver_->nextPendingConnection();
#else
    return 0;
#endif
}


int LocalServer::write( const char* str )
{
#ifndef OD_NO_QT
    QLocalSocket* qsocket = nextPendingConnection();
    if ( !qsocket ) return 0;

    int res = qsocket->write( str );
    qsocket->flush();
    qsocket->disconnectFromServer();
    return res;
#else
    return 0;
#endif
}
