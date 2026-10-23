/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odnetworkaccess.h"

#include "databuf.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "hiddenparam.h"
#include "iopar.h"
#include "networkcommon.h"
#include "od_istream.h"
#include "od_ostream.h"
#include "oddirs.h"
#include "opensslaccess.h"
#include "separstr.h"
#include "settings.h"
#include "task.h"
#include "thread.h"
#include "stringview.h"
#include "typeset.h"

# include <QByteArray>
# include <QNetworkProxy>
# include <QNetworkProxyFactory>
# include <QNetworkProxyQuery>
# include <QUrl>


static const int cMaxTransferRetries = 5;
static const int cProxyProbeTimeoutMs = 5000;


static double retryDelaySec( od_int64 filesz, int attempt, double maxdelaysec )
{
    const double base = filesz < (od_int64)mDef1MB ? 0.5 : 2.0;
    double delay = base;
    for ( int i=1; i<attempt; i++ )
	delay *= 2.;

    if ( delay > maxdelaysec )
	delay = maxdelaysec;

    const double jitterfrac = 0.1 * ( ( attempt % 5 ) - 2 ) / 2.;
    delay *= ( 1. + jitterfrac );
    return delay;
}


struct DownloadRoutes
{
    bool					askproxydialog	= true;
    ManagedObjectSet<Network::HttpProxyConfig>	urlproxies;
};


class FileDownloaderTask : public SequentialTask
{ mODTextTranslationClass(FileDownloaderTask);
public:
			FileDownloaderTask(const char* url);
			FileDownloaderTask(const char* url,DataBuffer&);
			FileDownloaderTask(const BufferStringSet& urls,
				       const BufferStringSet& outputpaths);
			~FileDownloaderTask();

    od_int64		getDownloadSize();

    uiString		uiMessage() const override;
    uiString		uiNrDoneText() const override;
    uiRetVal		allMessages() const;
    bool		hasFails() const;
    void		setContinueOnFail( bool yn )  { continueonfail_ = yn; }

private:
    od_int64		nrDone() const override;
    od_int64		totalNr() const override;
    bool		doPrepare(od_ostream* =nullptr) override;
    int			nextStep() override;
    bool		doFinish(bool success,od_ostream* =nullptr) override;

    void		setSaveAsPaths(const BufferStringSet&,const char*);
    void		readTimeOutFromSettings();
    int			errorOccured();
    bool		startDownload();
    bool		checkRangeResponse();
    void		handleRangeNotSatisfiable();
    void		handleFullResponseAfterRange();

    bool		writeData();
    bool		writeDataToFile(const char* buffer,int size);
    bool		writeDataToBuffer(const char* buffer,int size);

    bool		initneeded_ = true;
    bool		continueonfail_ = false;
    bool		waitingforretry_ = false;
    bool		resuming_ = false;
    bool		rangerequested_ = false;
    bool		rangestatuschecked_ = false;
    bool		forcefullrestart_ = false;
    BufferStringSet	urls_;
    BufferStringSet	saveaspaths_;
    TypeSet<od_int64>	filesizes_;
    int			currurlidx_	    = 0;
    int			nrfilesdownloaded_  = 0;
    int			retrycount_	    = 0;
    double		maxretrydelaysec_  =
				Network::sKeyTimeOutMs() / 1000.;
    DataBuffer*		databuffer_ = nullptr;
    od_ostream*		osd_ = nullptr;

    RefMan<Network::HttpRequestProcess> odnr_;

    od_int64		nrdone_ = 0;
    od_int64		nrdoneaturlstart_ = 0;
    od_int64		bytesondisk_ = 0;
    od_int64		currfilesize_ = -1;
    od_int64		totalnr_ = 0;
    double		retrywaitremaining_ = 0.;
    uiString		msg_;
    uiRetVal		uirv_;
    uiRetVal		retryattemptmsgs_;
    DownloadRoutes	routes_;
};


FileDownloaderTask::FileDownloaderTask( const BufferStringSet& urls,
				const BufferStringSet& outputpaths )
    : SequentialTask("Downloading files")
    , osd_(new od_ostream())
    , saveaspaths_(outputpaths)
    , urls_(urls)
{
    OD::OpenSSLAccess::loadOpenSSL(); //Keep at the first line
    readTimeOutFromSettings();
    totalnr_ = getDownloadSize();
}


FileDownloaderTask::FileDownloaderTask( const char* url, DataBuffer& db )
    : SequentialTask("Downloading file")
    , databuffer_(&db)
{
    OD::OpenSSLAccess::loadOpenSSL(); //Keep at the first line
    readTimeOutFromSettings();
    urls_.add( url );
    totalnr_ = getDownloadSize();
}


FileDownloaderTask::FileDownloaderTask( const char* url )
    : SequentialTask("Downloading file")
    , osd_(new od_ostream())
{
    OD::OpenSSLAccess::loadOpenSSL(); //Keep at the first line
    readTimeOutFromSettings();
    urls_.add(url);
    routes_.askproxydialog = false;
}


FileDownloaderTask::~FileDownloaderTask()
{
    delete osd_;
}


void FileDownloaderTask::readTimeOutFromSettings()
{
    int timeoutms = Network::sKeyTimeOutMs();
    Settings::common().get( Network::cKeyDLTimeOut(), timeoutms );
    if ( timeoutms <= 0 )
	timeoutms = Network::sKeyTimeOutMs();

    maxretrydelaysec_ = timeoutms / 1000.;
}


uiRetVal FileDownloaderTask::allMessages() const
{
    if ( uirv_.isOK() )
	return uiRetVal::OK();

    uiRetVal uirv( msg_ );
    uirv.add( uirv_ );
    return uirv;
}


bool FileDownloaderTask::doPrepare( od_ostream* strm )
{
    if ( totalnr_ < 1 )
	totalnr_ = 1;

    if ( !continueonfail_ )
    {
	for ( int idx=0; idx<filesizes_.size(); idx++ )
	{
	    if ( filesizes_[idx] < 0 )
	    {
		msg_ = tr( "Cannot determine download size" );
		return false;
	    }
	}
    }

    return SequentialTask::doPrepare( strm );
}


int FileDownloaderTask::nextStep()
{
    if ( waitingforretry_ )
    {
	if ( !shouldContinue() )
	    return ErrorOccurred();

	const double slice = 0.1;
	retrywaitremaining_ -= slice;
	if ( retrywaitremaining_ > 0. )
	{
	    Threads::sleep( slice );
	    return MoreToDo();
	}

	waitingforretry_ = false;
	initneeded_ = true;
    }

    if ( initneeded_ )
    {
	initneeded_ = false;
	if ( !urls_.validIdx(currurlidx_) )
	    return Finished();

	if ( filesizes_.validIdx(currurlidx_) && filesizes_[currurlidx_] < 0 )
	{
	    // HEAD already failed after retries: never attempt GET
	    currurlidx_++;
	    initneeded_ = true;
	    return MoreToDo();
	}

	if ( !startDownload() )
	    return ErrorOccurred();
    }

    if ( odnr_->isError() )
    {
	if ( rangerequested_ && odnr_->httpStatusCode()==416 )
	{
	    handleRangeNotSatisfiable();
	    initneeded_ = true;
	    return MoreToDo();
	}

	return errorOccured();
    }

    if ( !checkRangeResponse() )
	return ErrorOccurred();

    if ( !odnr_ )
	return MoreToDo();

    if ( !writeData() )
	return ErrorOccurred();

    if ( odnr_->isFinished() )
    {
	if ( !checkRangeResponse() )
	    return ErrorOccurred();

	if ( !odnr_ )
	    return MoreToDo();

	// Check for any residue data received since the last read
	if ( !writeData() )
	    return ErrorOccurred();

	retrycount_ = 0;
	retryattemptmsgs_.setEmpty();
	forcefullrestart_ = false;
	initneeded_ = true;
	nrfilesdownloaded_++;
	currurlidx_++;
	if ( osd_ && osd_->isOK() )
	    osd_->close();
    }

    return MoreToDo();
}


bool FileDownloaderTask::doFinish( bool success, od_ostream* strm )
{
    odnr_ = nullptr;
    return SequentialTask::doFinish( success, strm );
}


static Threads::Mutex& sessionProxyMutex()
{
    static Threads::Mutex lock( false );
    return lock;
}


static PtrMan<Network::HttpProxyConfig>& sessionProxySlot()
{
    static PtrMan<Network::HttpProxyConfig> slot;
    return slot;
}


static void rememberSessionProxy( const Network::HttpProxyConfig& cfg )
{
    Threads::MutexLocker locker( sessionProxyMutex() );
    sessionProxySlot() = new Network::HttpProxyConfig( cfg );
}


static bool copySessionProxy( Network::HttpProxyConfig& cfg )
{
    Threads::MutexLocker locker( sessionProxyMutex() );
    if ( !sessionProxySlot().ptr() )
	return false;

    cfg = *sessionProxySlot().ptr();
    return true;
}


static const char* firstSetEnv( const char* primary, const char* secondary )
{
    const char* val = GetEnvVar( primary );
    if ( val && *val )
	return val;

    val = GetEnvVar( secondary );
    return val && *val ? val : nullptr;
}


static const char* noProxyEnv()
{
    return firstSetEnv( "no_proxy", "NO_PROXY" );
}


bool Network::proxyExcludesHost( const char* host, const char* noproxy )
{
    if ( !host || !*host || !noproxy || !*noproxy )
	return false;

    BufferString list( noproxy );
    list.replace( '\t', ',' );
    list.replace( ' ', ',' );
    list.replace( ';', ',' );
    const SeparString tokens( list.buf(), ',' );
    for ( int idx=0; idx<tokens.size(); idx++ )
    {
	BufferString token( tokens[idx].buf() );
	token.trimBlanks();
	if ( token.isEmpty() )
	    continue;

	if ( token == "*" )
	    return true;

	if ( token.firstChar() == '.' )
	{
	    const BufferString stripped( token.buf() + 1 );
	    token = stripped;
	}

	if ( token.isEmpty() )
	    continue;

	if ( StringView(host).isEqual(token.buf(),OD::CaseInsensitive) )
	    return true;

	const BufferString suffix( ".", token.buf() );
	if ( StringView(host).endsWith(suffix.buf(),OD::CaseInsensitive) )
	    return true;
    }

    return false;
}


bool Network::proxySpecToConfig( const char* spec, HttpProxyConfig& cfg )
{
    cfg = HttpProxyConfig();
    if ( !spec || !*spec )
	return false;

    BufferString txt( spec );
    txt.trimBlanks();
    if ( txt.isEmpty() )
	return false;

    if ( !txt.contains("://") )
	txt = BufferString( "http://", txt.buf() );

    const QUrl qurl( txt.buf() );
    if ( !qurl.isValid() || qurl.host().isEmpty() )
	return false;

    const BufferString scheme( qurl.scheme() );
    if ( scheme.isEqual("socks5",OD::CaseInsensitive) ||
	 scheme.isEqual("socks5h",OD::CaseInsensitive) ||
	 scheme.isEqual("socks",OD::CaseInsensitive) )
	cfg.type_ = HttpProxyConfig::Type::Socks5;
    else if ( scheme.isEmpty() ||
	      scheme.isEqual("http",OD::CaseInsensitive) ||
	      scheme.isEqual("https",OD::CaseInsensitive) )
	cfg.type_ = HttpProxyConfig::Type::Http;
    else
	return false;

    const BufferString host( qurl.host() );
    const int port = qurl.port( -1 );
    cfg.authority_.setHost( host.buf() )
		  .setPort( mCast(PortNr_Type,port > 0 ? port : 8080) );
    cfg.username_ = BufferString( qurl.userName() );
    cfg.password_ = BufferString( qurl.password() );
    return !cfg.authority_.getHost().isEmpty();
}


bool Network::environmentProxyFor( const char* url, HttpProxyConfig& cfg )
{
    cfg = HttpProxyConfig();
    const char* spec = nullptr;
    if ( url && *url )
    {
	const QUrl qurl( url );
	const BufferString host( qurl.host() );
	const char* noproxy = noProxyEnv();
	if ( noproxy && proxyExcludesHost(host.buf(),noproxy) )
	    return false;

	const BufferString scheme( qurl.scheme() );
	if ( scheme.isEqual("https",OD::CaseInsensitive) )
	    spec = firstSetEnv( "https_proxy", "HTTPS_PROXY" );
	else
	    spec = firstSetEnv( "http_proxy", "HTTP_PROXY" );
    }
    else
    {
	spec = firstSetEnv( "https_proxy", "HTTPS_PROXY" );
	if ( !spec )
	    spec = firstSetEnv( "http_proxy", "HTTP_PROXY" );
    }

    if ( !spec )
	spec = firstSetEnv( "all_proxy", "ALL_PROXY" );

    if ( !spec )
	return false;

    return proxySpecToConfig( spec, cfg );
}


struct UnsavedProxyPassword
{
    Network::Authority	authority;
    BufferString	password;
};

static Threads::Lock& unsavedProxyLock()
{
    static Threads::Lock lock;
    return lock;
}

static UnsavedProxyPassword& unsavedProxySlot()
{
    static UnsavedProxyPassword pwd;
    return pwd;
}

static bool sameUnsavedAccount( const Network::Authority& stored,
				const char* host, int port,
				const char* username )
{
    Network::Authority query( host, mCast(PortNr_Type,port) );
    query.setUserInfo( username );
    return stored.getPort()==query.getPort() &&
	   stored.getUserInfo()==query.getUserInfo() &&
	   stored.getHost().isEqual( query.getHost().buf(),
				     OD::CaseInsensitive );
}

static void clearUnsavedProxyPassword()
{
    Threads::Locker locker( unsavedProxyLock() );
    unsavedProxySlot() = UnsavedProxyPassword();
}

static void setUnsavedProxyPassword( const char* host, int port,
				     const char* username,
				     const char* password )
{
    Threads::Locker locker( unsavedProxyLock() );
    UnsavedProxyPassword& pwd = unsavedProxySlot();
    if ( !password || !*password )
    {
	if ( !sameUnsavedAccount(pwd.authority,host,port,username) )
	    pwd = UnsavedProxyPassword();
	return;
    }

    pwd.authority = Network::Authority( host, mCast(PortNr_Type,port) );
    pwd.authority.setUserInfo( username );
    pwd.password = password;
}

static void fillUnsavedProxyPassword( const char* host, int port,
				      const char* username,
				      BufferString& password )
{
    Threads::Locker locker( unsavedProxyLock() );
    const UnsavedProxyPassword& pwd = unsavedProxySlot();
    if ( pwd.password.isEmpty() ||
	 !sameUnsavedAccount(pwd.authority,host,port,username) )
	return;

    password = pwd.password;
}

static bool readStoredProxy( Network::HttpProxyConfig& cfg )
{
    cfg = Network::HttpProxyConfig();
    Settings& setts = Settings::common();
    bool useproxy = false;
    setts.getYN( Network::sKeyUseProxy(), useproxy );
    if ( !useproxy )
	return false;

    BufferString host;
    setts.get( Network::sKeyProxyHost(), host );
    if ( host.isEmpty() )
	return false;

    int port = 1;
    setts.get( Network::sKeyProxyPort(), port );
    // Port 1 is the value stored when no proxy was configured.
    if ( port<=1 || port>65535 )
	return false;

    cfg.type_ = Network::HttpProxyConfig::Type::Http;
    cfg.authority_.setHost( host.buf() ).setPort( mCast(PortNr_Type,port) );

    bool auth = false;
    setts.getYN( Network::sKeyUseAuthentication(), auth );
    if ( !auth )
	return true;

    setts.get( Network::sKeyProxyUserName(), cfg.username_ );
    BufferString password;
    bool iscrypt = false;
    setts.get( Network::sKeyProxyPassword(), password );
    if ( setts.getYN(Network::sKeyCryptProxyPassword(),iscrypt) && iscrypt )
    {
	uiString str;
	str.setFromHexEncoded( password );
	password = toString( str );
    }

    if ( password.isEmpty() )
	fillUnsavedProxyPassword( host.buf(), port, cfg.username_.buf(),
				  password );

    cfg.password_ = password;
    return true;
}


static Network::HttpProxyConfig configFromQueryProxy( const QNetworkProxy& qp )
{
    Network::HttpProxyConfig cfg;
    if ( qp.type()==QNetworkProxy::HttpProxy ||
	 qp.type()==QNetworkProxy::HttpCachingProxy )
	cfg.type_ = Network::HttpProxyConfig::Type::Http;
    else if ( qp.type() == QNetworkProxy::Socks5Proxy )
	cfg.type_ = Network::HttpProxyConfig::Type::Socks5;
    else
	return cfg;

    if ( qp.hostName().isEmpty() )
    {
	cfg.type_ = Network::HttpProxyConfig::Type::Unset;
	return cfg;
    }

    const BufferString host( qp.hostName() );
    const int port = qp.port() > 0 ? qp.port() : 8080;
    cfg.authority_.setHost( host.buf() ).setPort( mCast(PortNr_Type,port) );
    cfg.username_ = BufferString( qp.user() );
    cfg.password_ = BufferString( qp.password() );
    return cfg;
}


static bool sameProxyEndpoint( const Network::HttpProxyConfig& a,
			       const Network::HttpProxyConfig& b )
{
    if ( a.type_ != b.type_ )
	return false;

    if ( a.type_ == Network::HttpProxyConfig::Type::Direct )
	return true;

    return a.authority_.getPort()==b.authority_.getPort() &&
	   a.authority_.getHost().isEqual( b.authority_.getHost().buf(),
					   OD::CaseInsensitive );
}


static void addProxyCandidate(
		ManagedObjectSet<Network::HttpProxyConfig>& cands,
		const Network::HttpProxyConfig& cfg )
{
    if ( cfg.type_ == Network::HttpProxyConfig::Type::Unset )
	return;

    if ( cfg.type_!=Network::HttpProxyConfig::Type::Direct &&
	 cfg.authority_.getHost().isEmpty() )
	return;

    for ( int idx=0; idx<cands.size(); idx++ )
    {
	Network::HttpProxyConfig& have = *cands.get( idx );
	if ( !sameProxyEndpoint(have,cfg) )
	    continue;

	const bool takeuser = have.username_.isEmpty() &&
			      !cfg.username_.isEmpty();
	const bool takepwd = have.password_.isEmpty() &&
			     !cfg.password_.isEmpty() &&
			     (takeuser || have.username_==cfg.username_);
	if ( takeuser )
	    have.username_ = cfg.username_;
	if ( takepwd )
	    have.password_ = cfg.password_;

	return;
    }

    cands.add( new Network::HttpProxyConfig(cfg) );
}


static Network::HttpProxyConfig* newDirectProxy()
{
    auto* cfg = new Network::HttpProxyConfig;
    cfg->type_ = Network::HttpProxyConfig::Type::Direct;
    return cfg;
}


static void collectDownloadProxies( const char* url,
			ManagedObjectSet<Network::HttpProxyConfig>& cands )
{
    cands.erase();
    const QUrl qurl( url ? url : "" );
    const BufferString host( qurl.host() );
    const char* noproxy = noProxyEnv();
    if ( noproxy && Network::proxyExcludesHost(host.buf(),noproxy) )
    {
	cands.add( newDirectProxy() );
	return;
    }

    Network::HttpProxyConfig session;
    if ( copySessionProxy(session) )
	addProxyCandidate( cands, session );

    Network::HttpProxyConfig envproxy;
    if ( Network::environmentProxyFor(url,envproxy) )
	addProxyCandidate( cands, envproxy );

    const QNetworkProxyQuery query( qurl );
    const QList<QNetworkProxy> systemproxies =
			QNetworkProxyFactory::systemProxyForQuery( query );
    for ( int idx=0; idx<systemproxies.size(); idx++ )
	addProxyCandidate( cands, configFromQueryProxy(systemproxies.at(idx)) );

    Network::HttpProxyConfig direct;
    direct.type_ = Network::HttpProxyConfig::Type::Direct;
    addProxyCandidate( cands, direct );

    Network::HttpProxyConfig stored;
    if ( readStoredProxy(stored) )
	addProxyCandidate( cands, stored );
}


Network::HttpProxyConfig Network::preferredDownloadProxy( const char* url )
{
    ManagedObjectSet<HttpProxyConfig> cands;
    collectDownloadProxies( url, cands );
    if ( cands.isEmpty() )
    {
	HttpProxyConfig direct;
	direct.type_ = HttpProxyConfig::Type::Direct;
	return direct;
    }

    return *cands.first();
}


static bool routeCanDownload( Network::HttpRequestProcess::FailureKind kind,
			      int status )
{
    if ( kind == Network::HttpRequestProcess::FailureKind::NoFailure )
	return true;

    return kind==Network::HttpRequestProcess::FailureKind::HttpFailure &&
	   ( status==405 || status==501 );
}


static bool routeReachedServer( Network::HttpRequestProcess::FailureKind kind,
				int status )
{
    if ( kind == Network::HttpRequestProcess::FailureKind::NoFailure )
	return true;

    return kind==Network::HttpRequestProcess::FailureKind::HttpFailure &&
	   status>=200 && status!=407;
}


static bool proxydialogdismissed_ = false;


static bool probeHead( const char* url, const Network::HttpProxyConfig& proxy,
		Network::HttpRequestProcess::FailureKind& kind,
		int& status, od_int64& size, uiRetVal& errmsg, uiString& msg )
{
    kind = Network::HttpRequestProcess::FailureKind::ConnectFailure;
    status = 0;
    size = -1;
    if ( proxy.type_ == Network::HttpProxyConfig::Type::Direct )
	msg = od_static_tr( "probeHead", "Determining size of %1" ).arg( url );
    else
    {
	const BufferString proxyhost = proxy.authority_.getHost();
	msg = od_static_tr( "probeHead", "Determining size of %1 via %2" )
		    .arg( url ).arg( proxyhost );
    }

    RefMan<Network::HttpRequest> req =
	new Network::HttpRequest( url, Network::HttpRequest::Head );
    req->setProxyConfig( proxy );
    req->setTransferTimeout( cProxyProbeTimeoutMs );
    RefMan<Network::HttpRequestProcess> odnr =
		Network::HttpRequestManager::instance().request( req.ptr() );
    if ( !odnr )
    {
	errmsg = od_static_tr( "probeHead",
			       "Cannot start request for %1" ).arg( url );
	return false;
    }

    odnr->waitForFinish( cProxyProbeTimeoutMs + 5000 );
    if ( odnr->isRunning() )
    {
	errmsg = od_static_tr( "probeHead",
			       "Connection to %1 timed out" ).arg( url );
	return true;
    }

    status = odnr->httpStatusCode();
    if ( odnr->isError() )
	kind = odnr->failureKind();
    else
	kind = Network::HttpRequestProcess::FailureKind::NoFailure;
    if ( status == 407 )
	kind = Network::HttpRequestProcess::FailureKind::ProxyAuthFailure;

    size = odnr->getContentLengthHeader();
    if ( mIsUdf(size) || size < 0 )
	size = 0;

    if ( odnr->isError() )
	errmsg = odnr->errMsgs();

    return true;
}


static bool selectRoute( const char* url, DownloadRoutes& routes,
			 od_int64& filesize, uiRetVal& errs, uiString& msg )
{
    ManagedObjectSet<Network::HttpProxyConfig> candidates;
    collectDownloadProxies( url, candidates );
    bool sawconnect = false;
    bool sawssl = false;
    for ( int idx=0; idx<candidates.size(); idx++ )
    {
	const Network::HttpProxyConfig& cand = *candidates.get( idx );
	Network::HttpRequestProcess::FailureKind kind =
		Network::HttpRequestProcess::FailureKind::ConnectFailure;
	int status = 0;
	od_int64 size = -1;
	uiRetVal attemptmsg;
	const bool started = probeHead( url, cand, kind, status, size,
					attemptmsg, msg );
	if ( !started )
	{
	    sawconnect = true;
	    if ( !attemptmsg.isOK() )
		errs.add( attemptmsg );
	    continue;
	}

	if ( routeReachedServer(kind,status) && !routeCanDownload(kind,status) )
	{
	    rememberSessionProxy( cand );
	    routes.urlproxies.add( new Network::HttpProxyConfig(cand) );
	    filesize = -1;
	    if ( !attemptmsg.isOK() )
		errs.add( attemptmsg );
	    else
		errs.add( od_static_tr("selectRoute",
				       "Server rejected %1").arg(url) );

	    return false;
	}

	if ( routeCanDownload(kind,status) )
	{
	    rememberSessionProxy( cand );
	    routes.urlproxies.add( new Network::HttpProxyConfig(cand) );
	    filesize = size < 0 ? 0 : size;
	    return true;
	}

	if ( kind == Network::HttpRequestProcess::FailureKind::SslFailure )
	    sawssl = true;
	else
	    sawconnect = true;

	if ( !attemptmsg.isOK() )
	    errs.add( attemptmsg );
    }

    if ( sawssl && !sawconnect )
    {
	errs.add( od_static_tr("selectRoute",
		"The server certificate could not be verified.") );
	routes.urlproxies.add( newDirectProxy() );
	filesize = -1;
	return false;
    }

    if ( routes.askproxydialog && !proxydialogdismissed_ &&
	 (sawconnect || candidates.isEmpty()) )
    {
	if ( !Network::getProxySettingsFromUser() )
	    proxydialogdismissed_ = true;
	else
	{
	    Network::HttpProxyConfig manual;
	    if ( readStoredProxy(manual) )
	    {
		Network::HttpRequestProcess::FailureKind kind =
		Network::HttpRequestProcess::FailureKind::ConnectFailure;
		int status = 0;
		od_int64 size = -1;
		uiRetVal attemptmsg;
		probeHead( url, manual, kind, status, size, attemptmsg, msg );
		if ( routeCanDownload(kind,status) )
		{
		    rememberSessionProxy( manual );
		    routes.urlproxies.add(
				new Network::HttpProxyConfig(manual) );
		    filesize = size < 0 ? 0 : size;
		    return true;
		}

		if ( routeReachedServer(kind,status) )
		{
		    rememberSessionProxy( manual );
		    routes.urlproxies.add(
				new Network::HttpProxyConfig(manual) );
		    filesize = -1;
		    if ( !attemptmsg.isOK() )
			errs.add( attemptmsg );

		    return false;
		}

		if ( !attemptmsg.isOK() )
		    errs.add( attemptmsg );
	    }
	}
    }

    routes.urlproxies.add( newDirectProxy() );
    filesize = -1;
    return false;
}


static HiddenParam<FileDownloader,DownloadRoutes*> hp_filedl_( nullptr );


static DownloadRoutes& fileDownloaderRoutes( FileDownloader& dl )
{
    return *hp_filedl_.getParam( &dl );
}


od_int64 FileDownloaderTask::getDownloadSize()
{
    od_int64 totalbytes = 0;
    filesizes_.setEmpty();
    routes_.urlproxies.erase();
    bool anyfailed = false;
    for ( int idx=0; idx<urls_.size(); idx++ )
    {
	const char* url = urls_.get( idx ).buf();
	od_int64 filesize = -1;
	uiRetVal headerr;
	if ( !selectRoute(url,routes_,filesize,headerr,msg_) )
	{
	    anyfailed = true;
	    uirv_.add( tr("Cannot determine size of %1").arg(url) );
	    if ( !headerr.isOK() )
		uirv_.add( headerr );
	    if ( !routes_.urlproxies.validIdx(idx) )
		routes_.urlproxies.add( newDirectProxy() );
	}
	else if ( !mIsUdf(filesize) && filesize > 0 )
	    totalbytes += filesize;

	filesizes_ += filesize;
    }

    odnr_ = nullptr;
    if ( anyfailed && urls_.size() == 1 )
    {
	msg_ = tr( "Cannot determine download size" );
	return -1;
    }

    msg_.setEmpty();
    return totalbytes;
}


bool FileDownloaderTask::startDownload()
{
    rangestatuschecked_ = false;
    resuming_ = false;

    if ( retrycount_ == 0 )
	nrdoneaturlstart_ = nrdone_;

    currfilesize_ = filesizes_.validIdx(currurlidx_)
		  ? filesizes_[currurlidx_] : mUdf(od_int64);

    const char* url = urls_.get( currurlidx_ ).buf();
    bool userange = false;

    if ( databuffer_ )
    {
	if ( retrycount_ > 0 )
	{
	    nrdone_ = nrdoneaturlstart_;
	    databuffer_->reSize( 0, false );
	}
    }
    else if ( saveaspaths_.validIdx(currurlidx_) )
    {
	const FilePath fp( saveaspaths_.get(currurlidx_) );
	const BufferString dest = fp.fullPath();

	if ( forcefullrestart_ ||
	     (retrycount_>0 && currfilesize_<(od_int64)mDef1MB) )
	{
	    if ( osd_ && osd_->isOK() )
		osd_->close();

	    if ( File::exists(dest) )
		File::remove( dest );

	    nrdone_ = nrdoneaturlstart_;
	    bytesondisk_ = 0;
	}
	else if ( retrycount_>0 && currfilesize_>=(od_int64)mDef1MB )
	{
	    const od_int64 ondisk = File::exists(dest)
				  ? File::getFileSize(dest) : 0;
	    if ( ondisk>0 && ondisk<currfilesize_ )
	    {
		userange = true;
		bytesondisk_ = ondisk;
		resuming_ = true;
	    }
	}

	if ( userange )
	{
	    if ( !File::exists(fp.pathOnly()) )
		File::createDir( fp.pathOnly() );

	    if ( osd_->isOK() )
		osd_->close();

	    osd_->open( dest, true );
	    if ( osd_->isBad() )
	    {
		uirv_.add( tr("Didn't have permission to write to: %1")
			  .arg(dest) );
		return false;
	    }
	}
	else if ( osd_ && osd_->isOK() )
	    osd_->close();
    }

    RefMan<Network::HttpRequest> req =
	new Network::HttpRequest( url, Network::HttpRequest::Get );
    if ( routes_.urlproxies.validIdx(currurlidx_) &&
	 routes_.urlproxies.get(currurlidx_) )
	req->setProxyConfig( *routes_.urlproxies.get(currurlidx_) );
    else
	req->setProxyConfig( Network::preferredDownloadProxy(url) );

    if ( userange )
    {
	BufferString hdrstr( "bytes=", bytesondisk_, "-" );
	req->setRawHeader( "Range", hdrstr.str() );
	rangerequested_ = true;
    }
    else
	rangerequested_ = false;

    odnr_ = Network::HttpRequestManager::instance().request( req.ptr() );
    if ( !odnr_ )
    {
	uirv_.add( tr("Cannot start request for %1").arg(url) );
	return false;
    }

    if ( retrycount_ > 0 )
    {
	msg_ = tr("Retrying download (%1/%2): %3")
		    .arg( retrycount_ ).arg( cMaxTransferRetries ).arg( url );
    }
    else
	msg_ = tr( "Downloading %1" ).arg( url );

    return true;
}


bool FileDownloaderTask::checkRangeResponse()
{
    if ( !rangerequested_ || rangestatuschecked_ || !odnr_ )
	return true;

    const int code = odnr_->httpStatusCode();
    if ( !code && !odnr_->isFinished() && !odnr_->downloadBytesAvailable() )
	return true;

    rangestatuschecked_ = true;

    if ( code==416 )
    {
	handleRangeNotSatisfiable();
	return true;
    }

    if ( code==200 )
	handleFullResponseAfterRange();

    return true;
}


void FileDownloaderTask::handleRangeNotSatisfiable()
{
    forcefullrestart_ = true;
    rangerequested_ = false;
    resuming_ = false;
    bytesondisk_ = 0;

    if ( osd_ && osd_->isOK() )
	osd_->close();

    if ( saveaspaths_.validIdx(currurlidx_) )
    {
	const FilePath fp( saveaspaths_.get(currurlidx_) );
	if ( File::exists(fp.fullPath()) )
	    File::remove( fp.fullPath() );
    }

    nrdone_ = nrdoneaturlstart_;
    odnr_ = nullptr;
    initneeded_ = true;
}


void FileDownloaderTask::handleFullResponseAfterRange()
{
    resuming_ = false;
    bytesondisk_ = 0;
    rangerequested_ = false;

    if ( osd_ && osd_->isOK() )
	osd_->close();

    nrdone_ = nrdoneaturlstart_;
}


bool FileDownloaderTask::writeData()
{
    od_int64 bytes = odnr_->downloadBytesAvailable();
    if ( !bytes )
	return true;

    mAllocLargeVarLenArr( char, buffer, bytes );
    char* bufferptr = buffer.ptr();
    bytes = odnr_->read( bufferptr, bytes );
    nrdone_ += bytes;
    if ( databuffer_ )
	return writeDataToBuffer( bufferptr, bytes );
    else
	return writeDataToFile( bufferptr, bytes );
}


bool FileDownloaderTask::writeDataToFile( const char* buffer, int size )
{
    const FilePath fp = saveaspaths_.get( currurlidx_ ).buf();
    if ( !osd_ )
	return false;

    if ( osd_->isBad() )
    {
	if ( !File::exists(fp.pathOnly()) )
	    File::createDir( fp.pathOnly() );

	osd_->open( fp.fullPath() );
	if ( osd_->isBad() )
	{
	    uirv_.add( tr("%1 Didn't have permission to write to: %2")
		      .arg(osd_->isBad()).arg(fp.fullPath()) );
	    return false;
	}
    }

    osd_->addBin( buffer, size );
    return osd_->isOK();
}


bool FileDownloaderTask::writeDataToBuffer( const char* buffer, int size )
{
    if ( !databuffer_ )
	return false;

    int buffersize = databuffer_->size();
    databuffer_->reSize( nrdone_ );
    OD::memCopy( databuffer_->data()+buffersize, buffer, size );
    return true;
}


int FileDownloaderTask::errorOccured()
{
    uiRetVal uiretval;
    if ( odnr_ )
	uiretval.add( odnr_->errMsgs() );

    if ( uiretval.isEmpty() )
	uiretval.add( tr("Oops! Something went wrong with the connection") );

    odnr_ = nullptr;
    if ( osd_ && osd_->isOK() )
	osd_->close();

    retrycount_++;
    if ( retrycount_ <= cMaxTransferRetries )
    {
	retryattemptmsgs_.add( uiretval );
	retrywaitremaining_ = retryDelaySec( currfilesize_, retrycount_,
						     maxretrydelaysec_ );
	waitingforretry_ = true;
	return MoreToDo();
    }

    uirv_.add( retryattemptmsgs_ );
    uirv_.add( uiretval );
    retryattemptmsgs_.setEmpty();
    retrycount_ = 0;

    if ( continueonfail_ )
    {
	initneeded_ = true;
	currurlidx_++;
	return MoreToDo();
    }

    return ErrorOccurred();
}


uiString FileDownloaderTask::uiMessage() const
{ return msg_; }


od_int64 FileDownloaderTask::nrDone() const
{ return nrdone_/1024; }


uiString FileDownloaderTask::uiNrDoneText() const
{ return tr("KBytes downloaded"); }


od_int64 FileDownloaderTask::totalNr() const
{ return totalnr_/1024; }


uiRetVal Network::downloadFile_( const char* url, const char* path,
				  TaskRunner* taskr )
{
    BufferStringSet urls; urls.add( url );
    BufferStringSet outputpath; outputpath.add( path );
    return downloadFiles_( urls, outputpath, taskr );
}


uiRetVal Network::downloadFiles_( BufferStringSet& urls, const char* path,
				   TaskRunner* taskr, bool canfail )
{
    BufferStringSet outputpaths;
    for ( int idx=0; idx<urls.size(); idx++ )
    {
	SeparString str( urls.get(idx).buf(), '/' );
	FilePath destpath( path );
	if ( str[str.size()-1].isEmpty() )
	    destpath.add( str[str.size() - 2] );
	else
	    destpath.add( str[str.size() - 1] );

	outputpaths.add( destpath.fullPath() );
    }

    return downloadFiles_( urls, outputpaths, taskr, canfail );
}


uiRetVal Network::downloadFiles_( BufferStringSet& urls,
				   BufferStringSet& outputpaths,
				   TaskRunner* taskr, bool canfail )
{
    if ( urls.size() != outputpaths.size() )
    {
	uiRetVal uirv;
	uirv.add( od_static_tr("downloadFiles",
			       "urls size is not equal to output path size") );
	return uirv;
    }

    FileDownloaderTask dl( urls, outputpaths );
    dl.setContinueOnFail( canfail );
    const bool res = TaskRunner::execute( taskr, dl ) &&
					dl.allMessages().isOK();

    return res ? uiRetVal::OK() : dl.allMessages();
}


bool Network::exists( const char* url )
{
    od_int64 dum; uiString msg;
    return getRemoteFileSize( url, dum, msg );
}

od_int64 Network::getFileSize( const char* url )
{
    return getFileSize( url, nullptr );
}


od_int64 Network::getFileSize( const char* url, uiString* errmsg )
{
    od_int64 ret;
    uiString msg;
    uiString& theerrormsg = errmsg ? *errmsg : msg;
    return getRemoteFileSize( url, ret, theerrormsg ) ? ret : 0;
}

bool Network::getContent( const char* url, BufferString& str )
{
    return getContent( url, str, nullptr, nullptr );
}


bool Network::getContent( const char* url, BufferString& str,
			  uiString* errmsg, TaskRunner* taskrun )
{
    DataBuffer databuffer( 0, 1 );
    FileDownloaderTask dl( url, databuffer );
    const bool res = TaskRunner::execute( taskrun, dl );
    if ( res )
	str = databuffer.getString();
    else if ( errmsg )
	*errmsg = dl.allMessages().messages().cat();

    return res;
}


bool Network::downloadFile( const char* url, const char* path,
			    uiString& errmsg, TaskRunner* taskr )
{
    const uiRetVal uirv = downloadFile_( url, path, taskr );
    if ( uirv.isError() )
	errmsg = uirv.messages().cat();

    return uirv.isOK();
}


bool Network::downloadFiles( BufferStringSet& urls, const char* path,
			     uiString& errmsg, TaskRunner* taskr )
{
    const uiRetVal uirv = downloadFiles_( urls, path, taskr );
    if ( uirv.isError() )
	errmsg = uirv.messages().cat();

    return uirv.isOK();
}


bool Network::downloadFiles( BufferStringSet& urls,BufferStringSet& outputpaths,
			     uiString& errmsg, TaskRunner* taskr )
{
    const uiRetVal uirv = downloadFiles_( urls, outputpaths, taskr );
    if ( uirv.isError() )
	errmsg = uirv.messages().cat();

    return uirv.isOK();
}


bool Network::downloadToBuffer( const char* url, DataBuffer& databuf,
				uiString& errmsg, TaskRunner* taskr )
{
    BufferString bs;
    const uiRetVal uirv = downloadToString( url, bs, taskr );
    if ( uirv.isError() )
	errmsg = uirv.messages().cat();

    databuf.reSize( bs.size(), false );
    databuf.reByte( 1, false );
    if ( databuf.isOk() && !bs.isEmpty() )
	OD::memCopy( (char*)databuf.data(), bs.str(), databuf.size() );

    return uirv.isOK();
}


uiRetVal Network::downloadToBuffer_( const char* url, DataBuffer& databuf,
				     TaskRunner* taskrun )
{
    BufferString str;
    uiString errmsg;
    const bool res = getContent( url, str, &errmsg, taskrun );
    databuf.reSize( str.size(), false );
    databuf.reByte( 1, false );
    if ( databuf.isOk() && !str.isEmpty() )
	OD::memCopy( (char*)databuf.data(), str.str(), databuf.size() );

    return res ? uiString::empty() : errmsg;
}


uiRetVal Network::downloadToString( const char* url, BufferString& str,
				    TaskRunner* taskrun )
{
    uiString errmsg;
    const bool res = getContent( url, str, &errmsg, taskrun );
    return res ? uiString::empty() : errmsg;
}


bool Network::getRemoteFileSize( const char* url, od_int64& size,
				 uiString& errmsg )
{
    if ( !url || !*url )
	{ size = -1; return false; }

    FileDownloader dl( url );
    size = dl.getDownloadSize();
    if ( size < 0 )
    {
	errmsg = dl.uiMessage();
	return false;
    }

    return true;
}


bool Network::ping( const char* url, uiString& msg )
{
    od_int64 pseudosize;
    return getRemoteFileSize( url, pseudosize, msg );
}


FileDownloader::FileDownloader( const BufferStringSet& urls,
				const BufferStringSet& outputpaths )
    : SequentialTask("Downloading files")
    , osd_(new od_ostream())
    , saveaspaths_( outputpaths )
    , urls_( urls )
{
    OD::OpenSSLAccess::loadOpenSSL(); //Keep at the first line
    hp_filedl_.setParam( this, new DownloadRoutes );
    totalnr_ = getDownloadSize();
}


FileDownloader::FileDownloader( const char* url, DataBuffer& db )
    : SequentialTask("Downloading file")
    , databuffer_(&db)
{
    OD::OpenSSLAccess::loadOpenSSL(); //Keep at the first line
    hp_filedl_.setParam( this, new DownloadRoutes );
    urls_.add(url);
    totalnr_ = getDownloadSize();
}


FileDownloader::FileDownloader( const char* url )
    : SequentialTask("Downloading file")
    , osd_(new od_ostream())
{
    OD::OpenSSLAccess::loadOpenSSL(); //Keep at the first line
    auto* routes = new DownloadRoutes;
    routes->askproxydialog = false;
    hp_filedl_.setParam( this, routes );
    urls_.add(url);
}


FileDownloader::~FileDownloader()
{
    hp_filedl_.removeAndDeleteParam( this );
    delete osd_;
}


int FileDownloader::nextStep()
{
    if ( totalnr_ < 0 )
	return errorOccured();

    if ( initneeded_ )
    {
	initneeded_ = false;
	if ( !urls_.validIdx( nrfilesdownloaded_ ) )
	    return Finished();

	const char* url = urls_.get(nrfilesdownloaded_).buf();
	RefMan<Network::HttpRequest> req =
	    new Network::HttpRequest( url, Network::HttpRequest::Get );
	DownloadRoutes& routes = fileDownloaderRoutes( *this );
	if ( routes.urlproxies.validIdx(nrfilesdownloaded_) &&
	     routes.urlproxies.get(nrfilesdownloaded_) )
	    req->setProxyConfig( *routes.urlproxies.get(nrfilesdownloaded_) );
	else
	    req->setProxyConfig( Network::preferredDownloadProxy(url) );

	odnr_ = Network::HttpRequestManager::instance().request( req.ptr() );
	if ( !odnr_ )
	    return errorOccured();
    }

    if ( odnr_->isError() )
	return errorOccured();

    if ( !writeData() )
	return ErrorOccurred();

    if ( odnr_->isFinished() )
    {
	// Check for any residue data received since the last read
	if ( !writeData() )
	    return ErrorOccurred();

	initneeded_ = true;
	nrfilesdownloaded_++;
	if ( osd_ && osd_->isOK() )
	    osd_->close();
    }

    return MoreToDo();
}


od_int64 FileDownloader::getDownloadSize()
{
    DownloadRoutes& routes = fileDownloaderRoutes( *this );
    od_int64 totalbytes = 0;
    routes.urlproxies.erase();
    bool anyfailed = false;
    uiRetVal errs;
    for ( int idx=0; idx<urls_.size(); idx++ )
    {
	const char* url = urls_.get(idx).buf();
	od_int64 filesize = -1;
	uiRetVal headerr;
	if ( !selectRoute(url,routes,filesize,headerr,msg_) )
	{
	    anyfailed = true;
	    errs.add( tr("Cannot determine size of %1").arg(url) );
	    if ( !headerr.isOK() )
		errs.add( headerr );
	    if ( !routes.urlproxies.validIdx(idx) )
		routes.urlproxies.add( newDirectProxy() );
	}
	else if ( !mIsUdf(filesize) && filesize > 0 )
	    totalbytes += filesize;
    }

    odnr_ = nullptr;
    if ( anyfailed && urls_.size()==1 )
    {
	msg_ = errs.isOK() ? tr( "Cannot determine download size" )
			   : errs.messages().cat();
	return -1;
    }

    if ( !anyfailed )
	msg_.setEmpty();

    return totalbytes;
}


bool FileDownloader::writeData()
{
    od_int64 bytes = odnr_->downloadBytesAvailable();
    if ( !bytes )
	return true;

    mAllocLargeVarLenArr( char, buffer, bytes );
    char* bufferptr = buffer.ptr();
    bytes = odnr_->read( bufferptr, bytes );
    nrdone_ += bytes;
    if ( databuffer_ )
	return writeDataToBuffer( bufferptr, bytes );
    else
	return writeDataToFile( bufferptr, bytes );
}


bool FileDownloader::writeDataToFile(const char* buffer, int size)
{
    FilePath fp = saveaspaths_.get(nrfilesdownloaded_).buf();
    if ( !osd_ )
	return false;

    if ( osd_->isBad() )
    {
	if ( !File::exists(fp.pathOnly()) )
	    File::createDir( fp.pathOnly() );

	osd_->open( saveaspaths_.get(nrfilesdownloaded_) );
	if ( osd_->isBad() )
	{
	    msg_ = tr("%1 Didn't have permission to write to: %2")
		 .arg(osd_->isBad()).arg(fp.fullPath());
	    return false;
	}
    }

    osd_->addBin( buffer, size );
    return osd_->isOK();
}


bool FileDownloader::writeDataToBuffer(const char* buffer, int size)
{
    if ( !databuffer_ )
	return false;

    int buffersize = databuffer_->size();
    databuffer_->reSize(nrdone_);
    OD::memCopy( databuffer_->data()+buffersize, buffer, size );
    return true;
}


int FileDownloader::errorOccured()
{
    uiRetVal uirv;
    if ( odnr_ )
	uirv.add( odnr_->errMsgs() );

    if ( uirv.isEmpty() )
	uirv.add( tr( "Oops! Something went wrong with the connection" ) );

    msg_ = uirv.messages().cat();
    return ErrorOccurred();
}


uiString FileDownloader::uiMessage() const
{ return msg_; }


od_int64 FileDownloader::nrDone() const
{return nrdone_/1024;}


uiString FileDownloader::uiNrDoneText() const
{ return tr("KBytes downloaded"); }


od_int64 FileDownloader::totalNr() const
{ return totalnr_/1024; }


// upload

static const char* sFullContentBoundary = "-------742f683860774f225764f172";
static const char* sContentBoundary = sFullContentBoundary + 2;
static const char* sHttpEndStrNewline = "\"\r\n";
static const char* sHttpNewline = sHttpEndStrNewline + 1;

static void addContentStart( BufferString& httpstr, const char* contnm )
{
    httpstr.add( sFullContentBoundary ).add( sHttpNewline )
	   .add( "Content-Disposition: form-data; name=\"" )
	   .add( contnm ).add( "\"" );
}
static void addContentStop( BufferString& httpstr )
{
    httpstr.add( sFullContentBoundary );
}
static void addPars( BufferString& httpstr, const IOPar& postvars )
{
    IOParIterator iter( postvars );
    BufferString key, val;
    while ( iter.next(key,val) )
    {
	addContentStart( httpstr, key );
	httpstr.add( sHttpNewline ).add( sHttpNewline )
	       .add( val ).add( sHttpNewline );
    }

    addContentStop( httpstr );
}


bool Network::uploadFile( const char* url, const char* localfname,
			  const char* remotefname, const char* ftype,
			  const IOPar& postvars, uiString& errmsg,
			  TaskRunner* taskr, uiString* retmsg )
{
    if ( !File::isFile(localfname) )
    {
	errmsg = od_static_tr( "uploadFile", "%1\nFile not found" )
			       .arg( localfname );
	return false;
    }
    const od_int64 filesize = File::getFileSize( localfname );
    if ( filesize > INT_MAX )
    {
	errmsg = od_static_tr( "uploadFile", "%1\nFile too large for upload" )
			       .arg( localfname );
	return false;
    }

    BufferString startstr;
    addContentStart( startstr, ftype );
    startstr.add( "; filename=\"").add( remotefname ).add( sHttpEndStrNewline )
	  .add( "Content-Type: application/octet-stream\r\n\r\n" );
    BufferString stopstr( sHttpNewline );
    addContentStart( stopstr, "upload" );
    stopstr.add( "\r\n\r\nOpendTect\r\n" );
    addPars( stopstr, postvars );

    const int startsize = startstr.size();
    const int stopsize = stopstr.size();
    const od_int64 totalsize = startsize + filesize + stopsize;
    if ( totalsize > INT_MAX )
    {
	errmsg = od_static_tr( "uploadFile",
		    "%1\nFile just too large for upload" ).arg( localfname );
	return false;
    }

    PtrMan<DataBuffer> databuffer = new DataBuffer( (int)totalsize, 1 );
    unsigned char* buf = databuffer->data();
    OD::memCopy( buf, startstr.str(), startsize );
    od_istream inpstrm( localfname );
    inpstrm.getBin( buf + startsize, filesize );
    inpstrm.close();
    OD::memCopy( buf + startsize + filesize, stopstr.str(), stopsize );

    BufferString header( "multipart/form-data; boundary=", sContentBoundary );
    DataUploader up( url, *databuffer, header );
    const bool res = taskr ? taskr->execute( up ) : up.execute();
    if ( !res )
	errmsg = up.uiMessage();
    else if ( retmsg )
	*retmsg = up.uiMessage();
    return res;
}


bool Network::uploadQuery( const char* url, const IOPar& querypars,
			   uiString& errmsg, TaskRunner* taskr,
			   uiString* retmsg)
{
    BufferString data;
    addPars( data, querypars );
    DataBuffer db( data.size(), 1 );
    OD::memCopy( db.data(), data.buf(), data.size() );
    BufferString header( "multipart/form-data; boundary=", sContentBoundary );
    DataUploader up( url, db, header );
    const bool res = taskr ? taskr->execute( up ) : up.execute();
    if ( !res )
	errmsg = up.uiMessage();
    else if ( retmsg )
	*retmsg = up.uiMessage();
    return res;
}


struct UploadRoute
{
    Network::HttpProxyConfig	proxy;
    char			routeok		= 0;
    char			askproxydialog	= 1;
};

static HiddenParam<DataUploader,UploadRoute*> hp_upload_( nullptr );

DataUploader::DataUploader( const char* url, const DataBuffer& data,
			    BufferString& header )
    : SequentialTask("Uploading data")
    , data_( data )
    , url_(url)
    , header_(header)
    , maxretrydelaysec_( Network::sKeyTimeOutMs() / 1000. )
{
    OD::OpenSSLAccess::loadOpenSSL(); //Keep at the first line
    readTimeOutFromSettings();
    auto* route = new UploadRoute;
    hp_upload_.setParam( this, route );

    DownloadRoutes routes;
    routes.askproxydialog = route->askproxydialog;
    od_int64 filesize = -1;
    uiRetVal errs;
    uiString probedmsg;
    const bool ok = selectRoute( url_.buf(), routes, filesize, errs,
				 probedmsg );
    route->routeok = ok ? 1 : 0;
    if ( ok && !routes.urlproxies.isEmpty() )
	route->proxy = *routes.urlproxies.first();
    else if ( !errs.isOK() )
	msg_ = errs.messages().cat();
    else if ( !probedmsg.isEmpty() )
	msg_ = probedmsg;
    else if ( !ok )
	msg_ = tr( "Cannot connect to %1" ).arg( url_ );
}


DataUploader::~DataUploader()
{
    hp_upload_.removeAndDeleteParam( this );
}


void DataUploader::readTimeOutFromSettings()
{
    int timeoutms = Network::sKeyTimeOutMs();
    Settings::common().get( Network::cKeyULTimeOut(), timeoutms );
    if ( timeoutms <= 0 )
	timeoutms = Network::sKeyTimeOutMs();

    maxretrydelaysec_ = timeoutms / 1000.;
}


bool DataUploader::startUpload()
{
    UploadRoute* route = hp_upload_.getParam( this );
    if ( !route || !route->routeok )
	return false;

    nrdone_ = 0;
    RefMan<Network::HttpRequest> req = new Network::HttpRequest( url_,
					   Network::HttpRequest::Post );
    req->setProxyConfig( route->proxy );
    req->contentType( header_ );
    req->payloadData( data_ );
    odnr_ = Network::HttpRequestManager::instance().request( req.ptr() );

    if ( retrycount_ > 0 )
    {
	msg_ = tr("Retrying upload (%1/%2): %3")
		    .arg( retrycount_ ).arg( cMaxTransferRetries ).arg( url_ );
    }
    else
	msg_ = tr( "Uploading to %1" ).arg( url_ );

    return true;
}


int DataUploader::nextStep()
{
    if ( waitingforretry_ )
    {
	if ( !shouldContinue() )
	    return ErrorOccurred();

	const double slice = 0.1;
	retrywaitremaining_ -= slice;
	if ( retrywaitremaining_ > 0. )
	{
	    Threads::sleep( slice );
	    return MoreToDo();
	}

	waitingforretry_ = false;
	initneeded_ = true;
    }

    if ( initneeded_ )
    {
	initneeded_ = false;
	if ( !startUpload() )
	    return ErrorOccurred();
    }

    if ( odnr_->isError() )
	return errorOccured();

    if ( odnr_->isFinished() )
    {
	odnr_->waitForDownloadData( 500 );
	msg_ = toUiString( odnr_->readAll() );
	retrycount_ = 0;
	retryattemptmsgs_.setEmpty();
	return Finished();
    }

    if ( odnr_->isRunning() )
    {
	nrdone_ = odnr_->getBytesUploaded();
	totalnr_ = odnr_->getTotalBytesToUpload();
    }

    return MoreToDo();
}


int DataUploader::errorOccured()
{
    uiRetVal uiretval;
    if ( odnr_ )
	uiretval.add( odnr_->errMsgs() );

    if ( uiretval.isEmpty() )
	uiretval.add( tr( "Oops! Something went wrong with the connection" ) );

    odnr_ = nullptr;

    retrycount_++;
    if ( retrycount_ <= cMaxTransferRetries )
    {
	retryattemptmsgs_.add( uiretval );
	retrywaitremaining_ = retryDelaySec( data_.size(), retrycount_,
					     maxretrydelaysec_ );
	waitingforretry_ = true;
	return MoreToDo();
    }

    uiRetVal allerrs( retryattemptmsgs_ );
    allerrs.add( uiretval );
    msg_ = allerrs.messages().cat();
    retryattemptmsgs_.setEmpty();
    return ErrorOccurred();
}


uiString DataUploader::uiMessage() const
{ return msg_; }


od_int64 DataUploader::nrDone() const
{return nrdone_/1024;}


uiString DataUploader::uiNrDoneText() const
{ return tr("KBytes uploaded"); }


od_int64 DataUploader::totalNr() const
{ return totalnr_/1024; }


void Network::setHttpProxyFromSettings()
{
    Settings& setts = Settings::common();
    bool auth = false;
    setts.getYN( Network::sKeyUseAuthentication(), auth );
    if ( auth )
    {
	BufferString password;
	bool iscrypt = false;
	setts.get( Network::sKeyProxyPassword(), password );
	if ( !password.isEmpty() &&
	     !setts.getYN(Network::sKeyCryptProxyPassword(),iscrypt) )
	{
	    uiString str = toUiString( password );
	    str.getHexEncoded( password );
	    setts.set( Network::sKeyProxyPassword(), password );
	    setts.setYN( Network::sKeyCryptProxyPassword(), true );
	    setts.write();
	}
    }

    setHttpProxyFromIOPar( setts );
}


void Network::setHttpProxyFromIOPar( const IOPar& pars )
{
    bool useproxy = false;
    pars.getYN( Network::sKeyUseProxy(), useproxy );
    if ( !useproxy )
    {
	clearUnsavedProxyPassword();
	QNetworkProxy proxy;
	proxy.setType( QNetworkProxy::NoProxy );
	QNetworkProxy::setApplicationProxy( proxy );
	return;
    }

    BufferString host;
    pars.get( Network::sKeyProxyHost(), host );
    if ( host.isEmpty() )
    {
	clearUnsavedProxyPassword();
	setHttpProxy( nullptr, 0 );
	return;
    }

    int port = 1;
    pars.get( Network::sKeyProxyPort(), port );

    bool auth = false;
    pars.getYN( Network::sKeyUseAuthentication(), auth );

    if ( auth )
    {
	BufferString username;
	pars.get( Network::sKeyProxyUserName(), username );

	BufferString password;
	bool iscrypt = false;
	pars.get( Network::sKeyProxyPassword(), password );
	if ( pars.getYN(Network::sKeyCryptProxyPassword(),iscrypt) )
	{
	    uiString str;
	    str.setFromHexEncoded( password );
	    password = toString( str );
	}

	setUnsavedProxyPassword( host, port, username, password );
	Network::setHttpProxy( host, port, auth, username, password );
    }
    else
    {
	clearUnsavedProxyPassword();
	Network::setHttpProxy( host, port );
    }
}


bool Network::getProxySettingsFromUser()
{
    NetworkUserQuery* inst = NetworkUserQuery::getNetworkUserQuery();
    if ( !inst ) return false;

    return inst->setFromUser();
}

void Network::setHttpProxy( const char*, int, bool, const char*, const char* )
{
    // HTTP proxies cannot listen. Installing one as the application proxy
    // makes QTcpServer::listen() fail, so every candidate port looks busy.
    // Downloads apply the saved or environment proxy on each request.
    QNetworkProxy proxy;
    proxy.setType( QNetworkProxy::NoProxy );
    QNetworkProxy::setApplicationProxy( proxy );
}



// NetworkUserQuery
NetworkUserQuery::NetworkUserQuery()
{}


NetworkUserQuery::~NetworkUserQuery()
{}


NetworkUserQuery* NetworkUserQuery::inst_ = nullptr;

void NetworkUserQuery::setNetworkUserQuery( NetworkUserQuery* newinst )
{
    inst_ = newinst;
}

NetworkUserQuery* NetworkUserQuery::getNetworkUserQuery()
{
    return inst_;
}
