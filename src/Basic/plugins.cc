/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID = "$Id: plugins.cc,v 1.31 2004-04-27 15:51:15 bert Exp $";

#include "plugins.h"
#include "filepath.h"
#include "filegen.h"
#include "dirlist.h"
#include "strmprov.h"

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

};

static const char* getHDir()
{
#ifdef __win__
    return "win";
#else
    const char* ret = getenv( "binsubdir" );
    return ret ? ret : getenv( "HDIR" );
#endif
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


#ifdef __win__
# define mNotLoadedRet(act) \
	{ if ( handle ) FreeLibrary(handle); return false; }
# define mFnGettter GetProcAddress
#else
# define mNotLoadedRet(act) \
	{ act; if ( handle ) dlclose(handle); return false; }
# define mFnGettter dlsym
#endif

#define mGetFn(typ,fn,nm1,nm2) \
	typ fn = (typ)mFnGettter( handle, getFnName(libnmonly,nm1,nm2) )


static bool loadPlugin( const char* lnm, int argc, char** argv,
			int inittype )
{
    if ( !lnm || !*lnm  || inittype == PI_AUTO_INIT_NONE )
	return false;

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
	return false;
    HMODULE handle = LoadLibrary( targetlibnm );

#else

    if ( !File_exists(libnm) )
	return false;
    void* handle = dlopen( libnm, RTLD_GLOBAL | RTLD_NOW );

#endif

    if ( !handle )
	mNotLoadedRet( std::cerr << dlerror() << std::endl )

    const BufferString libnmonly = FilePath(libnm).fileName();
    if ( inittype > 0 )
    {
	mGetFn(VoidIntRetFn,fn,"Get","PluginType");
	if ( !fn || inittype != (*fn)() )
	    mNotLoadedRet(;); // not an error: just not auto or not now
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
	piinf->dispname = "<NoName>"; piinf->creator = piinf->version = "";
	piinf->text = "No info available";
    }

    PIM().addLoaded( libnm, piinf );

    if ( getenv( "OD_SHOW_PLUGIN_LOAD" ) )
	std::cerr << "Successfully loaded plugin " << libnm << std::endl;

    return true;
}


extern "C" bool LoadPlugin( const char* libnm, int argc, char** argv )
{
    return loadPlugin( libnm, argc, argv, -1 );
}


static void loadALOPlugins( const char* dnm, const char* fnm,
			    int argc, char** argv, int inittype )
{
    FilePath fp( dnm ); fp.add( fnm );
    StreamData sd = StreamProvider( fp.fullPath() ).makeIStream();
    if ( !sd.usable() ) return;

    char buf[128];
    fp.setFileName( "libs" ); fp.add( "X" );
    while ( *sd.istrm )
    {
	sd.istrm->getline( buf, 128 );
#ifdef __win__
	BufferString libnm = buf; libnm += ".dll";
#else
	BufferString libnm = "lib"; libnm += buf; libnm += ".so";
#endif
	if ( !PIM().isLoaded(libnm) )
	{
	    fp.setFileName( libnm );
	    loadPlugin( fp.fullPath(), argc, argv, inittype );
	}
    }

    sd.close();
}


static bool isALO( const char* fnm )
{
    const char* extptr = strrchr( fnm, '.' );
    if ( !extptr || !*extptr ) return false;
    return caseInsensitiveEqual( extptr+1, "alo", 0 );
}


static void loadPluginDir( const char* dirnm, int argc, char** argv,
			 int inittype )
{
    DirList dl( dirnm, DirList::FilesOnly );
    dl.sort();
    BufferString prognm = getProgNm( argv[0] );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	BufferString libnm = dl.get(idx);
	if ( isALO(libnm) )
	{
	    *strchr( libnm.buf(), '.' ) = '\0';
	    if ( libnm == prognm )
		loadALOPlugins( dirnm, dl.get(idx), argc, argv, inittype );
	}
    }
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const BufferString& libnm = dl.get(idx);
	if ( !isALO(libnm) && !PIM().isLoaded(libnm) )
	{
	    FilePath fp( dirnm ); fp.add( libnm );
	    loadPlugin( fp.fullPath(), argc, argv, inittype );
	}
    }
}


static void autoLoadPlugins( const char* dirnm,
			     int argc, char** argv, int inittype )
{
#define mTryLoadDbgMsg(dir) \
    if ( DBG::isOn(DBG_SETTINGS) ) \
    { \
	BufferString msg( "Attempt load plugins from: " ); msg += dir; \
	DBG::message( msg ); \
    }

    if ( !File_isDirectory(dirnm) )
	return;

    mTryLoadDbgMsg( dirnm );
    loadPluginDir( dirnm, argc, argv, inittype );

    FilePath fp( dirnm ); fp.add( getProgNm( argv[0] ) );
    BufferString specdirnm = fp.fullPath();
    if ( !File_isDirectory(specdirnm) )
	return;

    mTryLoadDbgMsg( specdirnm );
    loadPluginDir( specdirnm, argc, argv, inittype );
}


static const char* getDefDir( bool instdir )
{
    static BufferString dnm;
    FilePath fp(instdir ? GetSoftwareDir() : GetSettingsDir());
    fp.add(plugindir).add( getHDir() );
    dnm = fp.fullPath();

    if( DBG::isOn(DBG_SETTINGS) )
    {
        BufferString msg( "plugins.cc - getDefDir(" );
        msg += instdir ? "inst" : "usr";
        msg += ") -> "; msg += dnm;
	DBG::message( msg );
    }

    return dnm.buf();
}


extern "C" void LoadAutoPlugins( int argc, char** argv, int inittype )
{
    autoLoadPlugins( getDefDir(false), argc, argv, inittype );
    autoLoadPlugins( getDefDir(true), argc, argv, inittype );
}


static char* errargv[] = { "<not set>", 0 };

PluginManager::PluginManager()
	: argc_(1)
    	, argv_(errargv)
{
}


void PluginManager::getUsrNm( const char* libnm, BufferString& nm ) const
{
    const BufferString libnmonly = FilePath(libnm).fileName();
    nm = getFnName( libnmonly, "", "" );
}


const char* PluginManager::defDir( bool instdir ) const
{
    return getDefDir( instdir );
}


const char* PluginManager::getFullName( const char* nm ) const
{
    BufferString curnm;
    for ( int idx=0; idx<loaded_.size(); idx++ )
    {
	const BufferString& lnm = *loaded_[idx];
	if ( lnm == nm ) return lnm.buf();
	curnm = FilePath(lnm).fileName();
	if ( curnm == nm ) return lnm.buf();
	getUsrNm( lnm, curnm );
	if ( curnm == nm ) return lnm.buf();
    }

    return 0;
}


bool PluginManager::isLoaded( const char* nm )
{
    return getFullName(nm) ? true : false;
}


const char* PluginManager::userName( const char* libnm ) const
{
    static BufferString ret;
    getUsrNm( libnm, ret );
    return ret.buf();
}


const PluginInfo& PluginManager::getInfo( const char* nm ) const
{
    static PluginInfo notloadedinfo = { "Unknown", "Not loaded", "", "" };

    const char* fullnm = getFullName( nm );
    if ( !fullnm ) return notloadedinfo;

    int idx = indexOf( loaded_, fullnm );
    if ( idx<0 ) return notloadedinfo; // Huh?

    return *info_[idx];
}
