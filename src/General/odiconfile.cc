/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "odiconfile.h"

#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "separstr.h"
#include "settings.h"

static const char* sLargeFileEnd = ".png";
static const char* sSmallFileEnd = "_small.png";
#define mIconDirStart "icons."
#define mIconDirDefault "Default"


OD::IconFile::IconFile( const char* identifier )
{
    BufferString icsetnm( mIconDirDefault );
    Settings::common().get( "Icon set name", icsetnm );
    BufferString dirnm( mIconDirStart, icsetnm );
    icdirnm_ = mGetSetupFileName(dirnm);
    if ( icsetnm == "Default" )
	usedeficons_ = false;
    else
    {
	usedeficons_ = true;
	deficdirnm_ = mGetSetupFileName( mIconDirStart mIconDirDefault );
    }

    set( identifier );
}


void OD::IconFile::set( const char* inpstr )
{
    setName( inpstr );
    state_ = Empty;
    fromdefault_ = false;

    fullpath_ = inpstr;
    if ( fullpath_.isEmpty() )
	return;

    FilePath fp( fullpath_ );
    if ( fp.isAbsolute() )
    {
	state_ = File::exists( fullpath_ ) ? Explicit : NotFound;
	return;
    }

    BufferString identifier( inpstr );
    if ( fullpath_.endsWith(sLargeFileEnd) )
    {
	char* pngptr = lastOcc( fullpath_.getCStr(), sLargeFileEnd );
	if ( pngptr ) *pngptr = '\0';
	setName( fullpath_ );
    }

    if ( !tryFind(identifier,false,Exists)
      && !tryFind(identifier,true,SmallOnly) )
    {
	pErrMsg(BufferString("Icon not found: '",identifier,"'"));
	state_ = NotFound;
    }
}


bool OD::IconFile::tryFind( const char* id, bool small,
				OD::IconFile::State state )
{
    BufferString fnm = getIconFileName( id, false, small );

    if ( File::exists(fnm) )
    {
	fullpath_ = fnm; state_ = state;
	return true;
    }

    if ( !usedeficons_ )
    {
	fnm = getIconFileName( id, true, small );
	if ( File::exists(fnm) )
	{
	    fromdefault_ = true; fullpath_ = fnm; state_ = state;
	    return true;
	}
    }

    return false;
}


BufferString OD::IconFile::getIconFileName( const char* id, bool fromdefault,
				bool small ) const
{
    const BufferString fnm( id, small ? sSmallFileEnd : sLargeFileEnd );
    FilePath fp( fromdefault ? deficdirnm_ : icdirnm_, fnm );
    return BufferString( fp.fullPath() );
}


bool OD::IconFile::isPresent( const char* identifier )
{
    OD::IconFile icf( identifier );
    return icf.state_ != NotFound;
}


BufferString OD::IconFile::fullFileName( bool small ) const
{
    switch ( state_ )
    {
	case Exists:
	{
	    if ( small )
	    {
		FilePath fp( fullpath_ );
		fp.setExtension( 0 );
		BufferString fnm( fp.fullPath(), sSmallFileEnd );
		if ( File::exists(fnm) )
		    return fnm;
	    }
	    // fallthrough: small wanted but not available
	}
	case SmallOnly:
	case Explicit:
	    return fullpath_;
	case Empty:
	    return getIconFileName( "empty", true, false );
	default:
	    break;
    }

    return getIconFileName( "iconnotfound", true, false );
}
