/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:	    Salil Agarwal
Date:	    October 2012
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "odnetworkaccess.h"

#include "databuf.h"
#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "odnetworkreply.h"
#include "od_ostream.h"
#include "od_istream.h"
#include "qnetworkaccessconn.h"
#include "settings.h"
#include "separstr.h"

#include <QByteArray>
#include <QEventLoop>
#include <QNetworkProxy>


bool Network::downloadFile( const char* url, const char* path,
			    uiString& errmsg, TaskRunner* tr )
{
    BufferStringSet urls; urls.add( url );
    BufferStringSet outputpath; outputpath.add( path );
    return Network::downloadFiles( urls, outputpath, errmsg, tr );
}


bool Network::downloadFiles( BufferStringSet& urls, const char* path,
			     uiString& errmsg, TaskRunner* tr )
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

    return Network::downloadFiles( urls, outputpaths, errmsg, tr );
}


bool Network::downloadFiles( BufferStringSet& urls,BufferStringSet& outputpaths,
			     uiString& errmsg, TaskRunner* tr )
{
    if ( urls.size() != outputpaths.size() )
	return false;

    Network::setHttpProxyFromSettings();
    FileDownloader dl( urls, outputpaths );
    const bool res = tr ? tr->execute( dl ) : dl.execute();
    if ( !res ) errmsg = dl.uiMessage();
    return res;
}


bool Network::downloadToBuffer( const char* url, DataBuffer* databuffer,
				uiString& errmsg, TaskRunner* tr )
{
    databuffer->reSize( 0, false );
    databuffer->reByte( 1, false );
    FileDownloader dl( url, databuffer );
    const bool res = tr ? tr->execute( dl ) : dl.execute();
    if ( !res ) errmsg = dl.uiMessage();
    return res;
}


bool Network::getRemoteFileSize( const char* url, od_int64& size,
				 uiString& errmsg )
{
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
    return Network::getRemoteFileSize( url, pseudosize, msg );
}


FileDownloader::FileDownloader( const BufferStringSet& urls,
				const BufferStringSet& outputpaths )
    : qeventloop_(0)
    , odnr_(0)
    , initneeded_(true)
    , msg_(0)
    , nrdone_(0)
    , nrfilesdownloaded_(0)
    , osd_(new od_ostream())
    , databuffer_(0)
    , saveaspaths_( outputpaths )
    , urls_( urls )
{ totalnr_ = getDownloadSize(); }


FileDownloader::FileDownloader( const char* url, DataBuffer* db )
    : qeventloop_(0)
    , odnr_(0)
    , initneeded_(true)
    , msg_(0)
    , nrdone_(0)
    , nrfilesdownloaded_(0)
    , osd_(0)
    , databuffer_(db)
{
    urls_.add(url);
    totalnr_ = getDownloadSize();
}


FileDownloader::FileDownloader( const char* url )
    : qeventloop_(0)
    , odnr_(0)
    , initneeded_(true)
    , msg_(0)
    , nrdone_(0)
    , nrfilesdownloaded_(0)
    , totalnr_(0)
    , osd_(new od_ostream())
    , databuffer_(0)
{ urls_.add(url); }


FileDownloader::~FileDownloader()
{
    delete qeventloop_;
    delete odnr_;
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
	    msg_.add( " Didn't have permission to write to: " );
	    msg_.add( fp.fullPath() );
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
    msg_ = "Oops! Something went wrong.\n";
    if( odnr_ )
	msg_.add( "Details: " ).add( qPrintable(
				       odnr_->qNetworkReply()->errorString()) );

    return ErrorOccurred();
}


uiStringCopy FileDownloader::uiMessage() const
{ return msg_; }


od_int64 FileDownloader::nrDone() const
{return nrdone_/1024;}


uiStringCopy FileDownloader::uiNrDoneText() const
{return "KBytes downloaded";}


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
			  TaskRunner* taskrunner )
{
    if ( !File::isFile(localfname) )
    {
	errmsg = uiString(" %1\nFile not found").arg( localfname );
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
    Network::setHttpProxyFromSettings();
    DataUploader up( url, *databuffer, header );
    const bool res = taskrunner ? taskrunner->execute( up ) : up.execute();
    if ( !res ) errmsg = up.uiMessage();
    return res;
}


bool Network::uploadQuery( const char* url, const IOPar& querypars,
			   uiString& errmsg, TaskRunner* tr)
{
    BufferString data;
    addPars( data, querypars);
    DataBuffer db( data.size(), 1 );
    OD::memCopy( db.data(), data.buf(), data.size() );
    BufferString header( "multipart/form-data; boundary=", mBoundary );
    Network::setHttpProxyFromSettings();
    DataUploader up( url, db, header );
    const bool res = tr ? tr->execute( up ) : up.execute();
    if ( !res ) errmsg = up.uiMessage();
    return res;
}


DataUploader::DataUploader( const char* url, const DataBuffer& data,
			    BufferString& header )
    : data_(new QByteArray(mCast(const char*,data.data()),data.size()))
    , qeventloop_(new QEventLoop())
    , nrdone_(0)
    , totalnr_(0)
    , odnr_(0)
    , msg_(0)
    , url_(url)
    , header_(header)
    , init_(true)
{}


DataUploader::~DataUploader()
{
    delete qeventloop_;
    delete odnr_;
    delete data_;
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
	return Finished();
    else if ( odnr_->qNetworkReply()->isRunning() )
    {
	nrdone_ = odnr_->getBytesUploaded();
	totalnr_ = odnr_->getTotalBytesToUpload();
    }

    return MoreToDo();
}


int DataUploader::errorOccured()
{
    msg_ = "Oops! Something went wrong.\n";
    if( odnr_ )
	msg_.add( "Details: " ).add( qPrintable(
				       odnr_->qNetworkReply()->errorString()) );

    return ErrorOccurred();
}


uiStringCopy DataUploader::uiMessage() const
{ return msg_; }


od_int64 DataUploader::nrDone() const
{return nrdone_/1024;}


uiStringCopy DataUploader::uiNrDoneText() const
{return "KBytes uploaded";}


od_int64 DataUploader::totalNr() const
{ return totalnr_/1024; }


void Network::setHttpProxyFromSettings()
{
    Settings& setts = Settings::common();
    bool useproxy = false;
    setts.getYN( Network::sKeyUseProxy(), useproxy );
    if ( !useproxy ) return;

    BufferString host;
    setts.get( Network::sKeyProxyHost(), host );
    if ( host.isEmpty() )
	return;

    int port = 1;
    setts.get( Network::sKeyProxyPort(), port );

    bool auth = false;
    setts.getYN( Network::sKeyUseAuthentication(), auth );

    if ( auth )
    {
	BufferString username;
	setts.get( Network::sKeyProxyUserName(), username );

	BufferString password;
	setts.get( Network::sKeyProxyPassword(), password );
	Network::setHttpProxy( host, port, auth, username, password );
    }
    else
	Network::setHttpProxy( host, port );
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

    QNetworkProxy::setApplicationProxy(proxy);
}


QNetworkAccessManager& ODNA()
{
    mDefineStaticLocalObject( QNetworkAccessManager*, odna, = 0 );
    if ( !odna )
	odna = new QNetworkAccessManager();

    return *odna;
}

