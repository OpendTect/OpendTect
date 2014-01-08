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
#include "tcpsocket.h"

/*\brief QTcpSocket communication class

  Internal object, to hide Qt's signal/slot mechanism.
*/

QT_BEGIN_NAMESPACE

class QTcpSocketComm : public QObject 
{
    Q_OBJECT
    friend class	TcpSocket;

protected:

QTcpSocketComm( QTcpSocket* qtcpsocket, TcpSocket* tcpsocket )
    : qtcpsocket_(qtcpsocket)
    , tcpsocket_(tcpsocket)
{
    connect( qtcpsocket, SIGNAL(connected()), this, SLOT(connected()) );
    connect( qtcpsocket, SIGNAL(disconnected()), this, SLOT(disconnected()) );
    connect( qtcpsocket, SIGNAL(hostFound()), this, SLOT(hostFound()) );
    connect( qtcpsocket, SIGNAL(readyRead()), this, SLOT(readyRead()) );
    connect( qtcpsocket, SIGNAL(error(QAbstractSocket::SocketError)),
	     this, SLOT(error()) );
    connect( qtcpsocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
	     this, SLOT(stateChanged()) );
}

private slots:

void connected()
{ tcpsocket_->connected.trigger( *tcpsocket_ ); }

void disconnected()
{ tcpsocket_->disconnected.trigger( *tcpsocket_ ); }

void hostFound()
{ tcpsocket_->hostFound.trigger( *tcpsocket_ ); }

void readyRead()
{ tcpsocket_->readyRead.trigger( *tcpsocket_ ); }

void error()
{ tcpsocket_->error.trigger( *tcpsocket_ ); }

void stateChanged()
{ tcpsocket_->stateChanged.trigger( *tcpsocket_ ); }

private:

    QTcpSocket*		qtcpsocket_;
    TcpSocket*		tcpsocket_;

};

QT_END_NAMESPACE

#endif
