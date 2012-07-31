/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2011
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: oddlsite.cc,v 1.24 2012-07-31 04:44:48 cvsranojay Exp $";

#include "oddlsite.h"
#include "odhttp.h"
#include "httptask.h"
#include "databuf.h"
#include "file.h"
#include "filepath.h"
#include "strmoper.h"
#include "strmprov.h"
#include "settings.h"
#include "executor.h"
#include "thread.h"
#include "timefun.h"

static const char* sKeyTimeOut = "Download.Timout";

ODDLSite::ODDLSite( const char* h, float t )
    : timeout_(t)
    , databuf_(0)
    , islocal_(h && matchString("DIR=",h))
    , isfailed_(false)
    , issecure_(false)
{
    odhttp_ = islocal_ ? 0 : new ODHttp;
    if ( !h ) h = "opendtect.org";
    int stroffs = 0;
    if ( islocal_ )
	stroffs = 4;
    else if ( matchString("http://",h) )
	stroffs = 7;
    else if ( matchString("https://",h) )
	{ stroffs = 8; issecure_ = true; }
    host_ = h + stroffs;

    // TODO handle timeout
    if ( timeout_ <= 0 )
    {
	Settings::common().get( sKeyTimeOut, timeout_ );
	if ( timeout_ <= 0 )
	    timeout_ = 5; // internet is quick these days
    }

    if ( host_.isEmpty() )
	host_ = "opendtect.org";
    if ( !islocal_ )
	odhttp_->requestFinished.notify( mCB(this,ODDLSite,reqFinish) );

    reConnect();
}


ODDLSite::~ODDLSite()
{
    delete databuf_;
    delete odhttp_;
}


void ODDLSite::setTimeOut( float t, bool sett )
{
    timeout_ = t;
    if ( sett )
    {
	Settings::common().set( sKeyTimeOut, timeout_ );
	Settings::common().write();
    }
}


bool ODDLSite::reConnect()
{
    if ( islocal_ )
	return !(isfailed_ = !File::isDirectory(host_));
    else if ( issecure_ )
	{ errmsg_ = "TODO secure access not implemented."; return false; }

    if ( odhttp_->state() == ODHttp::Unconnected )
    {
	if ( issecure_ )
	    odhttp_->setHttpsHost( host_ );
	else
	    odhttp_->setHost( host_ );
    }

    return true;
}


bool ODDLSite::getFile( const char* relfnm, const char* outfnm, TaskRunner* tr,
			    const char* nicename )
{
    delete databuf_; databuf_ = 0;

    if ( islocal_ )
	return getLocalFile( relfnm, outfnm );
  
    if ( outfnm )
    {
	odhttp_->setASynchronous( true );
	odhttp_->get( getFileName(relfnm), outfnm );
	HttpTask task( *odhttp_ );
	task.setName( nicename );
	if ( !(tr ? tr->execute( task ) : task.execute() ) )
	{
	    errmsg_ = task.message();
	    return false;
	}
    }
    else
    {
	odhttp_->setASynchronous( false );
	odhttp_->get( getFileName(relfnm), outfnm );
    }
    
    if ( odhttp_->isForcedAbort() )
    {
	reConnect();
	odhttp_->resetForceAbort();
	errmsg_ = ". Operation aborted by the user";
	File::remove( outfnm );
	return false;
    }

    const od_int64 totnr = odhttp_->totalNr();
    if ( totnr <= 0 )
	return false;

    const od_int64 nrbytes = odhttp_->bytesAvailable();
    databuf_ = new DataBuffer( nrbytes, 1, true );
    const char* buffer = odhttp_->readCharBuffer();
    const char* hdptr = strstr( buffer, "<HEAD>" );
    const char* errptr = strstr( buffer, "error" );
    if ( hdptr || errptr )
    {
	errmsg_ = relfnm;
	errmsg_ += " file not found on the server. "
		   "Please try with the other sites from the drop down list\nor "
		   "change the proxy settings if necessary";
	return false;
    }
    memcpy( databuf_->data(), buffer, nrbytes );

    if ( outfnm && *outfnm )
    {
	if ( File::getFileSize(outfnm) < 1024
	    || File::getFileSize(outfnm) < totnr )
	{
	    errmsg_ = ". Download failed, it seems, you might be having problem"
		      " with your internet connection ";
	    return false;
	}
    }
    
    return true;
}


bool ODDLSite::getLocalFile( const char* relfnm, const char* outfnm )
{
    const BufferString inpfnm( getFileName(relfnm) );

    if ( outfnm && *outfnm )
	return File::copy( inpfnm, outfnm );

    StreamData sd( StreamProvider(inpfnm).makeIStream() );
    if ( !sd.usable() )
	{ errmsg_ = "Cannot open "; errmsg_ += inpfnm; return false; }
    BufferString bs;
    const bool isok = StrmOper::readFile( *sd.istrm, bs );
    sd.close();
    if ( isok )
    {
	databuf_ = new DataBuffer( bs.size(), 1 );
	memcpy( (char*)databuf_->data(), bs.buf(), databuf_->size() );
    }
    return isok;
}


DataBuffer* ODDLSite::obtainResultBuf()
{
    DataBuffer* ret = databuf_;
    databuf_ = 0;
    return ret;
}


class ODDLSiteMultiFileGetter : public Executor
{
public:

ODDLSiteMultiFileGetter( ODDLSite& dls,
		const BufferStringSet& fnms, const char* outputdir )
    : Executor("File download")
    , dlsite_(dls)
    , fnms_(fnms)
    , curidx_(-1)
    , outdir_(outputdir)
    , httptask_(0)
{
    if ( !dlsite_.isOK() )
	{ msg_ = dlsite_.errMsg(); return; }
    if ( !File::isDirectory(outputdir) )
	{ msg_ = "Output directory does not exist"; return; }

    curidx_ = 0;
    if ( fnms_.isEmpty() )
	msg_ = "No files to get";
    else
	msg_.add( "Getting '" ).add( fnms_.get(curidx_) );

    if ( !dlsite_.islocal_ )
	httptask_ = new HttpTask( *dlsite_.odhttp_ );
}

~ODDLSiteMultiFileGetter()
{
    delete httptask_;
}

const char* message() const	{ return msg_; }
const char* nrDoneText() const	{ return "Files downloaded"; }
od_int64 nrDone() const		{ return curidx_ + 1; }
od_int64 totalNr() const	{ return fnms_.size(); }

    int				nextStep();

    ODDLSite&			dlsite_;
    HttpTask*			httptask_;
    const BufferStringSet&	fnms_;
    int				curidx_;
    BufferString		outdir_;
    BufferString		msg_;

};


int ODDLSiteMultiFileGetter::nextStep()
{
    if ( curidx_ < 0 )
	return ErrorOccurred();
    else if ( curidx_ >= fnms_.size() )
	return Finished();

    const BufferString inpfnm( dlsite_.getFileName(fnms_.get(curidx_)) );
    BufferString outfnm( FilePath(outdir_).add(inpfnm).fullPath() );
    bool isok = true;
    if ( dlsite_.islocal_ )
	isok = dlsite_.getFile( inpfnm, outfnm );
    else
    {
	dlsite_.odhttp_->get( inpfnm, outfnm );
	isok = dlsite_.odhttp_->state() > ODHttp::Connecting;
    }

    if ( isok )
	curidx_++;
    else
    {
	msg_ = dlsite_.errMsg();
	curidx_ = -1;
    }

    return MoreToDo();
}


bool ODDLSite::getFiles( const BufferStringSet& fnms, const char* outputdir,
			 TaskRunner& tr )
{
    errmsg_.setEmpty();

    ODDLSiteMultiFileGetter mfg( *this, fnms, outputdir );
    if ( !tr.execute(mfg) )
    {
	errmsg_ = mfg.curidx_ < 0 ? mfg.msg_.buf() : "";
	return false;
    }

    const bool res = mfg.httptask_ ? tr.execute( *mfg.httptask_ ) : true;
    if ( !res && !mfg.httptask_->userStop() )
	errmsg_ = mfg.httptask_->message();

    return res;
}


void ODDLSite::reqFinish( CallBacker* )
{
    //TODO implement
}


BufferString ODDLSite::getFileName( const char* relfnm ) const
{
    if ( islocal_ )
	return FilePath( host_, subdir_, relfnm ).fullPath();

    BufferString ret( "/" );
    if ( subdir_.isEmpty() )
	ret.add( relfnm );
    else
	ret.add( FilePath( subdir_, relfnm ).fullPath(FilePath::Unix) );
    return ret;
}


BufferString ODDLSite::fullURL( const char* relfnm ) const
{
    if ( islocal_ )
	return getFileName( relfnm );

    BufferString ret( "http://" );
    ret.add( host_ ).add( getFileName(relfnm) );
    return ret;
}
