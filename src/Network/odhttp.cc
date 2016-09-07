/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Salil Agarwal
 Date:		October 2012
________________________________________________________________________

-*/

#include "odhttp.h"

#include "databuf.h"
#include "genc.h"
#include "odnetworkaccess.h" //Proxy settings

#ifndef OD_NO_QT
#include "i_odhttpconn.h"

#include <QEventLoop>
#include <qcoreapplication.h>
#endif

using namespace Network;

static Threads::Atomic<int> nractivereplies = 0;

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
    , receiveddata_( 0 )
    , receiveddatalock_( true )
    , bytesdownloaded_( 0 )
    , totalbytestodownload_( 0 )
{ }


HttpRequestProcess::~HttpRequestProcess()
{
#ifndef OD_NO_QT
    if (qnetworkreply_)
    {
	qnetworkreply_->deleteLater();
	nractivereplies--;
    }

    if ( qnetworkreplyconn_ )
	qnetworkreplyconn_->deleteLater();

    Threads::Locker locker(receiveddatalock_);
    deleteAndZeroPtr(receiveddata_);

#endif
}


od_int64 HttpRequestProcess::getBytesUploaded() const
{ return bytesuploaded_; }


od_int64 HttpRequestProcess::getTotalBytesToUpload() const
{ return totalbytestoupload_; }


void HttpRequestProcess::setQNetowrkReply( QNetworkReply* qnr )
{
#ifndef OD_NO_QT
    nractivereplies++;
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


void HttpRequestProcess::reportDownloadProgress(od_int64 bytes,
						od_int64 totalbytes)
{
    bytesdownloaded_ = bytes;
    totalbytestodownload_ = totalbytes;
}


void HttpRequestProcess::reportError()
{
    statuslock_.lock();
    status_ = Error;
    errmsg_.setFrom( qnetworkreply_->errorString() );
    statuslock_.signal( true );
    statuslock_.unLock();

    error.trigger();
}

void HttpRequestProcess::reportUploadProgress( od_int64 bytes,
					       od_int64 totalbytes )
{
    bytesuploaded_ = bytes;
    totalbytestoupload_ = totalbytes;

    uploadProgress.trigger();
}

void HttpRequestProcess::reportFinished()
{
    statuslock_.lock();
    if ( status_==Running ) status_ = Finished;
    statuslock_.signal( true );
    statuslock_.unLock();

    finished.trigger();
}


void HttpRequestProcess::reportReadyRead()
{
    Threads::Locker locker(receiveddatalock_);
    if (!receiveddata_)
	receiveddata_ = new QByteArray;

    receiveddata_->append(qnetworkreply_->readAll());

    locker.unlockNow();

    downloadDataAvailable.trigger();
}


od_int64 HttpRequestProcess::getContentLengthHeader() const
{
    //Threadsafe ?
    return
      qnetworkreply_->header(QNetworkRequest::ContentLengthHeader).toLongLong();
}


od_int64 HttpRequestProcess::downloadBytesAvailable() const
{
    return receiveddata_ ? receiveddata_->length() : 0;
}


od_int64 HttpRequestProcess::read( char* data, od_int64 bufsize )
{
    Threads::Locker locker(receiveddatalock_);
    if (!receiveddata_)
	return 0;

    const int readsize = mMIN( bufsize, receiveddata_->length());

    memcpy(data, receiveddata_->data(), readsize );
    receiveddata_->remove(0, readsize);
    return readsize;
}


bool HttpRequestProcess::waitForDownloadData( int timeout_ms )
{
    //Threadsafe ?
    return qnetworkreply_->waitForReadyRead( timeout_ms );
}


BufferString HttpRequestProcess::readAll()
{
    BufferString res;
    Threads::Locker locker(receiveddatalock_);
    if (!receiveddata_)
	return res;

    res = QString( *receiveddata_ );
    receiveddata_->clear();
    locker.unlockNow();

    return res;
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
    , eventloop_( 0 )
{
    thread_ = new Threads::Thread(
	    mCB(this,HttpRequestManager,threadFuncCB),
	    "HttpRequestManager" );
    lock_.lock();
    while (!eventloop_)
	lock_.wait();
    lock_.unLock();
}


HttpRequestManager::~HttpRequestManager()
{
    shutDownThreading();
}


void HttpRequestManager::shutDownThreading()
{
    if ( eventloop_ )
    {
	eventloop_->exit();
    }

    if ( thread_ )
    {
	thread_->waitForFinish();
	deleteAndZeroPtr( thread_ );
    }
}


RefMan<HttpRequestProcess> HttpRequestManager::get( const HttpRequest& req )
{
    return request(req, Get);
}


RefMan<HttpRequestProcess> HttpRequestManager::post( const HttpRequest& req )
{
    return request(req, Post);
}



RefMan<HttpRequestProcess> HttpRequestManager::head( const HttpRequest& req )
{
    return request( req, Head );
}


RefMan<HttpRequestProcess>
HttpRequestManager::request( const HttpRequest& req,AccessType accesstype )
{
    RequestData rq( this, req, accesstype );
    CallBack::addToThread(thread_->threadID(),
	    		mCB(this, HttpRequestManager, doRequestCB), &rq);
    
    rq.wait();
   
   return rq.reply_;
}


void HttpRequestManager::threadFuncCB(CallBacker*)
{
#ifndef OD_NO_QT
    createReceiverForCurrentThread();

    Network::setHttpProxyFromSettings();

    lock_.lock();

    eventloop_ = new QEventLoop( 0 );
    qnam_ = new QNetworkAccessManager( eventloop_ );
    lock_.signal(true);
    lock_.unLock();


    eventloop_->exec();

    while (nractivereplies)
    {
    }

    //Ensure everyting is processed (i.e. deletion of replies).
    eventloop_->processEvents();

    removeReceiverForCurrentThread();
    qnam_ = 0; //Deleted with eventloop_
    deleteAndZeroPtr( eventloop_ );
#endif
}


void HttpRequestManager::doRequestCB(CallBacker* cb)
{
    HttpRequestManager::RequestData* rd =
	(HttpRequestManager::RequestData*) cb;

    QNetworkRequest qreq;
    rd->req_.fillRequest(qreq);

    rd->reply_ = new HttpRequestProcess;
    if (rd->at_ == Get)
	rd->reply_->setQNetowrkReply(qnam_->get(qreq));
    else if (rd->at_ == Post)
	rd->reply_->setQNetowrkReply(qnam_->post(qreq,
				     *rd->req_.postdata_ ));
    else
	rd->reply_->setQNetowrkReply(qnam_->head(qreq));

    rd->isrunlock_.lock();
    rd->isrun_ = true;
    rd->isrunlock_.signal(true);
    rd->isrunlock_.unLock();
}

static Threads::SpinLock namlock;
static PtrMan<HttpRequestManager> nam;

static void CloseInstance()
{
    HttpRequestManager::instance().shutDownThreading();
}


HttpRequestManager& HttpRequestManager::instance()
{
    namlock.lock();
    if ( !nam )
    {
	nam = new HttpRequestManager;
	NotifyExitProgram(CloseInstance);
    }

    namlock.unLock();
    return *nam;
}
