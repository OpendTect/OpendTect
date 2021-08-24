/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:	    Salil Agarwal
Date:	    October 2012
________________________________________________________________________

-*/

#include "odnetworkaccess.h"

#include "databuf.h"
#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "odnetworkreply.h"
#include "oscommand.h"
#include "od_ostream.h"
#include "od_istream.h"
#include "perthreadrepos.h"
#include "separstr.h"
#include "settings.h"
#include "sharedlibs.h"
#include "uistrings.h"

# include "qnetworkaccessconn.h"

# include <QByteArray>
# include <QEventLoop>
# include <QNetworkProxy>


namespace System
{
#ifdef __unix__
static bool findLibraryPath( const char* libnm, FilePath& ret )
{
#ifdef __mac__
    //TODO
    return false;
#else
    OS::MachineCommand mc( "/sbin/ldconfig" );
    mc.addFlag( "p", OS::OldStyle ).addPipe()
      .addArg( "grep" ).addArg( libnm );

    if ( !File::exists(mc.program()) )
	return false;

    BufferString ldoutstr;
    if ( !mc.execute(ldoutstr) )
	return false;

    BufferStringSet cmdoutlines, cmdoutlines_x64;
    cmdoutlines.unCat( ldoutstr.buf() );
    for ( const auto cmdoutline : cmdoutlines )
    {
      const SeparString cmdoutsep( cmdoutline->buf(), ' ' );
      if ( cmdoutsep.size() < 4 || !cmdoutsep[1].contains("x86-64") )
	  continue;

      const char* syslibnm = cmdoutsep[0].buf();
      if ( *syslibnm == '\t' )
	  syslibnm++;

      if ( FixedString(syslibnm) != libnm )
	  continue;

      cmdoutlines_x64.add( cmdoutsep[3] );
    }

    if ( cmdoutlines_x64.isEmpty() )
	return false;

    ret.set( cmdoutlines_x64.first()->buf() );
    return ret.exists();
#endif
}
#endif

} // namespace System


namespace Network
{

static bool canUseSystemOpenSSL()
{
#ifdef __unix__
    const FilePath libfp( __OpenSSL_Crypto_LIBRARY__ );
    const char* libnm = libfp.fileName().buf();
    FilePath ret;
    if ( !System::findLibraryPath(libnm,ret) )
	return false;
# ifdef __mac__
    return ret.exists();
# else
    OS::MachineCommand mc( "strings" );
    mc.addArg( ret.fullPath() ).addPipe()
      .addArg( "grep" )
      .addKeyedArg( "m", 1, OS::OldStyle )
      .addArg( "EVP_PKEY_param_check" );

    BufferString cmdoutstr;
    return mc.execute( cmdoutstr ) && !cmdoutstr.isEmpty();
# endif
#else
    return false;
#endif
}


static void loadOpenSSL()
{
    mIfNotFirstTime(return);
#ifdef __OpenSSL_Crypto_LIBRARY__
    if ( canUseSystemOpenSSL() )
	return;

    //Load first crypto, then ssl
#ifdef __mac__
    const BufferString ssldir( "../Resources/OpenSSL" );
#else
    const BufferString ssldir( "OpenSSL" );
#endif
    mDefineStaticLocalObject(PtrMan<RuntimeLibLoader>,cryptosha,
	    = new RuntimeLibLoader(__OpenSSL_Crypto_LIBRARY__,ssldir) );
# ifdef __OpenSSL_SSL_LIBRARY__
    mDefineStaticLocalObject(PtrMan<RuntimeLibLoader>,sslsha,
	    = cryptosha && cryptosha->isOK()
	    ? new RuntimeLibLoader(__OpenSSL_SSL_LIBRARY__,ssldir)
	    : nullptr );
# endif
#endif
}

} // namespace Network



bool Network::exists( const char* url )
{
    od_int64 dum; uiString msg;
    return getRemoteFileSize( url, dum, msg );
}

od_int64 Network::getFileSize( const char* url )
{
    od_int64 ret; uiString msg;
    return getRemoteFileSize( url, ret, msg ) ? ret : 0;
}

bool Network::getContent( const char* url, BufferString& bs )
{
    uiString msg; DataBuffer dbuf(0,1);
    if ( !downloadToBuffer(url,&dbuf,msg) || !dbuf.fitsInString() )
	return false;
    bs = dbuf.getString();
    return true;
}


bool Network::downloadFile( const char* url, const char* path,
			    uiString& errmsg, TaskRunner* taskr )
{
    BufferStringSet urls; urls.add( url );
    BufferStringSet outputpath; outputpath.add( path );
    return downloadFiles( urls, outputpath, errmsg, taskr );
}


bool Network::downloadFiles( BufferStringSet& urls, const char* path,
			     uiString& errmsg, TaskRunner* taskr )
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

    return downloadFiles( urls, outputpaths, errmsg, taskr );
}


bool Network::downloadFiles( BufferStringSet& urls,BufferStringSet& outputpaths,
			     uiString& errmsg, TaskRunner* taskr )
{
    if ( urls.size() != outputpaths.size() )
	return false;

    FileDownloader dl( urls, outputpaths );
    const bool res = taskr ? taskr->execute( dl ) : dl.execute();
    if ( !res ) errmsg = dl.uiMessage();
    return res;
}


bool Network::downloadToBuffer( const char* url, DataBuffer* databuffer,
				uiString& errmsg, TaskRunner* taskr )
{
    databuffer->reSize( 0, false );
    databuffer->reByte( 1, false );
    FileDownloader dl( url, databuffer );
    const bool res = taskr ? taskr->execute( dl ) : dl.execute();
    if ( !res ) errmsg = dl.uiMessage();
    return res;
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
    , qeventloop_(0)
    , odnr_(0)
    , initneeded_(true)
    , msg_(uiString::empty())
    , nrdone_(0)
    , nrfilesdownloaded_(0)
    , osd_(new od_ostream())
    , databuffer_(0)
    , saveaspaths_( outputpaths )
    , urls_( urls )
{
    Network::loadOpenSSL(); //Keep at the first line
    totalnr_ = getDownloadSize();
}


FileDownloader::FileDownloader( const char* url, DataBuffer* db )
    : SequentialTask("Downloading file")
    , qeventloop_(0)
    , odnr_(0)
    , initneeded_(true)
    , msg_(uiString::empty())
    , nrdone_(0)
    , nrfilesdownloaded_(0)
    , osd_(0)
    , databuffer_(db)
{
    Network::loadOpenSSL(); //Keep at the first line
    urls_.add(url);
    totalnr_ = getDownloadSize();
}


FileDownloader::FileDownloader( const char* url )
    : SequentialTask("Downloading file")
    , qeventloop_(0)
    , odnr_(0)
    , initneeded_(true)
    , msg_(uiString::empty())
    , nrdone_(0)
    , nrfilesdownloaded_(0)
    , totalnr_(0)
    , osd_(new od_ostream())
    , databuffer_(0)
{
    Network::loadOpenSSL(); //Keep at the first line
    urls_.add(url);
}


FileDownloader::~FileDownloader()
{
    delete qeventloop_;
    delete odnr_;
    if ( osd_ )
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

	delete odnr_;
	if ( !qeventloop_ )
	    qeventloop_ = new QEventLoop();

	odnr_ = new ODNetworkReply( ODNA().get(QNetworkRequest(QUrl(urls_.get
				   (nrfilesdownloaded_).buf()))), qeventloop_ );
    }

    qeventloop_->exec();
    if ( odnr_->qNetworkReply()->error() )
	return errorOccured();

    if ( odnr_->qNetworkReply()->bytesAvailable() && !writeData() )
	return ErrorOccurred();

    if ( odnr_->qNetworkReply()->isFinished() )
    {
	initneeded_ = true;
	nrfilesdownloaded_++;
	if ( osd_ && osd_->isOK() )
	    osd_->close();
    }

    return MoreToDo();
}


od_int64 FileDownloader::getDownloadSize()
{
    od_int64 totalbytes = 0;
    for ( int idx=0; idx<urls_.size(); idx++ )
    {
	QNetworkReply* qnr = ODNA().head( QNetworkRequest
						(QUrl(urls_.get(idx).buf())) );
	delete odnr_;
	QEventLoop qeventloop;
	odnr_ = new ODNetworkReply( qnr, &qeventloop );
	qeventloop.exec();
	if ( odnr_->qNetworkReply()->error() )
	    return errorOccured();

	od_int64 filesize = odnr_->qNetworkReply()->header
			    ( QNetworkRequest::ContentLengthHeader ).toInt();
	totalbytes += filesize;
	while( !odnr_->qNetworkReply()->isFinished() )
	    qeventloop.exec();
    }

    delete odnr_; odnr_ = 0;
    return totalbytes;
}


bool FileDownloader::writeData()
{
    od_int64 bytes = odnr_->qNetworkReply()->bytesAvailable();
    PtrMan<char> buffer = new char[bytes];
    odnr_->qNetworkReply()->read( buffer, bytes );
    nrdone_ += bytes;
    if ( databuffer_ )
	return writeDataToBuffer( buffer, bytes );
    else
	return writeDataToFile( buffer, bytes );
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
	    msg_ = tr("Didn't have permission to write to: %1")
		  .arg(fp.fullPath());
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
    msg_ = tr("Oops! Something went wrong.\n");
    if (odnr_)
	msg_ = tr("Details: %1")
	     .arg(qPrintable(odnr_->qNetworkReply()->errorString()));
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


#define mBoundary "---------------------------193971182219750"


void addPars( BufferString& data, const IOPar& postvars )
{
    for ( int idx=0; idx<postvars.size(); idx++ )
    {
	data.add( "--" ).add( mBoundary );
	data.add( "\r\nContent-Disposition: form-data; name=\"");
	data.add( postvars.getKey(idx).str() ).add( "\"\r\n\r\n" );
	data.add( postvars.getValue(idx).str() ).add( "\r\n" );
    }

    data.add( "--" ).add( mBoundary );
    return;
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

    BufferString bsdata;
    bsdata.add( "--" ).add( mBoundary );
    bsdata.add( "\r\nContent-Disposition: form-data; name=\"").add( ftype );
    bsdata.add( "\"; filename=\"").add( remotefname ).add( "\"\r\n" );
    bsdata.add( "Content-Type: application/octet-stream\r\n\r\n" );

    od_istream isd( localfname );
    int size = File::getFileSize( localfname );

    PtrMan<DataBuffer> databuffer = new DataBuffer( size+bsdata.size(),1 );
    OD::memCopy( databuffer->data(), bsdata.buf(), bsdata.size() );
    isd.getBin( databuffer->data() + bsdata.size(), size );
    isd.close();
    bsdata = ( "\r\n--" );
    bsdata.add( mBoundary );
    bsdata.add( "\r\nContent-Disposition: form-data; name=\"upload\"\r\n\r\n" );
    bsdata.add( "Uploader\r\n" );

    addPars( bsdata, postvars);
    const int prev_size = databuffer->size();
    databuffer->reSize( prev_size + bsdata.size() );
    OD::memCopy( databuffer->data()+prev_size, bsdata.buf(), bsdata.size() );
    BufferString header( "multipart/form-data; boundary=", mBoundary );
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
    BufferString header( "multipart/form-data; boundary=", mBoundary );
    DataUploader up( url, db, header );
    const bool res = taskr ? taskr->execute( up ) : up.execute();
    if ( !res )
	errmsg = up.uiMessage();
    else if ( retmsg )
	*retmsg = up.uiMessage();
    return res;
}


DataUploader::DataUploader( const char* url, const DataBuffer& data,
			    BufferString& header )
    : data_(new QByteArray(mCast(const char*,data.data()),data.size()))
    , qeventloop_(new QEventLoop())
    , nrdone_(0)
    , totalnr_(0)
    , odnr_(0)
    , msg_(uiString::empty())
    , url_(url)
    , header_(header)
    , init_(true)
{
    Network::loadOpenSSL(); //Keep at the first line
}


DataUploader::~DataUploader()
{
    delete qeventloop_;
    delete data_;
    delete odnr_;
}


int DataUploader::nextStep()
{
    if ( init_ )
    {
	QNetworkRequest qrequest( QUrl(url_.buf()) );
	qrequest.setHeader( QNetworkRequest::ContentTypeHeader, header_.buf() );
	qrequest.setHeader( QNetworkRequest::ContentLengthHeader,data_->size());
	delete odnr_;
	odnr_ = new ODNetworkReply( ODNA().post(qrequest,*data_), qeventloop_ );
	init_ = false;
    }

    qeventloop_->exec();

    if ( odnr_->qNetworkReply()->error() )
	return errorOccured();
    else if ( odnr_->qNetworkReply()->isFinished() )
    {
	if ( odnr_->qNetworkReply() )
	{
	    odnr_->qNetworkReply()->waitForReadyRead( 500 );
	    msg_.setFrom( QString( odnr_->qNetworkReply()->readAll() ) );
	}
	return Finished();
    }
    else if ( odnr_->qNetworkReply()->isRunning() )
    {
	nrdone_ = odnr_->getBytesUploaded();
	totalnr_ = odnr_->getTotalBytesToUpload();
    }

    return MoreToDo();
}


int DataUploader::errorOccured()
{
    msg_ = tr("Oops! Something went wrong.\n");
    if (odnr_)
	msg_ = tr("Details: %1")
	     .arg(qPrintable(odnr_->qNetworkReply()->errorString()));
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
    if ( !auth )
    {
	setHttpProxyFromIOPar( setts );
	return;
    }

    IOPar parcp( setts );
    BufferString password;
    bool iscrypt = false;
    parcp.get( Network::sKeyProxyPassword(), password );
    if ( password.isEmpty() )
    {
	getProxySettingsFromUser();
	return;
    }
    else if ( !setts.getYN(Network::sKeyCryptProxyPassword(),iscrypt) )
    {
	uiString str = toUiString( password );
	str.getHexEncoded( password );
	setts.set( Network::sKeyProxyPassword(), password );
	setts.setYN( Network::sKeyCryptProxyPassword(), true );
	setts.write();
    }

    setHttpProxyFromIOPar( parcp );
}


void Network::setHttpProxyFromIOPar( const IOPar& pars )
{
    bool useproxy = false;
    pars.getYN( Network::sKeyUseProxy(), useproxy );
    if ( !useproxy )
    {
	QNetworkProxy proxy;
	proxy.setType( QNetworkProxy::NoProxy );
	QNetworkProxy::setApplicationProxy( proxy );
	return;
    }

    BufferString host;
    pars.get( Network::sKeyProxyHost(), host );
    if ( host.isEmpty() )
	return;

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

	Network::setHttpProxy( host, port, auth, username, password );
    }
    else
	Network::setHttpProxy( host, port );
}


bool Network::getProxySettingsFromUser()
{
    NetworkUserQuery* inst = NetworkUserQuery::getNetworkUserQuery();
    if ( !inst ) return false;

    return inst->setFromUser();
}

void Network::setHttpProxy( const char* hostname, int port, bool auth,
			    const char* username, const char* password )
{
    QNetworkProxy proxy;
    proxy.setType( QNetworkProxy::HttpProxy );
    proxy.setHostName( hostname );
    proxy.setPort( port );
    if ( auth )
    {
	proxy.setUser( username );
	proxy.setPassword( password );
    }

    QNetworkProxy::setApplicationProxy( proxy );
}


NetworkUserQuery* NetworkUserQuery::inst_ = 0;

void NetworkUserQuery::setNetworkUserQuery( NetworkUserQuery* newinst )
{
    inst_ = newinst;
}

NetworkUserQuery* NetworkUserQuery::getNetworkUserQuery()
{
    return inst_;
}


QNetworkAccessManager& ODNA()
{
    mDefineStaticLocalObject(PerThreadObjectRepository<QNetworkAccessManager>*,
			     odnarepos, = 0);
    if ( !odnarepos )
    {
	odnarepos = new PerThreadObjectRepository<QNetworkAccessManager>;
	Network::setHttpProxyFromSettings();
    }

    return odnarepos->getObject();
}
