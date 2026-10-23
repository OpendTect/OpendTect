/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odhttp.h"

#include "databuf.h"
#include "envvars.h"
#include "genc.h"
#include "hiddenparam.h"
#include "networkcommon.h"
#include "odruncontext.h"
#include "od_ostream.h"

#ifndef OD_NO_QT
#include "i_odhttpconn.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslError>
#include <QUrlQuery>

#endif

namespace Network
{

struct HttpRequestExtras
{
    HttpProxyConfig	proxy;
    int			transfertimeoutms	= 0;
};

/* Route and failure kind stay off the exported objects. */
static HiddenParam<HttpRequest,HttpRequestExtras*> hp_request_( nullptr );
static HiddenParam<HttpRequestProcess,int> hp_failurekind_( 0 );


// HttpProxyConfig

HttpProxyConfig::HttpProxyConfig()
    : authority_(*new Authority)
{}


HttpProxyConfig::HttpProxyConfig( const HttpProxyConfig& oth )
    : authority_(*new Authority)
{
    *this = oth;
}


HttpProxyConfig::~HttpProxyConfig()
{
    delete &authority_;
}


HttpProxyConfig& HttpProxyConfig::operator=( const HttpProxyConfig& oth )
{
    if ( &oth == this )
	return *this;

    type_ = oth.type_;
    authority_.setHost( oth.authority_.getHost() );
    authority_.setPort( oth.authority_.getPort() );
    username_ = oth.username_;
    password_ = oth.password_;
    return *this;
}

HttpRequestProcess::HttpRequestProcess( const HttpRequest* request )
    : finished(this)
    , error(this)
    , downloadDataAvailable(this)
    , uploadProgress(this)
    , request_(request)
    , bytesuploaded_(mUdf(od_int64))
    , totalbytestoupload_(mUdf(od_int64))
    , bytesdownloaded_(mUdf(od_int64))
    , totalbytestodownload_(mUdf(od_int64))
    , contentlengthheader_(mUdf(od_int64))
{
    hp_failurekind_.setParam( this, (int)FailureKind::NoFailure );
}


HttpRequestProcess::~HttpRequestProcess()
{
    hp_failurekind_.removeParam( this );
#ifndef OD_NO_QT
    if ( qnetworkreply_ )
	qnetworkreply_->deleteLater();

    if ( qnetworkreplyconn_ )
	qnetworkreplyconn_->deleteLater();

    Threads::MutexLocker locker(receiveddatalock_);
    deleteAndNullPtr( receiveddata_ );
#endif
}


uiRetVal HttpRequestProcess::errMsgs() const
{
    Threads::MutexLocker locker( statuslock_ );
    return errmsgs_;
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


static HttpRequestProcess::FailureKind
	failureKindFromQtError( QNetworkReply::NetworkError err )
{
    if ( err == QNetworkReply::NoError )
	return HttpRequestProcess::FailureKind::NoFailure;

    if ( err == QNetworkReply::SslHandshakeFailedError )
	return HttpRequestProcess::FailureKind::SslFailure;

    if ( err == QNetworkReply::ProxyAuthenticationRequiredError )
	return HttpRequestProcess::FailureKind::ProxyAuthFailure;

    if ( err >= QNetworkReply::ProxyConnectionRefusedError &&
	 err <= QNetworkReply::UnknownProxyError )
	return HttpRequestProcess::FailureKind::ConnectFailure;

    if ( err >= QNetworkReply::ContentAccessDenied )
	return HttpRequestProcess::FailureKind::HttpFailure;

    return HttpRequestProcess::FailureKind::ConnectFailure;
}


HttpRequestProcess::FailureKind HttpRequestProcess::failureKind() const
{
    return sCast( FailureKind, hp_failurekind_.getParam(this) );
}


void HttpRequestProcess::reportSSLErrors( const QList<QSslError>& sslerrors )
{
    uiRetVal errors;
#ifndef QT_NO_OPENSSL
    {
	for ( int idx=0; idx<sslerrors.size(); idx++ )
	    errors.add( toUiString(sslerrors[idx].errorString()) );
    }
#endif
    if ( errors.isEmpty() )
	errors.add( tr("SSL Error (unknown)") );

    statuslock_.lock();
    status_ = Error;
    hp_failurekind_.setParam( this, (int)FailureKind::SslFailure );
    errmsgs_.add( errors );
    statuslock_.signal( true );
    statuslock_.unLock();

    error.trigger();
}


void HttpRequestProcess::reportError()
{
    const QNetworkReply::NetworkError qerr = qnetworkreply_
	? qnetworkreply_->error()
	: QNetworkReply::UnknownNetworkError;
    const FailureKind kind = failureKindFromQtError( qerr );
    const uiString errmsg = qnetworkreply_
	? toUiString( qnetworkreply_->errorString() )
	: tr("Network request failed");

    statuslock_.lock();
    status_ = Error;
    if ( hp_failurekind_.getParam(this) == (int)FailureKind::NoFailure )
	hp_failurekind_.setParam( this, (int)kind );

    errmsgs_.add( errmsg );

    static int addprint = -1;
    if ( addprint < 0 )
    {
	if ( OD::InDebugMode() )
	    addprint = !OD::InTestProgRunContext() ||
			GetEnvVarYN( "TEST_DEBUG_HTTPREQUESTS" ) ? 1 : 0;
	else
	    addprint = 0;
    }

    if ( addprint == 1 )
    {
	od_ostream& strm = od_cerr();
	strm << "Attributes:" << od_newline;
	for ( int idx=0; idx<=29; idx++ )
	{
	    const QNetworkRequest::Attribute attr =
					QNetworkRequest::Attribute( idx );
	    const QVariant value = qnetworkreply_->attribute( attr );
	    if ( !value.isValid() )
		continue;

	    const QString valuestr = value.toString();
	    strm << (int)attr << ": (" << value.typeName()
		 << ", " << BufferString(valuestr) << ")" << od_newline;
	}

	const auto& headers = qnetworkreply_->rawHeaderPairs();
	for ( const auto& header : headers )
	{
	    const std::string firststr = header.first.toStdString();
	    const std::string secondstr = header.second.toStdString();
	    strm << firststr.c_str() << ": "
		 << secondstr.c_str() << od_newline;
	}
    }

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


int HttpRequestProcess::httpStatusCode() const
{
#ifndef OD_NO_QT
    if ( !qnetworkreply_ )
	return 0;

    const QVariant attr =
	qnetworkreply_->attribute( QNetworkRequest::HttpStatusCodeAttribute );
    return attr.isValid() ? attr.toInt() : 0;
#else
    return 0;
#endif
}


od_int64 HttpRequestProcess::downloadBytesAvailable() const
{
    Threads::MutexLocker locker(receiveddatalock_);
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
{
    hp_request_.setParam( this, new HttpRequestExtras );
}


HttpRequest::HttpRequest( const HttpRequest& b )
    : payload_( b.payload_ ? new QByteArray(*b.payload_) : nullptr  )
    , url_( b.url_ )
    , contenttype_( b.contenttype_ )
    , rawheaders_( b.rawheaders_ )
    , encodeddata_( b.encodeddata_ )
    , accesstype_( b.accesstype_ )
{
    auto* extras = new HttpRequestExtras( *hp_request_.getParam(&b) );
    hp_request_.setParam( this, extras );
}


HttpRequest::~HttpRequest()
{
    hp_request_.removeAndDeleteParam( this );
    delete payload_;
}


HttpRequest& HttpRequest::payloadData( const DataBuffer& data )
{
    setPayloadData( data );
    return *this;
}


HttpRequest& HttpRequest::contentType(const BufferString& type )
{
    setContentType( type );
    return *this;
}


HttpRequest& HttpRequest::rawHeader( const char* key, const char* val )
{
    setRawHeader( key, val );
    return *this;
}


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


void HttpRequest::setEncodedData( const char* key, const char* val )
{
    encodeddata_.set( key, val );
}


void HttpRequest::setProxyConfig( const HttpProxyConfig& cfg )
{
    hp_request_.getParam( this )->proxy = cfg;
}


const HttpProxyConfig& HttpRequest::proxyConfig() const
{
    return hp_request_.getParam( this )->proxy;
}


void HttpRequest::setTransferTimeout( int timeoutms )
{
    hp_request_.getParam( this )->transfertimeoutms = timeoutms;
}


void HttpRequest::fillRequest( QNetworkRequest& req ) const
{
    req.setUrl( QUrl(url_.buf()) );
    if ( !contenttype_.isEmpty() )
	req.setHeader( QNetworkRequest::ContentTypeHeader, contenttype_.buf() );

    IOParIterator iter( rawheaders_ );
    BufferString key, val;
    while ( iter.next(key,val) )
	req.setRawHeader( key.buf(), val.buf() );

    if ( !encodeddata_.isEmpty() )
    {
	QUrlQuery query;
	IOParIterator dataiter( encodeddata_ );
	while ( dataiter.next(key,val) )
	    query.addQueryItem( key.buf(), val.buf() );

	HttpRequest* self = getNonConst( this );
	delete self->payload_;
	self->payload_ =
		new QByteArray( query.toString(QUrl::FullyEncoded).toUtf8() );
    }

    if ( payload_ )
	req.setHeader( QNetworkRequest::ContentLengthHeader, payload_->size() );

#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
    const int timeoutms = hp_request_.getParam( this )->transfertimeoutms;
    if ( timeoutms > 0 )
	req.setTransferTimeout( timeoutms );
#endif
}


static QNetworkProxy qNetworkProxy( const HttpProxyConfig& cfg )
{
    QNetworkProxy proxy;
    if ( cfg.type_ == HttpProxyConfig::Type::Http ||
	 cfg.type_ == HttpProxyConfig::Type::Socks5 )
    {
	proxy.setType( cfg.type_ == HttpProxyConfig::Type::Socks5
		? QNetworkProxy::Socks5Proxy : QNetworkProxy::HttpProxy );
	const BufferString host = cfg.authority_.getHost();
	proxy.setHostName( host.buf() );
	proxy.setPort( cfg.authority_.getPort() );
	if ( !cfg.username_.isEmpty() )
	{
	    proxy.setUser( cfg.username_.buf() );
	    proxy.setPassword( cfg.password_.buf() );
	}

	return proxy;
    }

    proxy.setType( QNetworkProxy::NoProxy );
    return proxy;
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
	deleteAndNullPtr( thread_ );
    }
}


RefMan<HttpRequestProcess> HttpRequestManager::get( const char* url )
{
    RefMan<HttpRequest> req = new HttpRequest( url, HttpRequest::Get );
    return request( req.ptr() );
}


RefMan<HttpRequestProcess> HttpRequestManager::head( const char* url )
{
    RefMan<HttpRequest> req = new HttpRequest( url, HttpRequest::Head );
    return request( req.ptr() );
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

    if ( !success )
	return nullptr;

    return res;
}


void HttpRequestManager::threadFuncCB(CallBacker*)
{
#ifndef OD_NO_QT
    createReceiverForCurrentThread();

    //Create outside of look as we'll trigger events and stuff in Qt.
    //The proxy is set on this access manager per request, so the
    //application proxy used by OpendTect sockets stays unchanged.
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
    deleteAndNullPtr( eventloop_ );
#endif
}


void HttpRequestManager::doRequestCB( CallBacker* cb )
{
    RefMan<HttpRequestProcess> reply = sCast(HttpRequestProcess*,cb);
    activeeventslock_.lock();
    if ( !activeevents_.isPresent( reply.ptr() ) )
    {
	activeeventslock_.unLock();
	return;
    }

    activeeventslock_.unLock();

    QNetworkRequest qreq;
    reply->request_->fillRequest(qreq);

    HttpProxyConfig proxy = reply->request_->proxyConfig();
    if ( proxy.type_ == HttpProxyConfig::Type::Unset )
	proxy = preferredDownloadProxy( reply->request_->url() );

    qnam_->setProxy( qNetworkProxy(proxy) );

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
