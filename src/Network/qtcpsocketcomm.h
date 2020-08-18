#ifndef qtcpsocketcomm_h
#define qtcpsocketcomm_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
________________________________________________________________________

-*/


#include <QLocalSocket>
#include <QTcpSocket>
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
    connect(qtcpsocket, SIGNAL(error(QAbstractSocket::SocketError)),
	this, SLOT(handleError(QAbstractSocket::SocketError)));
    connect(qtcpsocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
	this, SLOT(handleStateChange(QAbstractSocket::SocketState)));
}


QTcpSocketComm(QLocalSocket* qlocalsocket, Network::Socket* netsocket)
    : qlocalsocket_(qlocalsocket)
    , netsocket_(netsocket)
{
    connect(qlocalsocket, SIGNAL(disconnected()), this, SLOT(trigDisconnect()));
    connect(qlocalsocket, SIGNAL(readyRead()), this, SLOT(trigReadyRead()));
}

private slots:

void handleError( QAbstractSocket::SocketError err )
{
    if (netsocket_)
	netsocket_->error.trigger();
}

void handleStateChange(QAbstractSocket::SocketState state)
{
}


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

    QTcpSocket* qtcpsocket_	= nullptr;
    QLocalSocket* qlocalsocket_ = nullptr;
    Network::Socket*	netsocket_;

};

QT_END_NAMESPACE

#endif
