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
    connect( qtcpsocket, SIGNAL(disconnected()), this, SLOT(disconnected()) );
    connect( qtcpsocket, SIGNAL(readyRead()), this, SLOT(readyRead()) );
}

private slots:

void disconnected()
{ tcpsocket_->disconnected.trigger( *tcpsocket_ ); }


void readyRead()
{ tcpsocket_->readyRead.trigger( *tcpsocket_ ); }

private:

    QTcpSocket*		qtcpsocket_;
    TcpSocket*		tcpsocket_;

};

QT_END_NAMESPACE

#endif
