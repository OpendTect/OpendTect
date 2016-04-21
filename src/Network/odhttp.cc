/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Salil Agarwal
 Date:		October 2012
________________________________________________________________________

-*/

#include "odhttp.h"

#include "databuf.h"
#include "odnetworkaccess.h" //Proxy settings

#ifndef OD_NO_QT
#include "i_odhttpconn.h"

#include <QEventLoop>
#endif

using namespace Network;

HttpRequestProcess::HttpRequestProcess()
    : finished( this )
    , error( this )
    , uploadProgress( this )
    , downloadDataAvailable( this )
    , status_( Running )
#ifdef OD_NO_QT
    , qnetworkreply_(0)
    , qnetworkreplyconn_(0)
#endif
    , bytesuploaded_( 0 )
{
    error.notify( mCB(this,HttpRequestProcess,errorOccurred) );
    finished.notify( mCB(this,HttpRequestProcess,finish) );
    downloadDataAvailable.notify( mCB(this,HttpRequestProcess,dataAvailable) );
    uploadProgress.notify( mCB(this,HttpRequestProcess,uploadStatus) );
}


HttpRequestProcess::~HttpRequestProcess()
{
#ifndef OD_NO_QT
    if ( qnetworkreply_ )
	qnetworkreply_->deleteLater();

    if ( qnetworkreplyconn_ )
	qnetworkreplyconn_->deleteLater();

#endif
}


od_int64 HttpRequestProcess::getBytesUploaded() const
{ return bytesuploaded_; }


od_int64 HttpRequestProcess::getTotalBytesToUpload() const
{ return totalbytestoupload_; }


void HttpRequestProcess::setBytesUploaded(const od_int64 bytes)
{ bytesuploaded_ = bytes; }


void HttpRequestProcess::setTotalBytesToUpload(const od_int64 bytes)
{ totalbytestoupload_ = bytes; }


void HttpRequestProcess::setQNetowrkReply( QNetworkReply* qnr )
{
#ifndef OD_NO_QT
    qnetworkreply_ = qnr;
    qnetworkreplyconn_ = new QNetworkReplyConn(qnetworkreply_, this);
#endif
}


void HttpRequestProcess::waitForFinish()
{
    statuslock_.lock();
    while ( status_ == Running )
    {
	statuslock_.wait();
    }

    statuslock_.unLock();
}


bool HttpRequestProcess::errorOccurred(CallBacker*)
{
#ifndef OD_NO_QT
    statuslock_.lock();
    status_ = Error;
    errmsg_.setFrom( qnetworkreply_->errorString() );
    statuslock_.signal( true );
    statuslock_.unLock();

    return true;
#else
    return false;
#endif
}


bool HttpRequestProcess::finish(CallBacker*)
{
#ifndef OD_NO_QT
    statuslock_.lock();
    if ( status_==Running ) status_ = Finished;
    statuslock_.signal( true );
    statuslock_.unLock();

    return true;
#else
    return false;
#endif
}


bool HttpRequestProcess::dataAvailable( CallBacker* )
{
#ifndef OD_NO_QT
    return true;
#else
    return false;
#endif
}


od_int64 HttpRequestProcess::getContentLengthHeader() const
{
    return
      qnetworkreply_->header(QNetworkRequest::ContentLengthHeader).toLongLong();
}


od_int64 HttpRequestProcess::downloadBytesAvailable() const
{
    return qnetworkreply_->bytesAvailable();
}


od_int64 HttpRequestProcess::read( char* data, od_int64 bufsize )
{
    return qnetworkreply_->read( data, bufsize );
}


bool HttpRequestProcess::waitForDownloadData( int timeout_ms )
{
    return qnetworkreply_->waitForReadyRead( timeout_ms );
}


BufferString HttpRequestProcess::readAll()
{
    BufferString res;
    res = QString( qnetworkreply_->readAll() );
    return res;
}


bool HttpRequestProcess::uploadStatus( CallBacker* )
{
#ifndef OD_NO_QT
    return true;
#else
    return false;
#endif
}

//Network::HttpRequest implementation
HttpRequest::HttpRequest( const char* url )
    : url_( url )
    , postdata_( 0 )
{}


HttpRequest::HttpRequest( const HttpRequest& b )
    : url_( b.url_ )
    , postdata_( b.postdata_ ? new QByteArray(*b.postdata_) : 0  )
    , contenttype_( b.contenttype_ )
    , rawheaders_( b.rawheaders_ )
{}


HttpRequest::~HttpRequest()
{
    delete postdata_;
}


HttpRequest& HttpRequest::postData( const DataBuffer& data )
{ setPostData( data ); return *this; }


HttpRequest& HttpRequest::contentType(const BufferString& type )
{ setContentType( type ); return *this; }


void HttpRequest::setPostData( const DataBuffer& data )
{
    delete postdata_;
    postdata_ = new QByteArray(mCast(const char*,data.data()),data.size());
}


void HttpRequest::setContentType( const BufferString& type )
{
    contenttype_ = type;
}


void HttpRequest::setRawHeader( const char* key, const char* val )
{
    rawheaders_.set( key, val );
}


void HttpRequest::fillRequest( QNetworkRequest& req ) const
{
    req.setUrl( QUrl(url_.buf()) );
    if ( !contenttype_.isEmpty() )
	req.setHeader( QNetworkRequest::ContentTypeHeader, contenttype_.buf() );

    if ( postdata_ )
	req.setHeader( QNetworkRequest::ContentLengthHeader,postdata_->size());

    for ( int idx=0; idx<rawheaders_.size(); idx++ )
    {
	req.setRawHeader( rawheaders_.getKey(idx).buf(),
			 rawheaders_.getValue(idx).buf() );
    }
}


HttpRequestManager::HttpRequestManager()
    : thread_( 0 )
    , qnam_( 0 )
    , curreq_( 0 )
    , curreply_( 0 )
    , stopflag_( false )
    , eventloop_( 0 )
{
    thread_ = new Threads::Thread(mCB(this,HttpRequestManager,threadFuncCB));
}

HttpRequestManager::~HttpRequestManager()
{
    lock_.lock();
    stopflag_ = true;
    eventloop_->exit();
    lock_.unLock();

    thread_->waitForFinish();
    delete thread_;
}


RefMan<HttpRequestProcess> HttpRequestManager::get( const HttpRequest& req )
{
    return access( req, Get );
}


RefMan<HttpRequestProcess> HttpRequestManager::post( const HttpRequest& req )
{ return access( req, Post ); }



RefMan<HttpRequestProcess> HttpRequestManager::head( const HttpRequest& req )
{
    return access( req, Head );
}


RefMan<HttpRequestProcess> HttpRequestManager::access( const HttpRequest& req,
						       AccessType accesstype )
{
    QNetworkRequest qreq;
    req.fillRequest( qreq );

    lock_.lock();
    while ( !eventloop_ && !stopflag_ && curreq_ && curreply_ )
	lock_.wait();

    if ( stopflag_ )
    {
	lock_.unLock();
	return 0;
    }

    curreq_ = &qreq;

    curpostdata_ = req.postdata_;
    accesstype_ = accesstype;

    eventloop_->exit();

    while ( !stopflag_ && !curreply_ )
	lock_.wait();

    if ( stopflag_ )
    {
	lock_.unLock();
	return 0;
    }

    RefMan<HttpRequestProcess> res = curreply_;
    curreply_ = 0;
    lock_.signal( true );
    lock_.unLock();

    return res;
}


void HttpRequestManager::threadFuncCB(CallBacker*)
{
#ifndef OD_NO_QT
    lock_.lock();
    qnam_ = new QNetworkAccessManager;
    eventloop_ = new QEventLoop( qnam_ );
    Network::setHttpProxyFromSettings();


    while ( !stopflag_ )
    {
	if ( curreq_ && !curreply_ )
	{
	    curreply_ = new HttpRequestProcess;
	    if ( accesstype_==Get )
		curreply_->setQNetowrkReply( qnam_->get( *curreq_ ) );
	    else if ( accesstype_==Post )
		curreply_->setQNetowrkReply( qnam_->post( *curreq_,
							  *curpostdata_ ) );
	    else
		curreply_->setQNetowrkReply( qnam_->head( *curreq_ ) );

	    curreq_ = 0;
	    curpostdata_ = 0;
	}
	else
	{
	    lock_.signal( true );
	    lock_.unLock();
	    eventloop_->exec();
	    lock_.lock();
	}
    }

    delete qnam_;
#endif
}

static Threads::SpinLock namlock;
static PtrMan<HttpRequestManager> nam;


HttpRequestManager& HttpRequestManager::instance()
{
    namlock.lock();
    if ( !nam )
    {
	nam = new HttpRequestManager;
    }

    namlock.unLock();
    return *nam;
}
