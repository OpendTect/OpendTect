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
#include "od_istream.h"
#include "od_ostream.h"
#include "oddirs.h"
#include "opensslaccess.h"
#include "separstr.h"
#include "settings.h"
#include "task.h"
#include "thread.h"
#include "typeset.h"

# include <QByteArray>
# include <QNetworkProxy>


static const int cMaxTransferRetries = 5;
static const int cMaxSizeCheckRetries = 3;
static const double cSizeCheckRetryDelaySec = 0.25;


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


class FileDownloader : public SequentialTask
{ mODTextTranslationClass(FileDownloader);
public:
			FileDownloader(const char* url);
			FileDownloader(const char* url,DataBuffer&);
			FileDownloader(const BufferStringSet& urls,
				       const BufferStringSet& outputpaths);
			~FileDownloader();

    od_int64		getDownloadSize();

    uiString		uiMessage() const override;
    uiString		uiNrDoneText() const override;
    uiRetVal		allMessages() const;
    bool		hasFails() const;
    void		setContinueOnFail( bool yn )  { continueonfail_ = yn; }

private:
    od_int64		nrDone() const override;
    od_int64		totalNr() const override;
    double		progressFactor() const override;
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
};


//!>Provides file or data upload facility
class DataUploader : public SequentialTask
{ mODTextTranslationClass(DataUploader);
public:
			DataUploader(const char* url,const DataBuffer& data,
				     BufferString& header);
			~DataUploader();

    uiString		uiMessage() const override;
    uiString		uiNrDoneText() const override;

private:
    od_int64		nrDone() const override;
    od_int64		totalNr() const override;
    double		progressFactor() const override;

    bool		doPrepare(od_ostream* =nullptr) override;
    int			nextStep() override;
    int			errorOccured();
    bool		doFinish(bool,od_ostream* =nullptr) override;

    void		readTimeOutFromSettings();
    bool		startUpload();

    BufferString	url_;
    BufferString	header_;

    const DataBuffer&			data_;
    RefMan<Network::HttpRequestProcess> odnr_;

    bool		initneeded_ = true;
    bool		waitingforretry_ = false;
    int			retrycount_ = 0;
    double		maxretrydelaysec_ =
				Network::sKeyTimeOutMs() / 1000.;
    double		retrywaitremaining_ = 0.;
    od_int64		nrdone_ = 0;
    od_int64		totalnr_ = 1;
    uiString		msg_;
    uiRetVal		retryattemptmsgs_;
};


bool Network::exists( const char* url )
{
    od_int64 dum; uiString msg;
    return getRemoteFileSize( url, dum, msg );
}


od_int64 Network::getFileSize( const char* url, uiString* errmsg )
{
    od_int64 ret;
    uiString msg;
    uiString& theerrormsg = errmsg ? *errmsg : msg;
    return getRemoteFileSize( url, ret, theerrormsg ) ? ret : 0;
}


bool Network::getContent( const char* url, BufferString& str, uiString* errmsg,
			  TaskRunner* taskrun )
{
    DataBuffer databuffer( 0, 1 );
    FileDownloader dl( url, databuffer );
    const bool res = TaskRunner::execute( taskrun, dl );
    if ( res )
	str = databuffer.getString();
    else if ( errmsg )
	*errmsg = dl.allMessages().messages().cat();

    return res;
}


bool Network::putContent( const char* buf, int sz, const char* url,
			  uiString* errmsg, TaskRunner* taskrun )
{
    if ( sz < 0 )
	return false;

    DataBuffer databuffer( sz, 1 );
    if ( sz > 0 )
	OD::memCopy( (char*)databuffer.data(), buf, sz );

    BufferString header;
    DataUploader ul( url, databuffer, header );
    const bool res = TaskRunner::execute( taskrun, ul );
    if ( !res && errmsg )
	*errmsg = ul.uiMessage();

    return res;
}


uiRetVal Network::downloadFile( const char* url, const char* path,
				TaskRunner* taskr )
{
    BufferStringSet urls; urls.add( url );
    BufferStringSet outputpath; outputpath.add( path );
    return downloadFiles( urls, outputpath, taskr );
}


uiRetVal Network::downloadFiles( BufferStringSet& urls, const char* path,
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

    return downloadFiles( urls, outputpaths, taskr, canfail );
}


uiRetVal Network::downloadFiles( BufferStringSet& urls,
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

    FileDownloader dl( urls, outputpaths );
    dl.setContinueOnFail( canfail );
    const bool res = TaskRunner::execute( taskr, dl ) &&
					dl.allMessages().isOK();

    return res ? uiRetVal::OK() : dl.allMessages();
}


uiRetVal Network::downloadToBuffer( const char* url, DataBuffer& databuf,
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
    , saveaspaths_(outputpaths)
    , urls_(urls)
{
    OD::OpenSSLAccess::loadOpenSSL(); //Keep at the first line
    readTimeOutFromSettings();
    totalnr_ = getDownloadSize();
}


FileDownloader::FileDownloader( const char* url, DataBuffer& db )
    : SequentialTask("Downloading file")
    , databuffer_(&db)
{
    OD::OpenSSLAccess::loadOpenSSL(); //Keep at the first line
    readTimeOutFromSettings();
    urls_.add( url );
    totalnr_ = getDownloadSize();
}


FileDownloader::FileDownloader( const char* url )
    : SequentialTask("Downloading file")
    , osd_(new od_ostream())
{
    OD::OpenSSLAccess::loadOpenSSL(); //Keep at the first line
    readTimeOutFromSettings();
    urls_.add(url);
}


FileDownloader::~FileDownloader()
{
    delete osd_;
}


void FileDownloader::readTimeOutFromSettings()
{
    int timeoutms = Network::sKeyTimeOutMs();
    Settings::common().get( Network::cKeyDLTimeOut(), timeoutms );
    if ( timeoutms <= 0 )
	timeoutms = Network::sKeyTimeOutMs();

    maxretrydelaysec_ = timeoutms / 1000.;
}


uiRetVal FileDownloader::allMessages() const
{
    if ( uirv_.isOK() )
	return uiRetVal::OK();

    uiRetVal uirv( msg_ );
    uirv.add( uirv_ );
    return uirv;
}


bool FileDownloader::doPrepare( od_ostream* strm )
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


int FileDownloader::nextStep()
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


bool FileDownloader::doFinish( bool success, od_ostream* strm )
{
    odnr_ = nullptr;
    return SequentialTask::doFinish( success, strm );
}


od_int64 FileDownloader::getDownloadSize()
{
    od_int64 totalbytes = 0;
    filesizes_.setEmpty();
    bool anyfailed = false;
    for ( int idx=0; idx<urls_.size(); idx++ )
    {
	const char* url = urls_.get( idx ).buf();
	msg_ = tr( "Determining size of %1" ).arg( url );

	od_int64 filesize = -1;
	uiRetVal headerr;
	for ( int attempt=1; attempt<=cMaxSizeCheckRetries; attempt++ )
	{
	    odnr_ = Network::HttpRequestManager::instance().head( url );
	    odnr_->waitForFinish();

	    if ( !odnr_->isError() )
	    {
		filesize = odnr_->getContentLengthHeader();
		odnr_ = nullptr;
		break;
	    }

	    headerr = odnr_->errMsgs();
	    odnr_ = nullptr;

	    if ( attempt < cMaxSizeCheckRetries )
	    {
		msg_ = tr( "Retrying size check (%1/%2): %3" )
			.arg( attempt ).arg( cMaxSizeCheckRetries ).arg( url );
		Threads::sleep( cSizeCheckRetryDelaySec );
	    }
	}

	filesizes_ += filesize;
	if ( filesize < 0 )
	{
	    anyfailed = true;
	    uirv_.add( tr("Cannot determine size of %1").arg(url) );
	    if ( !headerr.isOK() )
		uirv_.add( headerr );
	}
	else if ( !mIsUdf(filesize) )
	    totalbytes += filesize;
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


bool FileDownloader::startDownload()
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

    if ( userange )
    {
	RefMan<Network::HttpRequest> req =
	    new Network::HttpRequest( url, Network::HttpRequest::Get );
	BufferString hdrstr( "bytes=", bytesondisk_, "-" );
	req->setRawHeader( "Range", hdrstr.str() );
	odnr_ = Network::HttpRequestManager::instance().request( req.ptr() );
	rangerequested_ = true;
    }
    else
    {
	odnr_ = Network::HttpRequestManager::instance().get( url );
	rangerequested_ = false;
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


bool FileDownloader::checkRangeResponse()
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


void FileDownloader::handleRangeNotSatisfiable()
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


void FileDownloader::handleFullResponseAfterRange()
{
    resuming_ = false;
    bytesondisk_ = 0;
    rangerequested_ = false;

    if ( osd_ && osd_->isOK() )
	osd_->close();

    nrdone_ = nrdoneaturlstart_;
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


bool FileDownloader::writeDataToFile( const char* buffer, int size )
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


bool FileDownloader::writeDataToBuffer( const char* buffer, int size )
{
    if ( !databuffer_ )
	return false;

    int buffersize = databuffer_->size();
    databuffer_->reSize( nrdone_ );
    OD::memCopy( databuffer_->data()+buffersize, buffer, size );
    return true;
}


int FileDownloader::errorOccured()
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


uiString FileDownloader::uiMessage() const
{ return msg_; }


uiString FileDownloader::uiNrDoneText() const
{ return tr("KBytes downloaded"); }


od_int64 FileDownloader::nrDone() const
{ return nrdone_; }


od_int64 FileDownloader::totalNr() const
{ return totalnr_; }


double FileDownloader::progressFactor() const
{ return 1./mDef1KB; }



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
    : SequentialTask("Uploading data")
    , data_( data )
    , url_(url)
    , header_(header)
{
    OD::OpenSSLAccess::loadOpenSSL(); //Keep at the first line
    readTimeOutFromSettings();
}


DataUploader::~DataUploader()
{
}


void DataUploader::readTimeOutFromSettings()
{
    int timeoutms = Network::sKeyTimeOutMs();
    Settings::common().get( Network::cKeyULTimeOut(), timeoutms );
    if ( timeoutms <= 0 )
	timeoutms = Network::sKeyTimeOutMs();

    maxretrydelaysec_ = timeoutms / 1000.;
}


bool DataUploader::doPrepare( od_ostream* strm )
{
    totalnr_ = data_.size() > 0 ? data_.size() : 1;
    return SequentialTask::doPrepare( strm );
}


bool DataUploader::startUpload()
{
    nrdone_ = 0;
    RefMan<Network::HttpRequest> req = new Network::HttpRequest( url_,
					   Network::HttpRequest::Post );
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


bool DataUploader::doFinish( bool success, od_ostream* strm )
{
    odnr_ = nullptr;
    return SequentialTask::doFinish( success, strm );
}


uiString DataUploader::uiMessage() const
{ return msg_; }


uiString DataUploader::uiNrDoneText() const
{ return tr("KBytes uploaded"); }


od_int64 DataUploader::nrDone() const
{ return nrdone_; }


od_int64 DataUploader::totalNr() const
{ return totalnr_; }


double DataUploader::progressFactor() const
{ return 1./mDef1KB; }


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
