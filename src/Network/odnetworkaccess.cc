/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odnetworkaccess.h"

#include "databuf.h"
#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "od_iostream.h"
#include "oddirs.h"
#include "opensslaccess.h"
#include "perthreadrepos.h"
#include "separstr.h"
#include "settings.h"
#include "uistrings.h"

# include <QByteArray>
# include <QNetworkProxy>


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
    if ( !downloadToBuffer(url,dbuf,msg) || !dbuf.fitsInString() )
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


bool Network::downloadToBuffer( const char* url, DataBuffer& databuffer,
				uiString& errmsg, TaskRunner* taskr )
{
    databuffer.reSize( 0, false );
    databuffer.reByte( 1, false );
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
    , osd_(new od_ostream())
    , saveaspaths_( outputpaths )
    , urls_( urls )
{
    OD::OpenSSLAccess::loadOpenSSL(); //Keep at the first line
    totalnr_ = getDownloadSize();
}


FileDownloader::FileDownloader( const char* url, DataBuffer& db )
    : SequentialTask("Downloading file")
    , databuffer_(&db)
{
    OD::OpenSSLAccess::loadOpenSSL(); //Keep at the first line
    urls_.add(url);
    totalnr_ = getDownloadSize();
}


FileDownloader::FileDownloader( const char* url )
    : SequentialTask("Downloading file")
    , osd_(new od_ostream())
{
    OD::OpenSSLAccess::loadOpenSSL(); //Keep at the first line
    urls_.add(url);
}


FileDownloader::~FileDownloader()
{
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
	odnr_ = Network::HttpRequestManager::instance().get( url );
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
    od_int64 totalbytes = 0;
    for ( int idx=0; idx<urls_.size(); idx++ )
    {
	const char* url = urls_.get(idx).buf();
	odnr_ = Network::HttpRequestManager::instance().head( url );
	odnr_->waitForFinish();

	if ( odnr_->isError() )
	    return errorOccured();


	od_int64 filesize = odnr_->getContentLengthHeader();
	totalbytes += filesize;
    }

    odnr_ = nullptr;
    return totalbytes;
}


bool FileDownloader::writeData()
{
    od_int64 bytes = odnr_->downloadBytesAvailable();
    if ( !bytes )
	return true;

    mAllocLargeVarLenArr( char, buffer, bytes );
    bytes = odnr_->read( buffer, bytes );
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
    if ( odnr_ )
	msg_ = odnr_->errMsg();

    if ( msg_.isEmpty() )
	msg_ = tr( "Oops! Something went wrong with the connection" );

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


DataUploader::DataUploader( const char* url, const DataBuffer& data,
			    BufferString& header )
    : data_( data )
    , url_(url)
    , header_(header)
{
    OD::OpenSSLAccess::loadOpenSSL(); //Keep at the first line
}


DataUploader::~DataUploader()
{
}


int DataUploader::nextStep()
{
    if ( init_ )
    {
	RefMan<Network::HttpRequest> req = new Network::HttpRequest( url_,
					       Network::HttpRequest::Post );
	req->contentType( header_ );
	req->payloadData( data_ );

	odnr_ = Network::HttpRequestManager::instance().request(req);
	init_ = false;
    }

    if ( odnr_->isError() )
	return errorOccured();
    else if ( odnr_->isFinished() )
    {
	odnr_->waitForDownloadData( 500 );
	msg_ = toUiString( odnr_->readAll() );

	return Finished();
    }
    else if ( odnr_->isRunning() )
    {
	nrdone_ = odnr_->getBytesUploaded();
	totalnr_ = odnr_->getTotalBytesToUpload();
    }

    return MoreToDo();
}


int DataUploader::errorOccured()
{
    if ( odnr_ )
	msg_ = odnr_->errMsg();

    if ( msg_.isEmpty() )
	msg_ = tr( "Oops! Something went wrong with the connection" );

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
    proxy.setType( QNetworkProxy::DefaultProxy );
    proxy.setHostName( hostname );
    proxy.setPort( port );
    if ( auth )
    {
	proxy.setUser( username );
	proxy.setPassword( password );
    }

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
