/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odiconfile.h"

#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "dirlist.h"
#include "settings.h"
#include "perthreadrepos.h"

static const char* sFileNameEnd = ".png";
#define mIconDirStart "icons."
#define mIconDirDefault "Default"


OD::IconFile::IconFile( OD::StdActionType typ )
{
    init( getIdentifier(typ) );
}


OD::IconFile::IconFile( const char* identifier )
{
    init( identifier );
}


void OD::IconFile::init( const char* identifier )
{
    BufferString icsetnm( mIconDirDefault );
    ::Settings::common().get( "Icon set name", icsetnm );
    BufferString dirnm( mIconDirStart, icsetnm );
    icdirnm_ = mGetSetupFileName(dirnm);
    alticdirnm_ = GetSetupDataFileName( ODSetupLoc_UserPluginDirOnly, dirnm,
					true );
    if ( alticdirnm_ == icdirnm_ )
	alticdirnm_.setEmpty();

    if ( icsetnm == "Default" )
	trydeficons_ = false;
    else
    {
	trydeficons_ = true;
	deficdirnm_ = mGetSetupFileName( mIconDirStart mIconDirDefault );
	altdeficdirnm_ = GetSetupDataFileName( ODSetupLoc_UserPluginDirOnly,
					mIconDirStart mIconDirDefault, true );
	if ( altdeficdirnm_ == deficdirnm_ )
	    altdeficdirnm_.setEmpty();
    }

    set( identifier );
}


void OD::IconFile::set( const char* inp )
{
    nms_.setEmpty();
    setName( inp );

    BufferString inpstr = inp;
    if ( inpstr.isEmpty() )
	return;

    FilePath fp( inpstr );
    if ( fp.isAbsolute() )
    {
	if ( File::exists(inpstr) )
	    nms_.add( inpstr );
	return;
    }

    BufferString identifier( inpstr );
    if ( inpstr.endsWith(sFileNameEnd) )
    {
	char* pngptr = lastOcc( inpstr.getCStr(), sFileNameEnd );
	if ( pngptr ) *pngptr = '\0';
	setName( inpstr );
	identifier = inpstr;
    }

    if ( findIcons(identifier,false) )
	return;

    if ( trydeficons_ )
    {
	if ( findIcons(identifier,true) )
	    return;
    }

    pErrMsg(BufferString("No icon found for identifier '",identifier,"'"));
}


const char* OD::IconFile::getIdentifier( OD::StdActionType typ )
{
    switch( typ )
    {
	case Apply: return "apply";
	case Cancel: return "cancel";
	case Define: return "define";
	case Delete: return "remove";
	case Edit: return "edit";
	case Examine: return "examine";
	case Help: return "help";
	case Ok: return "ok";
	case Options: return "options";
	case Properties: return "options";
	case Rename: return "renameobj";
	case Remove: return "delete";
	case Save: return "save";
	case SaveAs: return "saveas";
	case Select: return "selectfromlist";
	case Settings: return "options";
	case Unload: return "unload";
	case Video: return "video";
	default: break;
    }

    return "no_icon";
}


bool OD::IconFile::findIcons( const char* id, bool indef )
{
    const BufferString iconfnm( id, sFileNameEnd );

    const BufferString* dirnm = indef ? &deficdirnm_ : &icdirnm_;
    FilePath fp( dirnm->str(), iconfnm.str() );
    BufferString simplefnm = fp.fullPath();
    if ( File::exists(simplefnm.str()) )
	nms_.add( simplefnm.str() );
    else
    {
	const BufferString& altdirnm = indef ? altdeficdirnm_ : alticdirnm_;
	if ( !altdirnm.isEmpty() )
	{
	    fp.set( altdirnm.str() ).add( iconfnm.str() );
	    if ( fp.exists() )
	    {
		dirnm = &altdirnm;
		simplefnm = fp.fullPath();
		nms_.add( simplefnm.str() );
	    }
	}
    }

    BufferStringSet iconsubnms;
    ::Settings::common().get( "Icon sizes", iconsubnms );
    iconsubnms.addIfNew( "small" );

    for ( int idx=0; idx<iconsubnms.size(); idx++ )
    {
	FilePath fpsz( dirnm->buf() );
	BufferString fnm( id, ".", iconsubnms.get(idx) );
	fnm.add( sFileNameEnd );
	fpsz.add( fnm );
	if ( fpsz.exists() )
	    nms_.add( fpsz.fullPath() );
    }

    return nms_.size();
}


bool OD::IconFile::isPresent( const char* identifier )
{
    OD::IconFile icf( identifier );
    return !icf.nms_.isEmpty();
}


const char* OD::IconFile::notFoundIconFileName()
{
    mDeclStaticString( ret );

    if ( ret.isEmpty() )
    {
	OD::IconFile icf( "iconnotfound" );
	if ( !icf.haveData() )
	    ret = mGetSetupFileName("od.png");
	else
	    ret = icf.nms_.get(0);
    }

    return ret.str();
}
