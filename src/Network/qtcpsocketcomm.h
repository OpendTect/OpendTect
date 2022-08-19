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
    friend class	Network::Socket;

    void		disconnect() { netsocket_ = 0; }

protected:

QTcpSocketComm( QTcpSocket* qtcpsocket, Network::Socket* netsocket )
    : qtcpsocket_(qtcpsocket)
    , netsocket_(netsocket)
{
    connect( qtcpsocket, SIGNAL(disconnected()), this, SLOT(trigDisconnect()));
    connect( qtcpsocket, SIGNAL(readyRead()), this, SLOT(trigReadyRead()) );
}


QTcpSocketComm( QLocalSocket* qlocalsocket, Network::Socket* netsocket )
    : qlocalsocket_(qlocalsocket)
    , netsocket_(netsocket)
{
    connect( qlocalsocket, SIGNAL(disconnected()), this,
					    SLOT(trigDisconnect()) );
    connect( qlocalsocket, SIGNAL(readyRead()), this, SLOT(trigReadyRead()) );
}

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
