#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include <QTcpServer>
#include <QLocalServer>
#include "netserver.h"

/*\brief QTcpServer communication class

  Internal object, to hide Qt's signal/slot mechanism.
*/

QT_BEGIN_NAMESPACE

class QTcpServerComm : public QObject
{
Q_OBJECT
friend class Network::Server;

protected:

QTcpServerComm( QTcpServer* qtcpserver, Network::Server* netserver )
    : qtcpserver_(qtcpserver)
    , netserver_(netserver)
{
    if ( !qtcpserver || !netserver )
	return;
    connect( qtcpserver, &QTcpServer::newConnection,
	     this, &QTcpServerComm::notifNewConn );
}


QTcpServerComm( QLocalServer* qlocalserver, Network::Server* netserver )
    : qlocalserver_(qlocalserver)
    , netserver_(netserver)
{
    if ( !qlocalserver || !netserver )
	return;

    connect( qlocalserver, &QLocalServer::newConnection,
	     this, &QTcpServerComm::notifNewConn );
}


~QTcpServerComm()
{}

private slots:

void notifNewConn()
{
    netserver_->notifyNewConnection();
}

private:

    QTcpServer*		qtcpserver_	= nullptr;
    QLocalServer*	qlocalserver_	= nullptr;
    Network::Server*	netserver_;

};

QT_END_NAMESPACE
