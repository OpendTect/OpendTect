#include "plugins.h"
#include "filepath.h"
#include "filegen.h"
#include "dirlist.h"
#include "strmprov.h"
#include "envvars.h"
#include "oddirs.h"

#ifdef __win__
#include <Windows.h>
#else
#include <dlfcn.h>
#endif
#include <iostream>

#include "debugmasks.h"

PluginManager* PluginManager::theinst_ = 0;
static const char* sPluginDir = "plugins";
static const char* sKeyNoDispName = "??";

extern "C" {

    typedef int (*VoidIntRetFn)(void);
    typedef const char* (*ArgcArgvCCRetFn)(int,char**);
    typedef PluginInfo* (*PluginInfoRetFn)(void);

    void LoadAutoPlugins( int argc, char** argv, int inittype )
    {
	static int first_time = 1;
	if ( first_time )
	{
	    first_time = 0;
	    PIM().setArgs( argc, argv );
	}
	PIM().loadAuto( inittype == PI_AUTO_INIT_LATE );
    }

    int LoadPlugin( const char* libnm )
    {
	return PIM().load( libnm ) ? YES : NO;
    }
};


static char* errargv[] = { "<not set>", 0 };

PluginManager::PluginManager()
    : argc_(1)
    , argv_(errargv)
{
    getDefDirs();
}


void PluginManager::setArgs( int argc, char** argv )
{
    argc_ = argc, argv_ = argv;
    // poss todo: get new defdirs from argv
    mkALOList();
}


static BufferString getProgNm( const char* argv0 )
{
    FilePath fp( argv0 );
#ifdef __win__
    fp.setExtension( 0 );
#endif
    return fp.fileName();
}


static const char* getFnName( const char* libnm, const char* fnbeg,
			      const char* fnend )
{
    static BufferString ret;

    ret = fnbeg;

    if ( (*libnm     == 'l' || *libnm     == 'L')
      && (*(libnm+1) == 'i' || *(libnm+1) == 'I')
      && (*(libnm+2) == 'b' || *(libnm+2) == 'B') )
	libnm += 3;
    ret += libnm;

    char* ptr = strchr( ret.buf(), '.' );
    if ( ptr ) *ptr = '\0';
    ret += fnend;

    return ret.buf();
}


void PluginManager::getDefDirs()
{
    bool fromenv = false;
    BufferString dnm = GetEnvVar( "OD_APPL_PLUGIN_DIR" );
    if ( dnm == "" )
	dnm = GetSoftwareDir();
    else
	fromenv = true;

    FilePath fp( dnm );
    if ( !fromenv )
	fp.add( sPluginDir );
    appdir_ = fp.fullPath();
    fp.add( GetPlfSubDir() );
    applibdir_ = fp.fullPath();

    fromenv = false;
    dnm = GetEnvVar( "OD_USER_PLUGIN_DIR" );
    if ( dnm == "" )
	dnm = GetSettingsDir();
    else
	fromenv = true;

    fp.set( dnm );
    if ( !fromenv )
	fp.add( sPluginDir );
    userdir_ = fp.fullPath();
    fp.add( GetPlfSubDir() );
    userlibdir_ = fp.fullPath();

    if( DBG::isOn(DBG_SETTINGS) )
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
    const char* extptr = strrchr( fnm, '.' );
    if ( !extptr || !*extptr ) return false;
    return caseInsensitiveEqual( extptr+1, "alo", 0 );
}


const PluginManager::Data* PluginManager::findDataWithDispName(
			const char* nm ) const
{
    if ( !nm || !*nm ) return 0;
    for ( int idx=0; idx<data_.size(); idx++ )
    {
	Data* data = data_[idx];
	const PluginInfo* piinf = data->info_;
	if ( piinf && piinf->dispname && !strcmp(piinf->dispname,nm) )
	    return data;
    }
    return 0;
}


const char* PluginManager::getFileName( const PluginManager::Data& data ) const
{
    static BufferString ret;
    if ( data.autosource_ == Data::None )
	ret = data.name_;
    else
    {
	FilePath fp( data.autosource_ == Data::AppDir ?
		     applibdir_ : userlibdir_ );
	fp.add( "libs" );
	fp.add( data.name_ );
	ret = fp.fullPath();
    }
    return ret.buf();
}


PluginManager::Data* PluginManager::fndData( const char* nm ) const
{
    for ( int idx=0; idx<data_.size(); idx++ )
    {
	const Data* data = data_[idx];
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
    {
	FilePath fp( nm );
	return getFnName( fp.fileName(), "", "" );
    }

    return piinf->dispname;
}


static void* getLibHandle( const char* lnm )
{
    if ( !lnm || !*lnm  )
	return 0;

    void* ret = 0;

#ifdef __win__

    BufferString targetlibnm( lnm );
    if ( File_isLink(lnm) )
	targetlibnm = File_linkTarget(lnm);

    if ( File_exists(targetlibnm) )
    {
	ret = LoadLibrary( targetlibnm );

#else

    if ( File_exists(lnm) )
    {
	ret = dlopen( lnm, RTLD_GLOBAL | RTLD_NOW );

#endif
	if ( !ret )
	    std::cerr << dlerror() << std::endl;
    }


    if( DBG::isOn(DBG_SETTINGS) )
    {
	BufferString msg( "Attempt to get handle for plugin " );
	msg += lnm; msg += ret ? " succeeded" : " failed";
	DBG::message( msg );
    }

    return ret;
}


static void closeLibHandle( void*& handle )
{
    if ( !handle ) return;
#ifdef __win__
    FreeLibrary( handle );
#else
    dlclose( handle );
#endif
    handle = 0;
}


static PluginInfo* mkEmptyInfo()
{
    PluginInfo* piinf = new PluginInfo;
    piinf = new PluginInfo;
    piinf->dispname = sKeyNoDispName;
    piinf->creator = piinf->version = "";
    piinf->text = "No info available";
    return piinf;
}

#ifdef __win__
# define mFnGettter GetProcAddress
#else
# define mFnGettter dlsym
#endif

#define mGetFn(typ,fn,handle,nm1,nm2,libnm) \
	typ fn = (typ)mFnGettter( handle, getFnName(libnm,nm1,nm2) )


static PluginInfo* getPluginInfo( Handletype handle, const char* libnm )
{
    mGetFn(PluginInfoRetFn,fn,handle,"Get","PluginInfo",libnm);
    return fn ? (*fn)() : mkEmptyInfo();
}


static int getPluginType( Handletype handle, const char* libnm )
{
    mGetFn(VoidIntRetFn,fn,handle,"Get","PluginType",libnm);
    return fn ? (*fn)() : PI_AUTO_INIT_LATE;
}


void PluginManager::openALOEntries()
{
    for ( int idx=0; idx<data_.size(); idx++ )
    {
	Data& data = *data_[idx];
	if ( data.autosource_ == Data::None )
	    continue;
	data.handle_ = getLibHandle( getFileName(data) );
	if ( !data.handle_ )
	{
	    if ( data.autosource_ != Data::Both )
		continue;

	    data.autosource_ = Data::AppDir;
	    data.handle_ = getLibHandle( getFileName(data) );
	    if ( !data.handle_ )
		continue;
	}

	data.autotype_ = getPluginType( data.handle_, data.name_ );
	data.info_ = getPluginInfo( data.handle_, data.name_ );
    }
}


void PluginManager::getALOEntries( const char* dirnm, bool usrdir )
{
    DirList dl( dirnm, DirList::FilesOnly );
    dl.sort();
    const BufferString prognm = getProgNm( argv_[0] );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	BufferString fnm = dl.get(idx);
	if ( !isALO(fnm) ) continue;

	*strchr( fnm.buf(), '.' ) = '\0';
	if ( fnm != prognm ) continue;

	FilePath fp( dirnm ); fp.add( dl.get(idx) );
	StreamData sd = StreamProvider( fp.fullPath() ).makeIStream(false);
	if ( !sd.usable() ) { sd.close(); continue; }

	char buf[128];
	while ( *sd.istrm )
	{
	    sd.istrm->getline( buf, 128 );
	    if ( buf[0] == '\0' ) continue;

#ifdef __win__
	    BufferString libnm = buf; libnm += ".dll";
#else
# ifdef __mac__
	    BufferString libnm = "lib"; libnm += buf; libnm += ".dylib";
# else
	    BufferString libnm = "lib"; libnm += buf; libnm += ".so";
# endif
#endif

	    Data* data = findData( libnm );
	    if ( !data )
	    {
		data = new Data( libnm );
		data->autosource_ = usrdir ? Data::UserDir : Data::AppDir;
		data_ += data;
	    }
	    else if ( usrdir != Data::isUserDir(data->autosource_) )
		data->autosource_ = Data::Both;
	}

	sd.close();
    }
}


void PluginManager::mkALOList()
{
    getALOEntries( userlibdir_, true );
    getALOEntries( applibdir_, false );
    openALOEntries();
}


static bool loadPlugin( Handletype handle, int argc, char** argv,
       			const char* libnm )
{
    mGetFn(ArgcArgvCCRetFn,fn,handle,"Init","Plugin",libnm);
    if ( !fn )
    {
	const BufferString libnmonly = FilePath(libnm).fileName();
	std::cerr << "Cannot find "
		  << getFnName(libnmonly,"Init","Plugin")
		  << " function in " << libnm << std::endl;
	return false;
    }

    const char* ret = (*fn)( argc, argv );
    if ( ret )
    {
	const BufferString libnmonly = FilePath(libnm).fileName();
	std::cerr << "Error loading " << libnm << ":\n" << ret << std::endl;
	return false;
    }

    return true;
}


bool PluginManager::load( const char* libnm )
{
    Data* data = new Data( libnm );
    data->handle_ = getLibHandle( libnm );
    if ( !data->handle_ )
	{ delete data; return false; }

    FilePath fp( libnm );
    const BufferString libnmonly( fp.fileName() );
    if ( !loadPlugin(data->handle_,argc_,argv_,libnmonly) )
	{ closeLibHandle(data->handle_); delete data; return false; }

    data->info_ = getPluginInfo( data->handle_, libnmonly );
    data_ += data;
    return true;
}


void PluginManager::loadAuto( bool late )
{
    for ( int idx=0; idx<data_.size(); idx++ )
    {
	Data& data = *data_[idx];
	if ( !data.handle_ )
	    continue;

	const int pitype = late ? PI_AUTO_INIT_LATE : PI_AUTO_INIT_EARLY;
	if ( data.autotype_ != pitype )
	    continue;

	if ( !loadPlugin(data.handle_,argc_,argv_,data.name_) )
	{
	    data.info_ = 0;
	    closeLibHandle(data.handle_);
	}

	static bool shw_load = GetEnvVarYN( "OD_SHOW_PLUGIN_LOAD" );
	if ( shw_load )
	    std::cerr << "Successfully loaded plugin '"
		      << userName(data.name_) << "'" << std::endl;
    }
}
