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
static const char* plugindir = "plugins";

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
	fp.add( plugindir );
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
	fp.add( plugindir );
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
	const Data& data = *data_[idx];
	if ( data.info_ && data.info_->dispname
		&& !strcmp(data.info_->dispname,nm) )
	    return &data;
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


bool PluginManager::isLoaded( const char* nm ) const
{
    const Data* data = findData( nm );
    return data && data->info_;
}


bool PluginManager::isPresent( const char* nm ) const
{
    return findData( nm );
}


const char* PluginManager::userName( const char* nm ) const
{
    FilePath fp( nm );
    return getFnName( fp.fileName(), "", "" );
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
	if ( !sd.usable() ) continue;

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
    }
}


void PluginManager::mkALOList()
{
    getALOEntries( userlibdir_, true );
    getALOEntries( applibdir_, false );
}


#ifdef __win__
# define mNotLoadedRet(act) \
	{ if ( handle ) FreeLibrary(handle); return 0; }
# define mFnGettter GetProcAddress
#else
# define mNotLoadedRet(act) \
	{ act; if ( handle ) dlclose(handle); return 0; }
# define mFnGettter dlsym
#endif

#define mGetFn(typ,fn,nm1,nm2) \
	typ fn = (typ)mFnGettter( handle, getFnName(libnmonly,nm1,nm2) )


static PluginInfo* loadPlugin( const char* lnm, int argc, char** argv,
       				bool checkinittype=false, bool late=true )
{
    if ( !lnm || !*lnm  )
	return 0;

    const BufferString libnm( lnm );
    if( DBG::isOn(DBG_SETTINGS) )
    {
	BufferString msg( "Attempting to load plugin " );
	msg += libnm;
	DBG::message( msg );
    }

#ifdef __win__

    BufferString targetlibnm( libnm );
    if ( File_isLink(libnm) )
	targetlibnm = File_linkTarget(libnm);
    if ( !File_exists(targetlibnm) )
	return 0;
    HMODULE handle = LoadLibrary( targetlibnm );

#else

    if ( !File_exists(libnm) )
	return 0;
    void* handle = dlopen( libnm, RTLD_GLOBAL | RTLD_NOW );

#endif

    if ( !handle )
	mNotLoadedRet( std::cerr << dlerror() << std::endl )

    const BufferString libnmonly = FilePath(libnm).fileName();
    if ( checkinittype )
    {
	mGetFn(VoidIntRetFn,fn,"Get","PluginType");
	if ( fn )
	{
	    int inittype = (*fn)();
	    if ( (late  && inittype != PI_AUTO_INIT_LATE)
	      || (!late && inittype != PI_AUTO_INIT_EARLY) )
		mNotLoadedRet(;) // not an error: just not auto or not now
	}
    }

    mGetFn(ArgcArgvCCRetFn,fn2,"Init","Plugin");
    if ( !fn2 )
	mNotLoadedRet( std::cerr << "Cannot find "
			    << getFnName(libnmonly,"Init","Plugin")
			    << " function in " << libnm << std::endl )

    const char* ret = (*fn2)(argc,argv);
    if ( ret )
	mNotLoadedRet( std::cerr << libnm << ": " << ret << std::endl )

    mGetFn(PluginInfoRetFn,fn3,"Get","PluginInfo");
    PluginInfo* piinf = 0;
    if ( fn3 )
	piinf = (*fn3)();
    if ( !piinf )
    {
	piinf = new PluginInfo;
	piinf->dispname = "??"; piinf->creator = piinf->version = "";
	piinf->text = "No info available";
    }

    static bool shw_load = GetEnvVarYN( "OD_SHOW_PLUGIN_LOAD" );
    if ( shw_load )
	std::cerr << "Successfully loaded plugin " << libnm << std::endl;

    return piinf;
}


bool PluginManager::load( const char* libnm )
{
    PluginInfo* piinf = loadPlugin( libnm, argc_, argv_ );
    if ( !piinf ) return false;

    Data* data = new Data( libnm );
    data->info_ = piinf;
    data_ += data;
    return true;
}


bool PluginManager::loadAutoLib( Data& data, bool usr, bool late )
{
    PluginInfo* piinf = loadPlugin( getFileName(data), argc_, argv_,
	    			    true, late );
    if ( !piinf )
	return false;

    data.autosource_ = usr ? Data::UserDir : Data::AppDir;
    data.info_ = piinf;
    return true;
}


void PluginManager::loadAuto( bool late )
{
    for ( int idx=0; idx<data_.size(); idx++ )
    {
	Data& data = *data_[idx];
	if ( data.autosource_ == Data::None )
	    continue;

	if ( data.autosource_ != Data::AppDir )
	{
	    if ( loadAutoLib(data,true,late) )
		continue;
	}

	if ( data.autosource_ != Data::UserDir )
	    loadAutoLib( data, false, late );
    }
}
