/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: odhttp.cc,v 1.10 2010-10-28 05:59:50 cvsranojay Exp $";

#include "odhttp.h"
#include "qhttpconn.h"

#include <QByteArray>
#include <QFile>
#include <QHttp>
#include <QUrl>


ODHttp::ODHttp()
    : qhttp_(new QHttp)
    , requestStarted(this)
    , requestFinished(this)
    , messageReady(this)
    , readyRead(this)
    , done(this)
    , connected(this)
    , disconnected(this)
{
    qhttpconn_ = new QHttpConnector( qhttp_, this );

    error_ = false;
    requestid_ = 0;
    qfiles_.allowNull();

    requestFinished.notify( mCB(this,ODHttp,reqFinishedCB) );
}


ODHttp::~ODHttp()
{
    delete qhttpconn_;
}


int ODHttp::setHost( const char* host, int port )
{ return qhttp_->setHost( host, port ); }

int ODHttp::close()
{ return qhttp_->close(); }

void ODHttp::abort()
{ qhttp_->abort(); }

ODHttp::State ODHttp::state() const
{ return (ODHttp::State)(int)qhttp_->state(); }


int ODHttp::get( const char* path, const char* dest )
{
    QFile* qfile = 0;
    if ( dest )
    {
	qfile = new QFile( dest );
	qfile->open( QIODevice::WriteOnly );
    }

    qfiles_ += qfile;
    QUrl qurl( path );
    const int cmdid = qhttp_->get( qurl.toEncoded(), qfile );
    getids_ += cmdid;
    return cmdid;
}


wchar_t* ODHttp::readWCharBuffer() const
{
    QByteArray qbytearr = qhttp_->readAll();
    QString trl = QString::fromUtf8( qbytearr );
    wchar_t* warr = new wchar_t [qbytearr.size()+1];
    const int res = trl.toWCharArray( warr );
    return res == 0 ? 0 : warr;
}


char* ODHttp::readCharBuffer() const
{
    static QByteArray result;
    result = qhttp_->readAll();
    return result.data();
}


BufferString ODHttp::readBuffer() const
{
    QByteArray result = qhttp_->readAll();
    return result.constData();
}


bool ODHttp::hasPendingRequests() const
{ return qhttp_->hasPendingRequests(); }

od_int64 ODHttp::bytesAvailable() const
{ return qhttp_->bytesAvailable(); }


void ODHttp::setMessage( const char* msg )
{
    message_ = msg;
    messageReady.trigger();
}


void ODHttp::reqFinishedCB( CallBacker* )
{
    const int reqidx = getids_.indexOf( requestid_ );
    if ( qfiles_.validIdx(reqidx) )
    {
	QFile* qfile = qfiles_[reqidx];
	if ( qfile )
	    qfile->close();
	delete qfiles_.remove( reqidx );
	getids_.remove( reqidx );
    }
}
