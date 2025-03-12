/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "plugins.h"

#include "debug.h"
#include "dirlist.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "oddirs.h"
#include "separstr.h"
#include "settings.h"
#include "perthreadrepos.h"
#include "moddepmgr.h"
#include "msgh.h"
#include "keystrs.h"
#include "od_istream.h"
#include <string.h>

#ifndef __win__
# include <dlfcn.h>
#endif


static const char* sPluginDir = "plugins";
static const char* sPluginBinDir = "bin";

static const char* sKeyNoDispName = "??";

extern "C" {

    using VoidIntRetFn = int(*)(void);
    using ArgcArgvCCRetFn = const char*(*)(int,char**);
    using PluginInfoRetFn = PluginInfo*(*)(void);

    mExternC(Basic) void LoadAutoPlugins( int inittype )
    {
	PIM().loadAuto( inittype == PI_AUTO_INIT_LATE );
    }

    mExternC(Basic) int LoadPlugin( const char* libnm )
    {
	return PIM().load( libnm ) ? 1 : 0;
    }
};


SharedLibAccess::SharedLibAccess( const char* lnm )
	: handle_(nullptr)
{
    if ( !lnm || !*lnm  )
	return;

#ifdef __win__

    BufferString targetlibnm( lnm );
    if ( File::isLink(lnm) )
	targetlibnm = File::linkEnd(lnm);

    if ( File::exists(targetlibnm) )
    {
	const UINT oldmod = GetErrorMode();
	SetErrorMode( SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS );
	handle_ = LoadLibrary( targetlibnm );
	if ( !handle_ )
	{
	    char* ptr = NULL;
	    FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
			   FORMAT_MESSAGE_FROM_SYSTEM, NULL,
			   GetLastError(), 0, (char* )&ptr, 1024, NULL );
	    errmsg_ = BufferString( ptr );
	}
	SetErrorMode( oldmod );
    }

#else

    if ( File::exists(lnm) )
    {
	handle_ = dlopen( lnm, RTLD_GLOBAL | RTLD_NOW );

	if ( !handle_ )
	    errmsg_ = BufferString( dlerror() );

	dlerror();    /* Clear any existing error */
    }

#endif
    else
    {
	errmsg_.set( "Library file not found: " ).add( lnm );
    }

    if ( !errmsg_.isEmpty() )
	ErrMsg( errmsg_ );

    if( DBG::isOn(DBG_PROGSTART) )
    {
	BufferString msg( "Attempt to get open handle for sh lib " );
	msg += lnm; msg += handle_ ? " succeeded" : " failed";
	if ( !handle_ && !errmsg_.isEmpty() )
	    msg.add( ": " ).add( errmsg_ );

	DBG::message( msg );
    }
}


void SharedLibAccess::close()
{
    if ( !handle_ ) return;
#ifdef __win__
    FreeLibrary( handle_ );
#else
    dlclose( handle_ );
#endif
    handle_ = nullptr;
}


void* SharedLibAccess::getFunction( const char* fnnm ) const
{
    if ( !handle_ )
	return nullptr;

#ifdef __win__
    return (void*)GetProcAddress( handle_, fnnm );
#else
    return dlsym( handle_, fnnm );
#endif
}


static BufferString mkLibName( const char* modnm )
{
    BufferString ret;
    if ( !__iswin__ )
	ret.set( "lib" );

    ret.add( modnm ).add( "." );
    if ( __iswin__ )
	ret.add( "dll" );
    else if ( __ismac__ )
	ret.add( "dylib" );
    else if ( __islinux__ )
	ret.add( "so" );

    return ret;
}


void SharedLibAccess::getLibName( const char* modnm, char* out, int sz )
{
    BufferString libnm( mkLibName(modnm) );
#ifdef __win__
    strncpy_s( out, sz, libnm.buf(), sz );
#else
    strncpy( out, libnm.buf(), sz );
#endif
}


// RuntimeLibLoader

RuntimeLibLoader::RuntimeLibLoader( const char* filenm, const char* subdir )
{
    const FilePath libfp( filenm );
    const FilePath relfp( __iswin__ ? GetExecPlfDir() : GetLibPlfDir(),
			  subdir, libfp.fileName() );
    if ( relfp.exists() )
	sha_ = new SharedLibAccess( relfp.fullPath() );
    else if ( libfp.exists() )
	sha_ = new SharedLibAccess( libfp.fullPath() );
}


RuntimeLibLoader::~RuntimeLibLoader()
{
    if ( sha_ )
	sha_->close();

    delete sha_;
}

bool RuntimeLibLoader::isOK() const
{
    return sha_ && sha_->isOK();
}


static const char* getFileBaseName( const char* libnm )
{
    mDeclStaticString( ret );
    const FilePath fp( libnm );
    const BufferString filebasenm = fp.baseName();
    if ( !__iswin__ && filebasenm.startsWith("lib",OD::CaseInsensitive) )
	ret.set( filebasenm.str()+3 );
    else
	ret.set( filebasenm );

    return ret.buf();
}


// PluginManager::Data

BufferString PluginManager::Data::getFileName() const
{
    return name_;
}


BufferString PluginManager::Data::getBaseName() const
{
    return BufferString( getFileBaseName(getFileName()) );
}


static const char* getFnName( const char* fnbeg, const char* libnm,
			      const char* fnend )
{
    mDeclStaticString( ret );
    ret.set( fnbeg ).add( getFileBaseName(libnm) ).add( fnend );
    return ret.buf();
}


static bool isALO( const char* fnm )
{
    const char* extptr = lastOcc( fnm, '.' );
    if ( !extptr || !*extptr ) return false;
    return caseInsensitiveEqual( extptr+1, "alo", 0 );
}


static PluginInfo* mkEmptyInfo()
{
    return new PluginInfo( sKeyNoDispName, sKey::EmptyString(),
			   sKey::EmptyString(),
			   sKey::EmptyString(),"No info available");
}


static PluginInfo* getPluginInfo( SharedLibAccess* sla, const char* libnm )
{
    PluginInfoRetFn fn = nullptr;
    if ( sla )
    {
	const BufferString funcnm = getFnName( "Get", libnm, "PluginInfo" );
	fn = (PluginInfoRetFn)sla->getFunction( funcnm );
    }

    return fn ? (*fn)() : mkEmptyInfo();
}


static int getPluginType( SharedLibAccess* sla, const char* libnm )
{
    VoidIntRetFn fn = nullptr;
    if ( sla )
    {
	const BufferString funcnm = getFnName( "Get", libnm, "PluginType" );
	fn = (VoidIntRetFn)sla->getFunction( funcnm );
    }

    return fn ? (*fn)() : PI_AUTO_INIT_LATE;
}


using boolFromVoidFn = bool(*)(void);
static boolFromVoidFn localsslfn_ = nullptr;

mGlobal(Basic) void setGlobal_Basic_OpenSSLFn(boolFromVoidFn);
void setGlobal_Basic_OpenSSLFn( boolFromVoidFn sslfn )
{
    localsslfn_ = sslfn;
}

static bool loadOpenSSLIfNeeded( const char* libbasenm )
{
    static int res = -1;
    if ( res >=0 )
	return res == 1;

    if ( !__islinux__ )
    {
	res = 1;
	return res == 1;
    }

    FilePath opensslfp( GetSetupDataFileName(ODSetupLoc_SWDirOnly,
					     "OpenSSL", false), libbasenm );
    opensslfp.setExtension( "txt" );
    if ( !opensslfp.exists() )
	return false;

    res = localsslfn_ && (*localsslfn_)() ? 1 : 0;
    return res == 1;
}


static bool loadPlugin( SharedLibAccess* sla, int argc, char** argv,
			const char* libnm, bool mandatory )
{
    ArgcArgvCCRetFn fn = nullptr;
    if ( sla )
    {
	const BufferString funcnm = getFnName( "Init", libnm, "Plugin" );
	fn = (ArgcArgvCCRetFn)sla->getFunction( funcnm );
    }

    if ( !fn ) // their bad
    {
	if ( mandatory )
	{
	    const BufferString libnmonly = FilePath(libnm).fileName();
	    ErrMsg( BufferString( libnmonly,
			" does not have a InitPlugin function"));
	}

	return false;
    }

    const char* ret = (*fn)( argc, argv );
    if ( ret && *ret )
    {
	const BufferString libnmonly = FilePath(libnm).fileName();
	BufferString msg( "Message from " );
	msg += libnm; msg += ":\n\t"; msg += ret;
	UsrMsg( msg );
	return false;
    }

    return true;
}


// PluginManager

PluginManager::PluginManager()
    : allPluginsLoaded(this)
{
    if ( !AreProgramArgsSet() )
    {
	pErrMsg("Program arguments are not set!");
	DBG::forceCrash(false);
    }

    getDefDirs();
    mkALOList();
}


PluginManager::~PluginManager()
{
    deepErase( data_ );
}


const char* PluginManager::sKeyApplPluginDir()
{
    return "OD_APPL_PLUGIN_DIR";
}


const char* PluginManager::sKeyUserPluginDir()
{
    return "OD_USER_PLUGIN_DIR";
}


const char* PluginManager::getApplDir()
{
    mDeclStaticString( ret );
    if ( ret.isEmpty() )
	ret = GetEnvVar( sKeyApplPluginDir() );

    return ret.buf();
}


const char* PluginManager::getUserDir()
{
    mDeclStaticString( ret );
    if ( ret.isEmpty() )
	ret = GetEnvVar( sKeyUserPluginDir() );

    return ret.buf();
}


void PluginManager::getDefDirs()
{
    BufferString dnm = getApplDir();
    if ( dnm.isEmpty() )
    {
	appdir_ = GetSoftwareDir(false);
	applibdir_ = GetLibPlfDir();
    }
    else
    {
	FilePath fp( dnm );
	appdir_ = fp.fullPath();
	fp.add( sPluginBinDir );
	fp.add( GetPlfSubDir() );
	fp.add( GetBinSubDir() );

	applibdir_ = fp.fullPath();
    }

    dnm = getUserDir();
    if ( dnm.isEmpty() )
	dnm = GetSettingsDir();

    FilePath fp( dnm );
    userdir_ = fp.fullPath();
    fp.add( sPluginBinDir );
    fp.add( GetPlfSubDir() );
    fp.add( GetBinSubDir() );
    userlibdir_ = fp.fullPath();

    if( DBG::isOn(DBG_PROGSTART) )
    {
        BufferString msg( "plugins.cc - getDefDirs\n" );
	msg += "appdir_="; msg += appdir_;
	msg += " userdir_="; msg += userdir_;
	msg += "\napplibdir_="; msg += applibdir_;
	msg += " userlibdir_="; msg += userlibdir_;
	DBG::message( msg );
    }
}


const PluginManager::Data* PluginManager::findDataWithDispName(
						const char* nm ) const
{
    if ( !nm || !*nm )
	return nullptr;

    for ( auto* data : data_ )
    {
	const PluginInfo* piinf = data->info_;
	if ( piinf && piinf->dispname_ && StringView(piinf->dispname_)==nm )
	    return data;
    }

    return nullptr;
}


const char* PluginManager::getFileName( const PluginManager::Data& data ) const
{
    mDeclStaticString( ret );
    if ( data.autosource_ == Data::None )
	ret = data.getFileName();
    else
	ret = FilePath(
		data.autosource_ == Data::AppDir ?  applibdir_ : userlibdir_,
		data.getFileName() ).fullPath();

    return ret.buf();
}


PluginManager::Data* PluginManager::fndData( const char* nm ) const
{
    for ( auto* data : data_ )
    {
	if ( data->name_ == nm )
	    return const_cast<Data*>(data);
    }

    return nullptr;
}


bool PluginManager::isPresent( const char* nm ) const
{
    return findData( nm );
}


const char* PluginManager::userName( const char* nm ) const
{
    const Data* data = findData( nm );
    const PluginInfo* piinf = data ? data->info_ : nullptr;
    if ( !piinf )
	return getFileBaseName( nm );

    return piinf->dispname_;
}


const char* PluginManager::moduleName( const char* nm )
{
    return getFileBaseName( nm );
}


void PluginManager::getNotLoadedByUser( BufferStringSet& dontloadlist ) const
{
    BufferString bs;
    Settings::common().get( sKeyDontLoad(), bs );
    FileMultiString fms( bs );
    const int sz = fms.size();
    for ( int idx=0; idx<sz; idx++ )
    {
	const BufferString pinm( fms[idx] );
	if ( !pinm.isEmpty() )
	    dontloadlist.add( pinm );
    }
}


void PluginManager::openALOEntries()
{
    for ( auto* dataptr : data_ )
    {
	Data& data = *dataptr;
	deleteAndNullPtr( data.sla_ );
	if ( data.autosource_ == Data::None )
	    continue;

	loadOpenSSLIfNeeded( data.getBaseName() );
	data.sla_ = new SharedLibAccess( getFileName(data) );
	if ( !data.sla_->isOK() )
	{
	    const StringView errmsg( data.sla_->errMsg() );
	    if ( !errmsg.isEmpty() )
		ErrMsg( errmsg );

	    deleteAndNullPtr( data.sla_ );
	    if ( data.autosource_ == Data::Both )
	    {
		data.autosource_ = data.autosource_ == Data::UserDir
				    ? Data::AppDir : Data::UserDir;
		data.sla_ = new SharedLibAccess( getFileName(data) );
		if ( !data.sla_->isOK() )
		    { deleteAndNullPtr( data.sla_ ); }
	    }
	}

	if ( data.sla_ )
	{
	    data.autotype_ = getPluginType( data.sla_, data.name_ );
	    data.info_ = getPluginInfo( data.sla_, data.name_ );
	}
	else
	    OD::ModDeps().ensureLoaded( data.getBaseName() );
    }
}


void PluginManager::getALOEntries( const char* dirnm, bool usrdir )
{
    FilePath fp( dirnm, sPluginDir, GetPlfSubDir() );
    DirList dl( fp.fullPath(), File::FilesInDir );
    const BufferString prognm = GetExecutableName();
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	BufferString fnm = dl.get(idx);
	if ( !isALO(fnm) ) continue;

	char* ptr = firstOcc( fnm.getCStr(), '.' );
	if ( ptr ) *ptr = '\0';
	if ( fnm != prognm ) continue;

	od_istream strm( dl.fullPath(idx) );
	while ( strm.isOK() )
	{
	    BufferString libnmbase;
	    if ( !strm.getLine(libnmbase) || libnmbase.isEmpty() )
		continue;

	    const BufferString libnm( mkLibName(libnmbase) );
	    Data* data = findData( libnm.buf() );
	    if ( !data )
	    {
		data = new Data( libnm );
		data->autosource_ = usrdir ? Data::UserDir : Data::AppDir;
		if ( !FilePath(getFileName(*data)).exists() )
		{
		    data->autosource_ = usrdir ? Data::AppDir : Data::UserDir;
		    if ( !FilePath(getFileName(*data)).exists() )
			continue;
		}

		data_ += data;
	    }
	}
    }
}


void PluginManager::mkALOList()
{
    getALOEntries( userdir_, true );
    FilePath fp( appdir_ );
    if ( __ismac__ )
	fp.add( "Resources" );

    getALOEntries( fp.fullPath(), false );
    openALOEntries();
}


bool PluginManager::load( const char* libnm )
{
    return load( libnm, Data::None, -1 );
}


bool PluginManager::load( const char* libnm, Data::AutoSource src,
			  int autotype )
{
    FilePath fp( libnm );
    const BufferString libnmonly( fp.fileName() );

    Data* data = new Data( libnmonly );
    loadOpenSSLIfNeeded( data->getBaseName() );
    data->sla_ = new SharedLibAccess( libnm );
    if ( !data->sla_->isOK() )
    {
	ErrMsg( data->sla_->errMsg(), true );
	if ( src == Data::None )
	    delete data;
	else
	    data_.add( data );

	return false;
    }

    data->info_ = getPluginInfo( data->sla_, libnmonly );
    if ( !data->info_ )
    {
	ErrMsg( BufferString( libnm, " does not return plugin information.") );
	if ( src == Data::None )
	    delete data;
	else
	    data_.add( data );

	return false;
    }

    Data* existing = const_cast<Data*>(
		     findDataWithDispName( data->info_->dispname_ ) );

    if ( existing && existing->sla_ && existing->sla_->isOK() )
    {
	data->sla_->close();
	deleteAndNullPtr( data );

	if ( existing->isloaded_ )
	{
	    ErrMsg( BufferString( libnm, " is already loaded.") );
	    return false;
	}

	if ( !loadPlugin(existing->sla_,GetArgC(),GetArgV(),libnmonly,true) )
	{
	    existing->info_ = nullptr;
	    existing->sla_->close();
	    deleteAndNullPtr( existing->sla_ );
	    return false;
	}

	existing->isloaded_ = true;
    }
    else
    {
	existing = findData( libnmonly.buf() );
	if ( existing )
	{
	    if ( existing->sla_ )
		existing->sla_->close();
	    else
	    {
		existing->sla_ = data->sla_;
		existing->info_ = data->info_;
		data->sla_ = nullptr;
	    }

	    deleteAndNullPtr( data );

	    if ( existing->isloaded_ )
	    {
		ErrMsg( BufferString( libnm, " is already loaded.") );
		return false;
	    }

	    if ( !loadPlugin(existing->sla_,GetArgC(),GetArgV(),libnmonly,true))
	    {
		existing->info_ = nullptr;
		deleteAndNullPtr( existing->sla_ );
		return false;
	    }

	    existing->isloaded_ = true;
	}
	else
	{
	    data->autosource_ = src;
	    data->autotype_ = autotype;
	    if ( !loadPlugin(data->sla_,GetArgC(),GetArgV(),libnmonly,true) )
	    {
		if ( src == Data::None )
		{
		    data->sla_->close();
		    delete data;
		}
		else
		{
		    data->info_ = nullptr;
		    deleteAndNullPtr( data->sla_ );
		    data_.add( data );
		}

		return false;
	    }

	    data->isloaded_ = true;
	    data_.add( data );
	}
    }

    mDefineStaticLocalObject(bool,shw_load,
			     = GetEnvVarYN("OD_SHOW_PLUGIN_LOAD"));
    if ( shw_load && data && data->isloaded_ )
    {
	BufferString msg;
	if ( data->sla_ )
	    msg = "Successfully loaded plugin '";
	else
	    msg = "Failed to load plugin '";

	msg += userName(data->name_); msg += "'";
	UsrMsg( msg );
    }

    return true;
}


bool PluginManager::unload( const char* libnm )
{
    Data* data = findData( libnm );
    if ( !data || !data->isloaded_ || !data->sla_ )
	return false;

    data->sla_->close();
    deleteAndNullPtr( data->sla_ );
    data->info_ = nullptr;
    data->isloaded_ = false;
    return true;
}


void PluginManager::loadAuto( bool late, bool withfilter )
{
    BufferStringSet dontloadlist;
    if ( withfilter )
	getNotLoadedByUser( dontloadlist );

    const int pitype = late ? PI_AUTO_INIT_LATE : PI_AUTO_INIT_EARLY;
    for ( auto* dataptr : data_ )
    {
	Data& data = *dataptr;
	if ( !data.sla_ || !data.sla_->isOK() || data.autosource_==Data::None )
	    continue;

	if ( data.autotype_ != pitype || data.isloaded_ )
	    continue;

	const BufferString modnm = data.getBaseName();
	if ( data.info_ && dontloadlist.isPresent(modnm) )
	    continue;

	data.isloaded_ = loadPlugin( data.sla_, GetArgC(), GetArgV(),
				     data.name_, false );
	if ( !data.isloaded_ )
	{
	    data.info_ = nullptr;
	    data.sla_->close();
	    deleteAndNullPtr( data.sla_ );
	}

	mDefineStaticLocalObject(bool,shw_load,
				 = GetEnvVarYN("OD_SHOW_PLUGIN_LOAD"));
	if ( shw_load )
	{
	    BufferString msg;
	    if ( data.sla_ )
		msg = "Successfully loaded plugin '";
	    else
		msg = "Failed to load plugin '";

	    msg += userName(data.name_); msg += "'";
	    UsrMsg( msg );
	}
    }

    allPluginsLoaded.trigger( pitype );
}


void PluginManager::unLoadAll()
{
    for ( auto* data : data_ )
    {
	if ( !data || !data->isloaded_ || !data->sla_ )
	    continue;

	data->sla_->close();
	deleteAndNullPtr( data->sla_ );
	data->isloaded_ = false;
    }
}


PluginManager& PIM()
{
    mDefineStaticLocalObject(PtrMan<PluginManager>,inst,= new PluginManager);
    return *inst;
}
