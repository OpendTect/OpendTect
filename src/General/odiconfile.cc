/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odiconfile.h"

#include "dirlist.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "perthreadrepos.h"
#include "settings.h"

static const char* sFileNameEnd = ".png";
#define mIconDirStart "icons."
#define mIconDirDefault "Default"

namespace OD
{

const BufferStringSet& getIconDirs()
{
    static PtrMan<BufferStringSet> ret = new BufferStringSet();
    return *ret.ptr();
}


const BufferStringSet& getIconSubNames()
{
    static PtrMan<BufferStringSet> ret = new BufferStringSet();
    BufferStringSet& iconsubnms = *ret.ptr();
    ::Settings::common().get( "Icon sizes", iconsubnms );
    iconsubnms.addIfNew( "small" );

    return *ret.ptr();
}

} // namespace OD


OD::IconFile::IconFile( OD::StdActionType typ )
{
    init( getIdentifier(typ) );
}


OD::IconFile::IconFile( const char* identifier )
{
    init( identifier );
}


const char* OD::IconFile::getIconSubFolderName()
{
    mDeclStaticString( ret );
    ret.set( mIconDirStart );
    BufferString icsetnm( mIconDirDefault );
    ::Settings::common().get( "Icon set name", icsetnm );
    ret.add( icsetnm );

    return ret.str();
}


const char* OD::IconFile::getDefaultIconSubFolderName()
{
    mDeclStaticString( ret );
    if ( ret.isEmpty() )
	ret.set( mIconDirStart ).add( mIconDirDefault );

    return ret.str();
}


void OD::IconFile::init( const char* identifier )
{
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

    if ( findIcons(identifier) )
	return;

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


bool OD::IconFile::findIcons( const char* id )
{
    const BufferString iconfnm( id, sFileNameEnd );
    const BufferStringSet& icondirs = getIconDirs();
    for ( const auto* icondir : icondirs )
    {
	const FilePath fp( icondir->str(), iconfnm.str() );
	const BufferString simplefnm = fp.fullPath();
	if ( !File::exists(simplefnm.str()) )
	    continue;

	nms_.add( simplefnm.str() );
	break;
    }

    const BufferStringSet& iconsubnms = getIconSubNames();
    for ( const auto* icondir : icondirs )
    {
	for ( const auto* iconsubnm : iconsubnms )
	{
	    FilePath fpsz( icondir->buf() );
	    BufferString fnm( id, ".", iconsubnm->buf() );
	    fnm.add( sFileNameEnd );
	    fpsz.add( fnm );
	    if ( fpsz.exists() )
		nms_.add( fpsz.fullPath() );
	}
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
	if ( icf.haveData() )
	    ret = icf.nms_.first()->buf();
	else
	    ret = GetSWSetupShareFileName( "od.png" );
    }

    return ret.str();
}


void OD::IconFile::getIconSubFolderNames( BufferStringSet& ret )
{
    for ( const auto* dirnm : getIconDirs() )
    {
	const FilePath fp( dirnm->str() );
	const BufferString parentdir = fp.pathOnly();
	const DirList dl( parentdir.buf(), File::DirListType::DirsInDir,
			  "icons.*" );
	for ( int idx=0; idx<dl.size(); idx++ )
	{
	    const FilePath dirfp( dl.get(idx) );
	    ret.addIfNew( dirfp.extension() );
	}
    }
}


void OD::IconFile::reInit()
{
    const_cast<BufferStringSet&>( getIconDirs() ).setEmpty();
}


bool OD::addIconsFolder( const char* path )
{
    if ( !File::isDirectory(path) )
	return false;

    auto& icondirs = const_cast<BufferStringSet&>( getIconDirs() );
    return icondirs.addIfNew( path );
}
