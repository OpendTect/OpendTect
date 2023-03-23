#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include <QTcpSocket>
#include <QLocalSocket>
#include "netsocket.h"

/*\brief QTcpSocket communication class

  Internal object, to hide Qt's signal/slot mechanism.
*/

QT_BEGIN_NAMESPACE

class QTcpSocketComm : public QObject
{
Q_OBJECT
friend class Network::Socket;

void disconnect()
{
    netsocket_ = nullptr;
}

protected:

QTcpSocketComm( QTcpSocket* qtcpsocket, Network::Socket* netsocket )
    : qtcpsocket_(qtcpsocket)
    , netsocket_(netsocket)
{
    connect( qtcpsocket, &QAbstractSocket::disconnected,
	     this, &QTcpSocketComm::trigDisconnect);
    connect( qtcpsocket, &QIODevice::readyRead,
	     this, &QTcpSocketComm::trigReadyRead );
}


QTcpSocketComm( QLocalSocket* qlocalsocket, Network::Socket* netsocket )
    : qlocalsocket_(qlocalsocket)
    , netsocket_(netsocket)
{
    connect( qlocalsocket, &QLocalSocket::disconnected,
	     this, &QTcpSocketComm::trigDisconnect );
    connect( qlocalsocket, &QIODevice::readyRead,
	     this, &QTcpSocketComm::trigReadyRead );
}


~QTcpSocketComm()
{}

private slots:

void trigDisconnect()
{
    if ( netsocket_ )
	netsocket_->disconnected.trigger( *netsocket_ );
}


void trigReadyRead()
{
    if ( netsocket_ )
	netsocket_->readyRead.trigger( *netsocket_ );
}

private:

    QTcpSocket*		qtcpsocket_	= nullptr;
    QLocalSocket*	qlocalsocket_	= nullptr;
    Network::Socket*	netsocket_;

};

QT_END_NAMESPACE
