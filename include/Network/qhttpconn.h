#ifndef qhttpconn_h
#define qhttpconn_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2006
 RCS:           $Id$
________________________________________________________________________

-*/

#include "odhttp.h"
#include <QHttp>

QT_BEGIN_NAMESPACE

class QHttpConnector : public QObject
{
    Q_OBJECT
    friend class ODHttp;

protected:

QHttpConnector( QHttp* snder, ODHttp* receiver )
    : sender_(snder), receiver_(receiver)
{
    connect( sender_, SIGNAL(readyRead(const QHttpResponseHeader&)),
	     this, SLOT(readyRead(const QHttpResponseHeader&)) );

    connect( sender_, SIGNAL(stateChanged(int)),
	     this, SLOT(stateChanged(int)) );

    connect( sender_, SIGNAL(requestStarted(int)),
	     this, SLOT(requestStarted(int)) );

    connect( sender_, SIGNAL(requestFinished(int,bool)),
	     this, SLOT(requestFinished(int,bool)) );

    connect( sender_, SIGNAL(dataReadProgress(int,int)),
	     this, SLOT(dataReadProgress(int,int)) );

    connect( sender_, SIGNAL(done(bool)),
	     this, SLOT(done(bool)) );
}

private slots:

void stateChanged( int state )
{
    if ( state == QHttp::Unconnected )
    {
	receiver_->disconnected.trigger( *receiver_ );
	receiver_->setMessage( "Connection closed" );
    }
    else if ( state == QHttp::Connected )
    {
	receiver_->connected.trigger( *receiver_ );
	receiver_->setMessage( "Connected");
    }
}


void readyRead( const QHttpResponseHeader& )
{
    receiver_->readyRead.trigger( *receiver_ );
}


void requestStarted( int id )
{
    receiver_->requestid_ = id;
    receiver_->requestStarted.trigger( *receiver_ );
}


void dataReadProgress( int dne, int total )
{
    receiver_->nrdone_ = dne;
    receiver_->totalnr_ = total;
    receiver_->dataReadProgress.trigger( *receiver_ );
}


void requestFinished( int id, bool error )
{
    receiver_->requestid_ = id;
    receiver_->error_ = error;
    if ( error )
	receiver_->setMessage( sender_->errorString().toAscii().data() );

    receiver_->requestFinished.trigger( *receiver_ );
}


void done( bool error )
{
    receiver_->error_ = error;
    receiver_->message_ = error ? sender_->errorString().toAscii().data()
				: "Sucessfully finished";
    receiver_->done.trigger( *receiver_ );
}

private:

    QHttp*	sender_;
    ODHttp*	receiver_;
};

QT_END_NAMESPACE

#endif
