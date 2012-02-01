/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: odhttp.cc,v 1.17 2012-02-01 20:36:40 cvsnanne Exp $";

#include "odhttp.h"
#include "qhttpconn.h"

#include <QByteArray>
#include <QEventLoop>
#include <QFile>
#include <QHttp>
#include <QUrl>


class MyHttp : public QHttp
{
public:
MyHttp() : QHttp(), asynchronous_(false)		{}

~MyHttp()
{
    if ( qeventloop_.isRunning() )
	qeventloop_.exit();
}

void init()
{
    qfiles_.allowNull();
}

void startEventLoop()
{
    if ( !asynchronous_ && !qeventloop_.isRunning() )
	qeventloop_.exec();
}

void stopEventLoop()
{
    if ( !asynchronous_ )
	qeventloop_.exit();
}

int _setHost( const char* host, int port )
{
    const int id = setHost( host, port );
    startEventLoop();
    return id;
}

int _get( const char* path, const char* dest )
{
    QFile* qfile = 0;
    if ( dest && *dest )
    {
	qfile = new QFile( dest );
	qfile->open( QIODevice::WriteOnly );
    }

    qfiles_ += qfile;
    QUrl qurl( path );
    const int id = get( qurl.toEncoded(), qfile );
    getids_ += id;
    startEventLoop();
    return id;
}


void handleFinishedRequest( int reqid )
{
    const int reqidx = getids_.indexOf( reqid );
    if ( qfiles_.validIdx(reqidx) )
    {
	QFile* qfile = qfiles_[reqidx];
	if ( qfile )
	    qfile->close();
	delete qfiles_.remove( reqidx );
	getids_.remove( reqidx );
    }

    stopEventLoop();
}

    bool		asynchronous_;

protected:
    QEventLoop	qeventloop_;

    TypeSet<int>	getids_;
    ObjectSet<QFile>	qfiles_;
};



ODHttp::ODHttp()
    : qhttp_(new MyHttp)
    , requestStarted(this)
    , requestFinished(this)
    , messageReady(this)
    , dataReadProgress(this)
    , readyRead(this)
    , done(this)
    , connected(this)
    , disconnected(this)
{
    qhttpconn_ = new QHttpConnector( qhttp_, this );
    qhttp_->init();

    error_ = false;
    requestid_ = 0;

    requestFinished.notify( mCB(this,ODHttp,reqFinishedCB) );
}


ODHttp::~ODHttp()
{
    delete qhttpconn_;
}


void ODHttp::setASynchronous( bool yn )
{ qhttp_->asynchronous_ = yn; }


// ToDo: support username and passwd
int ODHttp::setProxy( const char* host, int port,
		      const char* usrnm, const char* pwd )
{ return qhttp_->setProxy( host, port ); }

int ODHttp::setHttpsHost( const char* host, int port )
{ return qhttp_->setHost( host, QHttp::ConnectionModeHttps ); }

int ODHttp::setHost( const char* host, int port )
{ return qhttp_->_setHost( host, port ); }

int ODHttp::close()
{ return qhttp_->close(); }

void ODHttp::abort()
{ qhttp_->abort(); }

ODHttp::State ODHttp::state() const
{ return (ODHttp::State)(int)qhttp_->state(); }

int ODHttp::get( const char* path, const char* dest )
{ return qhttp_->_get( path, dest ); }


wchar_t* ODHttp::readWCharBuffer() const
{
    QByteArray qbytearr = qhttp_->readAll();
    QString trl = QString::fromUtf8( qbytearr );
    wchar_t* warr = new wchar_t [qbytearr.size()+1];
    const int res = trl.toWCharArray( warr );
    return res == 0 ? 0 : warr;
}


const char* ODHttp::readCharBuffer() const
{
    static QByteArray result;
    result = qhttp_->readAll();
    return result.constData();
}


BufferString ODHttp::readBuffer() const
{
    QByteArray result = qhttp_->readAll();
    return result.constData();
}


bool ODHttp::hasPendingRequests() const
{ return qhttp_->hasPendingRequests(); }

void ODHttp::clearPendingRequests()
{ return qhttp_->clearPendingRequests(); }

od_int64 ODHttp::bytesAvailable() const
{ return qhttp_->bytesAvailable(); }


void ODHttp::setMessage( const char* msg )
{
    message_ = msg;
    messageReady.trigger();
}


void ODHttp::reqFinishedCB( CallBacker* )
{
    qhttp_->handleFinishedRequest( requestid_ );
}
