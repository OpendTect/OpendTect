#ifndef qtcpservercomm_h
#define qtcpservercomm_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
________________________________________________________________________

-*/

#include <QTcpServer>
#include "netserver.h"

/*\brief QTcpServer communication class

  Internal object, to hide Qt's signal/slot mechanism.
*/

QT_BEGIN_NAMESPACE

class QTcpServerComm : public QObject
{
    Q_OBJECT
    friend class	Network::Server;

protected:

QTcpServerComm( QTcpServer* qtcpserver, Network::Server* netserver )
    : qtcpserver_(qtcpserver)
    , netserver_(netserver)
{
    connect( qtcpserver, SIGNAL(newConnection()), this, SLOT(notifNewConn()) );
}

private slots:

void notifNewConn()
{
    netserver_->notifyNewConnection();
}

private:

    QTcpServer*		qtcpserver_;
    Network::Server*	netserver_;

};

QT_END_NAMESPACE

#endif
