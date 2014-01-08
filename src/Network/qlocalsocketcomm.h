#ifndef qlocalsocketcomm_h
#define qlocalsocketcomm_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include <QLocalSocket>
#include "localsocket.h"

/*\brief QLocalSocket communication class

  Internal object, to hide Qt's signal/slot mechanism.
*/

QT_BEGIN_NAMESPACE

class QLocalSocketComm : public QObject 
{
    Q_OBJECT
    friend class	LocalSocket;

protected:

QLocalSocketComm( QLocalSocket* qlocalsocket, LocalSocket* localsocket )
    : qlocalsocket_(qlocalsocket)
    , localsocket_(localsocket)
{
    connect( qlocalsocket, SIGNAL(connected()), this, SLOT(connected()) );
    connect( qlocalsocket, SIGNAL(disconnected()), this, SLOT(disconnected()) );
    connect( qlocalsocket, SIGNAL(error(QLocalSocket::LocalSocketError)),
	     this, SLOT(error()) );
    connect( qlocalsocket, SIGNAL(readyRead()), this, SLOT(readyRead()) );
}

private slots:

void connected()
{ localsocket_->connected.trigger( *localsocket_ ); }

void disconnected()
{ localsocket_->disconnected.trigger( *localsocket_ ); }

void error()
{ localsocket_->error.trigger( *localsocket_ ); }

void readyRead()
{ localsocket_->readyRead.trigger( *localsocket_ ); }

private:

    QLocalSocket*		qlocalsocket_;
    LocalSocket*		localsocket_;

};

QT_END_NAMESPACE

#endif
