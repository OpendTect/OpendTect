/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: odhttp.cc,v 1.1 2010-08-30 10:05:10 cvsnanne Exp $";

#include "odhttp.h"
#include "qhttpconn.h"

#include <QFile>
#include <QHttp>


ODHttp::ODHttp()
    : qhttp_(new QHttp)
    , requestStarted(this)
    , requestFinished(this)
    , messageReady(this)
    , readyRead(this)
    , done(this)
    , disconnected(this)
{
    qhttpconn_ = new QHttpConnector( qhttp_, this );

    error_ = false;
    requestid_ = 0;
    connectionstate_ = -1;
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


int ODHttp::get( const char* path, const char* dest )
{
    QFile* qfile = 0;
    if ( dest )
    {
	qfile = new QFile( dest );
	qfile->open( QIODevice::WriteOnly );
	qfiles_ += qfile;
    }

    const int cmdid = qhttp_->get( path, qfile );
    getids_ += cmdid;
    return cmdid;
}


BufferString ODHttp::readAll() const
{
    QString result = qhttp_->readAll();
    return result.toAscii().data();
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
