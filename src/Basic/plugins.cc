/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2003
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
#include "staticstring.h"
#include "moddepmgr.h"
#include "msgh.h"
#include "od_istream.h"
#include "odver.h"
#include "odplugin.h"
#include <string.h>

#ifndef __win__
# include <dlfcn.h>
#endif


static const char* sPluginDir = "plugins";
static const char* sPluginBinDir = "bin";

static const char* sKeyNoDispName = "??";

extern "C" {

    typedef int (*VoidIntRetFn)(void);
    typedef void (*VoidVoidRetFn)(void);
    typedef const char* (*ArgcArgvCCRetFn)(int,char**);
    typedef PluginInfo* (*PluginInfoRetFn)(void);

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
	    errmsg_.set( ptr );
	}
	SetErrorMode( oldmod );
    }

#else

    if ( File::exists(lnm) )
    {
	handle_ = dlopen( lnm, RTLD_GLOBAL | RTLD_NOW );

	if ( !handle_ )
	    errmsg_.set( dlerror() );

	dlerror();    /* Clear any existing error */
    }

#endif
    else
    {
	errmsg_.set( "Library file not found: " ).add( lnm );
    }

#ifdef __debug__
    if ( !errmsg_.isEmpty() )
	ErrMsg( errmsg_ );
#endif

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
#ifdef __win__
    ret.set( modnm ).add( ".dll" );
#else
    ret.set( "lib" ).add( modnm );
# ifdef __mac__
    ret.add( ".dylib" );
# else
    ret.add( ".so" );
# endif
#endif
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



RuntimeLibLoader::RuntimeLibLoader( const char* filenm, const char* subdir )
{
    const File::Path libfp( filenm );
    const File::Path relfp( GetExecPlfDir(), subdir, libfp.fileName() );
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



BufferString PluginManager::Data::version() const
{
    BufferString ver( info_ ? info_->version_ : "" );
    if ( ver.isEmpty() || ver == mODPluginVersion )
	GetSpecificODVersion( 0, ver );
    if ( ver.isEmpty() || ver.startsWith( "[error" ) )
	ver.set( toString(mODMajorVersion) )
	   .add('.').add( mODMinorVersion )
	   .add('.').add( mODPatchVersion );
    return ver;
}


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


static const char* getFnName( const char* libnm, const char* fnbeg,
			      const char* fnend )
{
    mDeclStaticString( ret );

    ret = fnbeg;

    if ( (*libnm     == 'l' || *libnm     == 'L')
      && (*(libnm+1) == 'i' || *(libnm+1) == 'I')
      && (*(libnm+2) == 'b' || *(libnm+2) == 'B') )
	libnm += 3;
    ret += libnm;

    char* ptr = firstOcc( ret.getCStr(), '.' );
    if ( ptr ) *ptr = '\0';
    ret += fnend;

    return ret.buf();
}


void PluginManager::getDefDirs()
{
    BufferString dnm = GetEnvVar( "OD_APPL_PLUGIN_DIR" );
    if ( dnm.isEmpty() )
    {
	appdir_ = GetSoftwareDir(0);
	applibdir_ = GetLibPlfDir();
    }
    else
    {
	File::Path fp( dnm );
	appdir_ = fp.fullPath();
	fp.add( sPluginBinDir );
	fp.add( GetPlfSubDir() );
	fp.add( GetBinSubDir() );

	applibdir_ = fp.fullPath();
    }

    dnm = GetEnvVar( "OD_USER_PLUGIN_DIR" );
    if ( dnm.isEmpty() )
	dnm = GetSettingsDir();

    File::Path fp( dnm );
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


static bool isALO( const char* fnm )
{
    const char* extptr = lastOcc( fnm, '.' );
    if ( !extptr || !*extptr ) return false;
    return caseInsensitiveEqual( extptr+1, "alo", 0 );
}


const PluginManager::Data* PluginManager::findDataWithDispName(
			const char* nm ) const
{
    if ( !nm || !*nm ) return 0;
    for ( auto* data : data_ )
    {
	const PluginInfo* piinf = data->info_;
	if ( piinf && piinf->dispname_ && FixedString(piinf->dispname_)==nm)
	    return data;
    }
    return 0;
}


const char* PluginManager::getFileName( const PluginManager::Data& data ) const
{
    mDeclStaticString( ret );
    if ( data.autosource_ == Data::None )
	ret = data.name_;
    else
	ret = File::Path(
		data.autosource_ == Data::AppDir ?  applibdir_ : userlibdir_,
		data.name_ ).fullPath();
    return ret.buf();
}


PluginManager::Data* PluginManager::fndData( const char* nm ) const
{
    for ( auto* data : data_ )
    {
	if ( data->name_ == nm )
	    return const_cast<Data*>(data);
    }
    return 0;
}


bool PluginManager::isPresent( const char* nm ) const
{
    return findData( nm );
}


const char* PluginManager::userName( const char* nm ) const
{
    const Data* data = findData( nm );
    const PluginInfo* piinf = data ? data->info_ : 0;
    if ( !piinf )
	return moduleName( nm );

    return piinf->dispname_;
}


const char* PluginManager::moduleName( const char* nm )
{
    return getFnName( nm, "", "" );
}


static PluginInfo* mkEmptyInfo()
{
    return new PluginInfo( sKeyNoDispName, OD::EmptyString(), OD::EmptyString(),
			   OD::EmptyString(), "No info available" );
}


#define mGetFn(typ,sla,nm1,nm2,libnm) \
	typ fn = sla ? (typ)sla->getFunction( getFnName(libnm,nm1,nm2) ) : 0


static PluginInfo* getPluginInfo( SharedLibAccess* sla, const char* libnm )
{
    mGetFn(PluginInfoRetFn,sla,"Get","PluginInfo",libnm);
    return fn ? (*fn)() : mkEmptyInfo();
}


static int getPluginType( SharedLibAccess* sla, const char* libnm )
{
    mGetFn(VoidIntRetFn,sla,"Get","PluginType",libnm);
    return fn ? (*fn)() : PI_AUTO_INIT_LATE;
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
	deleteAndZeroPtr( data.sla_ );
	if ( data.autosource_ == Data::None )
	    continue;

	data.sla_ = new SharedLibAccess( getFileName(data) );
	if ( !data.sla_->isOK() )
	{
	    if ( !data.sla_->errMsg().isEmpty() )
		ErrMsg( data.sla_->errMsg() );

	    deleteAndZeroPtr( data.sla_ );
	    if ( data.autosource_ == Data::Both )
	    {
		data.autosource_ = data.autosource_ == Data::UserDir
				    ? Data::AppDir : Data::UserDir;
		data.sla_ = new SharedLibAccess( getFileName(data) );
		if ( !data.sla_->isOK() )
		    { deleteAndZeroPtr( data.sla_ ); }
	    }
	}

	if ( !data.isexternal_ )
	{
	    if ( !data.sla_ )
		OD::ModDeps().ensureLoaded( moduleName( data.name_ ) );
	    else
	    {
		data.autotype_ = getPluginType( data.sla_, data.name_ );
		data.info_ = getPluginInfo( data.sla_, data.name_ );
	    }
	}
    }
}


void PluginManager::getALOEntries( const char* dirnm, bool usrdir )
{
    File::Path fp( dirnm, sPluginDir, GetPlfSubDir() );
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

	    bool isext = false;
	    if ( libnmbase.startsWith("EXT_LIB_") )
	    {
		libnmbase = BufferString( libnmbase.str() + 8 );
		isext = true;
	    }

	    const BufferString libnm( mkLibName(libnmbase) );
	    Data* data = findData( libnm.buf() );
	    if ( !data )
	    {
		data = new Data( libnm );
		data->autosource_ = usrdir ? Data::UserDir : Data::AppDir;
		data_ += data;
	    }
	    else if ( usrdir != Data::isUserDir(data->autosource_) )
		data->autosource_ = Data::Both;

	    data->isexternal_ = isext;
	}
    }
}


void PluginManager::mkALOList()
{
    getALOEntries( userdir_, true );
#ifdef __mac__
    getALOEntries( File::Path(appdir_,"Resources").fullPath(), false );
#else
    getALOEntries( appdir_, false );
#endif
    openALOEntries();
}


static bool loadPlugin( SharedLibAccess* sla, int argc, char** argv,
			const char* libnm, bool mandatory )
{
    mGetFn(ArgcArgvCCRetFn,sla,"Init","Plugin",libnm);
    if ( !fn ) // their bad
    {
	if ( mandatory )
	{
	    const BufferString libnmonly = File::Path(libnm).fileName();
	    ErrMsg( BufferString( libnmonly,
			" does not have a InitPlugin function"));
	}

	return false;
    }

    const char* ret = (*fn)( argc, argv );
    if ( ret )
    {
	const BufferString libnmonly = File::Path(libnm).fileName();
	BufferString msg( "Message from " );
	msg += libnm; msg += ":\n\t"; msg += ret;
	UsrMsg( msg );
	return false;
    }

    return true;
}


static bool loadSurvRelTools( SharedLibAccess* sla, const char* libnm )
{
    mGetFn(VoidVoidRetFn,sla,"Load","PluginSurvRelTools",libnm);
    if ( !fn )
	return false;

    (*fn)();
    return true;
}


bool PluginManager::load( const char* libnm )
{
    File::Path fp( libnm );
    const BufferString libnmonly( fp.fileName() );

    Data* data = new Data( libnmonly );
    data->sla_ = new SharedLibAccess( libnm );
    if ( !data->sla_->isOK() )
    {
	ErrMsg( data->sla_->errMsg(), true );
	delete data;
	return false;
    }

    data->info_ = getPluginInfo( data->sla_, libnmonly );
    if ( !data->info_ )
    {
	ErrMsg( BufferString( libnm, " does not return plugin information.") );
	delete data;
	return false;
    }

    Data* existing = const_cast<Data*>(
	findDataWithDispName( data->info_->dispname_ ) );

    if ( existing && existing->sla_ && existing->sla_->isOK() )
    {
	data->sla_->close();
	delete data;
	data = 0;

	if ( existing->isloaded_ )
	{
	    ErrMsg( BufferString( libnm, " is already loaded.") );
	    return false;
	}

	if ( !data->isexternal_
	  && !loadPlugin(existing->sla_,GetArgC(),GetArgV(),libnmonly,true) )
	{
	    existing->info_ = 0;
	    existing->sla_->close();
	    delete existing->sla_; existing->sla_ = 0;
	    return false;
	}

	existing->isloaded_ = true;
    }
    else
    {
	if ( !data->isexternal_
	  && !loadPlugin(data->sla_,GetArgC(),GetArgV(),libnmonly,true) )
	{
	    data->sla_->close();
	    delete data;
	    return false;
	}

	data->isloaded_ = true;
	data_ += data;
    }

    return true;
}


void PluginManager::loadAuto( bool late )
{
    BufferStringSet dontloadlist;
    getNotLoadedByUser( dontloadlist );

    const int pitype = late ? PI_AUTO_INIT_LATE : PI_AUTO_INIT_EARLY;
    for ( auto* dataptr : data_ )
    {
	Data& data = *dataptr;
	if ( !data.sla_ || !data.sla_->isOK() || data.autosource_==Data::None )
	    continue;

	if ( data.autotype_ != pitype )
	    continue;

	const BufferString modnm = moduleName( data.name_ );
	if ( data.info_ && dontloadlist.isPresent(modnm) )
	    continue;

	if ( !data.isexternal_
	  && !loadPlugin(data.sla_,GetArgC(),GetArgV(),data.name_,false) )
	{
	    data.info_ = 0;
	    data.sla_->close();
	    delete data.sla_; data.sla_ = 0;
	}

	data.isloaded_ = true;

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


void PluginManager::loadSurveyRelatedTools()
{
    BufferStringSet dontloadlist;
    getNotLoadedByUser( dontloadlist );

    for ( auto* dataptr : data_ )
    {
	Data& data = *dataptr;
	if ( !data.sla_ || !data.sla_->isOK() || data.autosource_==Data::None )
	    continue;

	if ( data.autotype_ != PI_AUTO_INIT_LATE )
	    continue;

	const BufferString modnm = moduleName( data.name_ );
	if ( data.info_ && dontloadlist.isPresent(modnm) )
	    continue;

	if ( !loadSurvRelTools(data.sla_,data.name_) )
	    continue;

	data.isloaded_ = true;

	mDefineStaticLocalObject(bool,shw_load,
				 = GetEnvVarYN("OD_SHOW_PLUGIN_LOAD"));
	if ( shw_load )
	{
	    BufferString msg;
	    if ( data.sla_ )
		msg = "Successfully loaded SurvRelTools for plugin '";
	    else
		msg = "Failed to load SurvRelTools for plugin '";
	    msg += userName(data.name_); msg += "'";
	    UsrMsg( msg );
	}
    }
}


void PluginManager::unLoadAll()
{
    for ( auto* data : data_ )
    {
	if ( !data || !data->isloaded_ || !data->sla_ )
	    continue;

	data->sla_->close();
	deleteAndZeroPtr( data->sla_ );
	data->isloaded_ = false;
    }
}


PluginManager& PIM()
{
    mDefineStaticLocalObject(PtrMan<PluginManager>,inst,= new PluginManager);
    return *inst;
}
