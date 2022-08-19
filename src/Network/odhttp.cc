/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odhttp.h"

#include "databuf.h"
#include "genc.h"
#include "odnetworkaccess.h" //Proxy settings

#ifndef OD_NO_QT
#include "i_odhttpconn.h"

#include <QEventLoop>

#include <QSslError>
#include <QCoreApplication>

#endif

namespace Network
{

HttpRequestProcess::HttpRequestProcess( const HttpRequest* request )
    : finished(this)
    , error(this)
    , uploadProgress(this)
    , downloadDataAvailable(this)
    , bytesuploaded_(mUdf(od_int64))
    , totalbytestoupload_(mUdf(od_int64))
    , bytesdownloaded_(mUdf(od_int64))
    , totalbytestodownload_(mUdf(od_int64))
    , contentlengthheader_(mUdf(od_int64))
    , request_(request)
{
}


HttpRequestProcess::~HttpRequestProcess()
{
#ifndef OD_NO_QT
    if ( qnetworkreply_ )
	qnetworkreply_->deleteLater();

    if ( qnetworkreplyconn_ )
	qnetworkreplyconn_->deleteLater();

    Threads::MutexLocker locker(receiveddatalock_);
    deleteAndZeroPtr( receiveddata_ );
#endif
}


uiString HttpRequestProcess::errMsg() const
{
    Threads::MutexLocker locker( statuslock_ );
    return errmsg_;
}


bool HttpRequestProcess::isRunning() const
{
    Threads::MutexLocker locker( statuslock_ );
    return status_==Running;
}


bool HttpRequestProcess::isFinished() const
{
    Threads::MutexLocker locker( statuslock_ );
    return status_==Finished;
}


bool HttpRequestProcess::isError() const
{
    Threads::MutexLocker locker( statuslock_ );
    return status_==Error;
}


od_int64 HttpRequestProcess::getBytesUploaded() const
{ return bytesuploaded_; }


od_int64 HttpRequestProcess::getTotalBytesToUpload() const
{ return totalbytestoupload_; }


bool HttpRequestProcess::waitForRequestStart()
{
    statuslock_.lock();
    while ( status_==NotStarted )
    {
	if ( !statuslock_.wait( 10000 ) ) //timeout if for safety only
	{
	    pErrMsg("Timeout hit on wait" );
	    break;
	}
    }

    const bool res = status_!=NotStarted;
    statuslock_.unLock();
    return res;
}


void HttpRequestProcess::setQNetworkReply( QNetworkReply* qnr )
{
#ifndef OD_NO_QT
    qnetworkreply_ = qnr;
    if ( qnr )
	qnetworkreplyconn_ = new QNetworkReplyConn(qnetworkreply_, this);

    statuslock_.lock();
    status_ = qnr ? Running : Error;
    statuslock_.signal( true );
    statuslock_.unLock();

    if ( !qnr )
	error.trigger();
#endif
}


void HttpRequestProcess::waitForFinish( int timeout )
{
    unsigned long usedtimeout = timeout<0 ? ULONG_MAX : timeout;

    statuslock_.lock();
    while ( status_ == Running )
    {
	if ( !statuslock_.wait( usedtimeout ) )
	    break;
    }

    statuslock_.unLock();
}


void HttpRequestProcess::reportDownloadProgress(od_int64 bytes,
						od_int64 totalbytes)
{
    bytesdownloaded_ = bytes>=0 ? bytes : mUdf(od_int64);
    totalbytestodownload_ = totalbytes>=0 ? totalbytes : mUdf(od_int64);
}


void HttpRequestProcess::reportMetaDataChanged()
{
    contentlengthheader_ =
      qnetworkreply_->header(QNetworkRequest::ContentLengthHeader).toLongLong();
}


void HttpRequestProcess::reportSSLErrors( const QList<QSslError>& list )
{
    pErrMsg("SSL error occurred - SSL error handling not implemented");
    QString errmsg( "SSL Error");

#ifndef QT_NO_OPENSSL
    if ( list.size() )
	errmsg = list[0].errorString();
#endif

    statuslock_.lock();
    status_ = Error;
    errmsg_.setFrom( errmsg );
    statuslock_.signal( true );
    statuslock_.unLock();

    error.trigger();
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
    receiveddatalock_.lock();
    if (!receiveddata_)
	receiveddata_ = new QByteArray;

    receiveddata_->append(qnetworkreply_->readAll());
    receiveddatalock_.signal(true);
    receiveddatalock_.unLock();

    downloadDataAvailable.trigger();
}


od_int64 HttpRequestProcess::getContentLengthHeader() const
{
    return contentlengthheader_;
}


od_int64 HttpRequestProcess::downloadBytesAvailable() const
{
    return receiveddata_ ? receiveddata_->length() : 0;
}


od_int64 HttpRequestProcess::read( char* data, od_int64 bufsize )
{
    Threads::MutexLocker locker(receiveddatalock_);
    if (!receiveddata_)
	return 0;

    const int readsize = mMIN( bufsize, receiveddata_->length());

    memcpy(data, receiveddata_->data(), readsize );
    receiveddata_->remove(0, readsize);
    return readsize;
}


bool HttpRequestProcess::waitForDownloadData( int timeout )
{
    unsigned long usedtimeout = timeout<0 ? ULONG_MAX : timeout;
    receiveddatalock_.lock();
    while ( !receiveddata_ || !receiveddata_->size() )
    {
	if ( !receiveddatalock_.wait( usedtimeout ) )
	{
	    break;
	}
    }

    const bool res = receiveddata_ && receiveddata_->size();

    receiveddatalock_.unLock();

    return res;
}


BufferString HttpRequestProcess::readAll()
{
    BufferString res;
    Threads::MutexLocker locker(receiveddatalock_);
    if (!receiveddata_)
	return res;

    res = QString( *receiveddata_ );
    receiveddata_->clear();
    locker.unLock();

    return res;
}


//Network::HttpRequest implementation
HttpRequest::HttpRequest( const char* url, AccessType at )
    : url_( url )
    , accesstype_( at )
{}


HttpRequest::HttpRequest( const HttpRequest& b )
    : url_( b.url_ )
    , payload_( b.payload_ ? new QByteArray(*b.payload_) : nullptr  )
    , contenttype_( b.contenttype_ )
    , rawheaders_( b.rawheaders_ )
    , accesstype_( b.accesstype_ )
{}


HttpRequest::~HttpRequest()
{
    delete payload_;
}


HttpRequest& HttpRequest::payloadData( const DataBuffer& data )
{ setPayloadData( data ); return *this; }


HttpRequest& HttpRequest::contentType(const BufferString& type )
{ setContentType( type ); return *this; }


void HttpRequest::setPayloadData( const DataBuffer& data )
{
    delete payload_;
    payload_ = new QByteArray(rCast(const char*,data.data()),data.size());
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

    if ( payload_ )
	req.setHeader( QNetworkRequest::ContentLengthHeader,payload_->size());

    IOParIterator iter( rawheaders_ );
    BufferString key, val;
    while ( iter.next(key,val) )
    {
	req.setRawHeader( key.buf(), val.buf() );
    }
}


HttpRequestManager::HttpRequestManager()
{
    Threads::ConditionVar eventlooplock;
    eventlooplock_ = &eventlooplock;
    thread_ = new Threads::Thread( mCB(this,HttpRequestManager,threadFuncCB),
				   "HttpRequestManager" );
    eventlooplock.lock();
    while ( !eventloop_ )
	eventlooplock.wait();
    eventlooplock.unLock();

    eventlooplock_ = nullptr;
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


RefMan<HttpRequestProcess> HttpRequestManager::get( const char* url )
{
    RefMan<HttpRequest> req = new HttpRequest( url, HttpRequest::Get );
    return request( req );
}


RefMan<HttpRequestProcess> HttpRequestManager::head( const char* url )
{
    RefMan<HttpRequest> req = new HttpRequest( url, HttpRequest::Head );
    return request( req );
}


RefMan<HttpRequestProcess>
	HttpRequestManager::request( const HttpRequest* req )
{
    RefMan<HttpRequestProcess> res = new HttpRequestProcess( req );
    activeeventslock_.lock();
    activeevents_ += res.ptr();
    activeeventslock_.unLock();

    CallBack::addToThread(thread_->threadID(),
			mCB(this, HttpRequestManager, doRequestCB), res.ptr() );

    const bool success = res->waitForRequestStart();

    activeeventslock_.lock();
    activeevents_ -= res.ptr();
    activeeventslock_.unLock();

    return success ? res : 0;
}


void HttpRequestManager::threadFuncCB(CallBacker*)
{
#ifndef OD_NO_QT
    createReceiverForCurrentThread();

    //Create outside of look as we'll trigger events and stuff in Qt
    Network::setHttpProxyFromSettings();
    auto* eventloop = new QEventLoop( 0 );
    auto* qnam = new QNetworkAccessManager( eventloop );

    //Lock, set the parameters and signal
    eventlooplock_->lock();
    eventloop_ = eventloop;
    qnam_ = qnam;
    eventlooplock_->signal(true);
    eventlooplock_->unLock();

    eventloop_->exec();

    //Process all waiting events by setting their reply to null
    activeeventslock_.lock();
    RefObjectSet<HttpRequestProcess> events = activeevents_;
    activeeventslock_.unLock();

    for ( int idx=0; idx<events.size(); idx++ )
	events[idx]->setQNetworkReply( 0 );

    removeReceiverForCurrentThread();
    qnam_ = nullptr; //Deleted with eventloop_
    deleteAndZeroPtr( eventloop_ );
#endif
}


void HttpRequestManager::doRequestCB( CallBacker* cb )
{
    RefMan<HttpRequestProcess> reply = sCast(HttpRequestProcess*,cb);
    activeeventslock_.lock();
    if ( !activeevents_.isPresent( reply ) )
    {
	activeeventslock_.unLock();
	return;
    }

    activeeventslock_.unLock();

    QNetworkRequest qreq;
    reply->request_->fillRequest(qreq);

    if (reply->request_->accesstype_ == HttpRequest::Get)
	reply->setQNetworkReply(qnam_->get(qreq));
    else if (reply->request_->accesstype_ == HttpRequest::Put)
	reply->setQNetworkReply(qnam_->put(qreq,*reply->request_->payload_));
    else if (reply->request_->accesstype_ == HttpRequest::Post)
	reply->setQNetworkReply(qnam_->post(qreq,*reply->request_->payload_));
    else if (reply->request_->accesstype_ == HttpRequest::Delete )
	reply->setQNetworkReply(qnam_->deleteResource(qreq));
    else
	reply->setQNetworkReply(qnam_->head(qreq));
}

static Threads::SpinLock namlock;
static PtrMan<HttpRequestManager> nam;

void HttpRequestManager::CloseInstance()
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

} // namespace Network
