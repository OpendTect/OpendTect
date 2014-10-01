#ifndef qtcpsocketcomm_h
#define qtcpsocketcomm_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include <QTcpSocket>
#include "netsocket.h"
#include "tcpsocket.h"

/*\brief QTcpSocket communication class

  Internal object, to hide Qt's signal/slot mechanism.
*/

QT_BEGIN_NAMESPACE

class QTcpSocketComm : public QObject
{
    Q_OBJECT
    friend class	Network::Socket;

    void		disconnect() { socket_ = 0; }

protected:

QTcpSocketComm( QTcpSocket* qtcpsocket, Network::Socket* sock )
    : qtcpsocket_(qtcpsocket)
    , socket_(sock)
{
    connect( qtcpsocket, SIGNAL(disconnected()), this, SLOT(disconnected()) );
    connect( qtcpsocket, SIGNAL(readyRead()), this, SLOT(readyRead()) );
}

private slots:

void disconnected()
{
    if ( socket_ )
	socket_->disconnected.trigger( *socket_ );
}


void readyRead()
{
    if ( socket_ )
	socket_->readyRead.trigger( *socket_ );
}

private:

    QTcpSocket*		qtcpsocket_;
    Network::Socket*	socket_;

};


class OldQTcpSocketComm : public QObject
{
    Q_OBJECT
    friend class	TcpSocket;

    void		disconnect() { tcpsocket_ = 0; }

protected:

OldQTcpSocketComm( QTcpSocket* qtcpsocket, TcpSocket* tcpsocket )
    : qtcpsocket_(qtcpsocket)
    , tcpsocket_(tcpsocket)
{
    connect( qtcpsocket, SIGNAL(disconnected()), this, SLOT(disconnected()) );
    connect( qtcpsocket, SIGNAL(readyRead()), this, SLOT(readyRead()) );
}

private slots:

void disconnected()
{
    if ( tcpsocket_ )
	tcpsocket_->disconnected.trigger( *tcpsocket_ );
}


void readyRead()
{
    if ( tcpsocket_ )
	tcpsocket_->readyRead.trigger( *tcpsocket_ );
}

private:

    QTcpSocket*		qtcpsocket_;
    TcpSocket*		tcpsocket_;

};

QT_END_NAMESPACE

#endif
