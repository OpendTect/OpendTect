/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2014
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
    if ( icsetnm == "Default" )
	trydeficons_ = false;
    else
    {
	trydeficons_ = true;
	deficdirnm_ = mGetSetupFileName( mIconDirStart mIconDirDefault );
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
    const BufferString& dirnm = indef ? deficdirnm_ : icdirnm_;
    FilePath fp( dirnm, BufferString(id,".png") );
    const BufferString simplefnm( fp.fullPath() );
    if ( File::exists(simplefnm) )
	nms_.add( simplefnm );

    BufferStringSet iconsubnms;
    ::Settings::common().get( "Icon sizes", iconsubnms );
    iconsubnms.addIfNew( "small" );

    for ( int idx=0; idx<iconsubnms.size(); idx++ )
    {
	FilePath fpsz( dirnm );
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
