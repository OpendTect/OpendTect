/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "localsocket.h"
#ifndef OD_NO_QT
#include "qlocalsocketcomm.h"
#include <QByteArray>
#endif


LocalSocket::LocalSocket()
#ifndef OD_NO_QT
    : qlocalsocket_(new QLocalSocket)
    , comm_(new QLocalSocketComm(qlocalsocket_,this))
#else
    : qlocalsocket_(0)
    , comm_(0)
#endif
    , connected(this)
    , disconnected(this)
    , error(this)
    , readyRead(this)
{
}


LocalSocket::~LocalSocket()
{
#ifndef OD_NO_QT
    delete qlocalsocket_;
    delete comm_;
#endif
}


const char* LocalSocket::errorMsg() const
{
#ifndef OD_NO_QT
    errmsg_ = qlocalsocket_->errorString().toLatin1().constData();
    return errmsg_.buf();
#else
    return 0;
#endif
}


void LocalSocket::connectToServer( const char* host )
{
#ifndef OD_NO_QT
    qlocalsocket_->connectToServer( QString(host) );
#else
    return;
#endif
}

void LocalSocket::disconnectFromServer()
{
#ifndef OD_NO_QT
    qlocalsocket_->disconnectFromServer();
#else
    return;
#endif
}

void LocalSocket::abort()
{
#ifndef OD_NO_QT
    qlocalsocket_->abort();
#else
    return;
#endif
}

bool LocalSocket::waitForConnected( int msec )
{
#ifndef OD_NO_QT
    return qlocalsocket_->waitForConnected( msec );
#else
    return false;
#endif
}

bool LocalSocket::waitForReadyRead( int msec )
{
#ifndef OD_NO_QT
    return qlocalsocket_->waitForReadyRead( msec );
#else
    return false;
#endif
}

int LocalSocket::write( const char* str )
{
#ifndef OD_NO_QT
    return qlocalsocket_->write( str, 1024 );
#else
    return 0;
#endif
}


void LocalSocket::read( BufferString& str )
{
#ifndef OD_NO_QT
    QByteArray ba = qlocalsocket_->readAll();
    str = ba.data();
#else
    return;
#endif
}
