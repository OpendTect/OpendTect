/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "madio.h"
#include "keystrs.h"
#include "file.h"
#include "filepath.h"
#include "strmprov.h"
#include "envvars.h"
#include "oddirs.h"
#include "iopar.h"
#include "perthreadrepos.h"

const char* ODMad::FileSpec::sKeyMaskFile()	{ return "Mask File Name"; }
const char* ODMad::sKeyMadagascar()		{ return "Madagascar"; }
int ODMad::sKeyMadSelKey()			{ return 909090; }


ODMad::FileSpec::FileSpec( bool fr )
    : forread_(fr)
{
}


bool ODMad::FileSpec::fileNameOK( const char* fnm ) const
{
    if ( !forread_ )
    {
	FilePath fp( fnm );
	if ( !File::isWritable(fp.pathOnly()) )
	{
	    errmsg_ = tr("Directory '%1' is not writable")
		    .arg(fp.pathOnly());
	    return false;
	}
    }
    else if ( fnm && *fnm && File::isEmpty(fnm) )
    {
	errmsg_ = tr("File '%1' is not readable")
		.arg(fnm);
	return false;
    }

    return true;
}


bool ODMad::FileSpec::set( const char* fnm, const char* maskfnm )
{
    if ( !fnm || !*fnm )
    { errmsg_ = tr("No file name provided"); return false; }
    FilePath fp( fnm );
    if ( !fp.isAbsolute() )
	fp.set( GetDataDir() ).add( sKeyMadagascar() ).add( fnm );
    fnm_ = fp.fullPath();

    if ( !maskfnm || !*maskfnm )
	maskfnm_.setEmpty();
    else
    {
	fp.set( maskfnm );
	if ( !fp.isAbsolute() )
	    fp.set( GetDataDir() ).add( sKeyMadagascar() ).add( maskfnm );
	maskfnm_ = fp.fullPath();
    }

    return fileNameOK( fnm_ ) && fileNameOK( maskfnm_ );
}


void ODMad::FileSpec::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Type(), forread_ ? "Read" : "Write" );
    iop.set( sKey::FileName(), fnm_ );
    iop.set( sKeyMaskFile(), maskfnm_ );
}


bool ODMad::FileSpec::usePar( const IOPar& iop )
{
    const BufferString res = iop.find( sKey::Type() );
    if ( res.isEmpty() )
	forread_ = true;
    else
    {
	const BufferString firstchar( res[0] );
	forread_ = !firstchar.isEqual( "W", CaseInsensitive );
    }

    BufferString fnm = fnm_, maskfnm = maskfnm_;
    iop.get( sKey::FileName(), fnm );
    iop.get( sKeyMaskFile(), maskfnm );
    return set( fnm, maskfnm );
}


const char* ODMad::FileSpec::defPath()
{
    mDeclStaticString( ret );
    ret = FilePath( GetDataDir(), sKeyMadagascar() ).fullPath();
    return ret.buf();
}


const char* ODMad::FileSpec::madDataPath()
{
    mDeclStaticString( ret );

    ret = GetEnvVar("DATAPATH");
    if ( ret.isEmpty() )
	ret = defPath();
    return ret.buf();
}


StreamData ODMad::FileSpec::open() const
{
    return doOpen( fnm_ );
}


StreamData ODMad::FileSpec::openMask() const
{
    return maskfnm_.isEmpty() ? StreamData() : doOpen( maskfnm_ );
}


#define mErrRet(msg) \
	{ errmsg_ = msg; return StreamData(); }

StreamData ODMad::FileSpec::doOpen( const char* fnm ) const
{
    if ( forread_ )
    {
	if (!File::exists(fnm))
	    mErrRet(tr("File '%1' does not exist").arg( fnm ) )
	else if ( File::isEmpty(fnm) )
	    mErrRet(tr("File '%1' is empty").arg(fnm))
    }
    else
    {
	FilePath fp( fnm );
	const BufferString dirnm( fp.pathOnly() );
	if ( !File::isDirectory(dirnm) )
	    mErrRet(tr("Directory '%1' does not exist").arg( dirnm ))
	else if ( !File::isWritable(dirnm) )
	    mErrRet(tr("Directory '%1' is not writable").arg( dirnm ) )
    }

    StreamData ret = forread_ ? StreamProvider::createIStream( fnm )
			      : StreamProvider::createOStream( fnm );
    if ( !ret.usable() )
	    mErrRet(tr("File '%1' cannot be opened").arg( fnm ) )

    return ret;
}
