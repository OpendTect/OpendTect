/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : June 2007
-*/

static const char* rcsID = "$Id: madio.cc,v 1.3 2007-12-20 16:18:54 cvsbert Exp $";

#include "madio.h"
#include "keystrs.h"
#include "filegen.h"
#include "filepath.h"
#include "strmprov.h"
#include "envvars.h"
#include "iopar.h"

const char* ODMad::FileSpec::sKeyDataPath = "Data Path";
const char* ODMad::sKeyMadagascar = "Madagascar";
const char* ODMad::sKeyMadSelKey = "909090";


ODMad::FileSpec::FileSpec( bool fr )
    : forread_(fr)
{
}


static BufferString getDataFnm( const char* fnm, const char* dir, bool forread )
{
    FilePath fp( fnm ); fp.setPath( dir );
    const BufferString ret( fp.fullPath() );
    if ( !forread )
    {
	if ( File_isWritable(dir) )
	    return ret;
    }
    else if ( !File_isEmpty(ret) )
	return ret;

    return BufferString();
}


static BufferString getDataPath( const char* dp, const char* fnm, bool forread )
{
    BufferString ret( dp );
    if ( dp ) return ret;

    ret = getDataFnm( fnm, ODMad::FileSpec::defDataPath(), forread );
    ret = FilePath( ret ).pathOnly();
    return ret;
}


void ODMad::FileSpec::set( const char* fnm, const char* dp )
{
    fname_ = fnm;
    datapath_ = getDataPath( dp, fnm, forread_ );
}


void ODMad::FileSpec::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Type, forread_ ? "Read" : "Write" );
    iop.set( sKey::FileName, fname_ );
    iop.set( sKeyDataPath, datapath_ );
}


bool ODMad::FileSpec::usePar( const IOPar& iop )
{
    const char* res = iop.find( sKey::Type );
    forread_ = !res || (*res != 'w' && *res != 'W');

    BufferString fnm( fname_ );
    iop.get( sKey::FileName, fnm );
    if ( fnm.isEmpty() )
    {
	errmsg_ = "No filename specified";
	return false;
    }

    BufferString dp( datapath_ );
    iop.get( sKeyDataPath, dp );
    set( fnm, dp );
    if ( datapath_.isEmpty() )
    {
	errmsg_ = "Cannot find a valid DATAPATH";
	return false;
    }

    return true;
}


const char* ODMad::FileSpec::defDataPath()
{
    static BufferString* ret = 0;
    if ( ret ) return ret->buf();

    ret = new BufferString( GetEnvVar("DATAPATH") );
    if ( File_isDirectory(ret->buf()) )
	return ret->buf();

    //TODO support ~/.datapath
    // ~/.datapath may contain: [machine_name] datapath=/path/to_file/

    return ret->buf();
}


#define mErrRet(s1,s2,s3) \
	{ errmsg_ = s1; errmsg_+= s2; errmsg_ += s3; return StreamData(); }

StreamData ODMad::FileSpec::open() const
{
    FilePath fp( fname_ );
    if ( !fname_.isEmpty() && !fp.isAbsolute() )
	fp.setPath( datapath_ );

    const BufferString fname( fp.fullPath() );
    if ( fname_.isEmpty() )
	const_cast<ODMad::FileSpec*>(this)->fname_ = StreamProvider::sStdIO;
    else if ( forread_ )
    {
	if ( !File_exists(fname_) )
	    mErrRet("File '",fname_,"' does not exist")
	else if ( File_isEmpty(fname_) )
	    mErrRet("File '",fname_,"' is empty")
    }
    else
    {
	const BufferString dirnm( fp.pathOnly() );
	if ( !File_isDirectory(dirnm) )
	    mErrRet("Directory '",dirnm,"' does not exist")
	else if ( !File_isWritable(dirnm) )
	    mErrRet("Directory '",dirnm,"' is not writable")
    }

    StreamData ret = forread_ ? StreamProvider(fname_).makeIStream()
			      : StreamProvider(fname_).makeOStream();
    if ( !ret.usable() )
	mErrRet("File '",fname_,"' cannot be opened")

    return ret;
}
