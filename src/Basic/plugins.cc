/*+
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID = "$Id: plugins.cc,v 1.19 2003-10-28 12:15:22 arend Exp $";

#include "plugins.h"
#include "filegen.h"
#include "dirlist.h"

#ifdef __win__
#include <Windows.h>
#else
#include <dlfcn.h>
#endif
#include <iostream>

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


static const char* getFnName( const char* libnm, const char* fnend )
{
    static BufferString ret;

    if ( (*libnm     == 'l' || *libnm     == 'L')
      && (*(libnm+1) == 'i' || *(libnm+1) == 'I')
      && (*(libnm+2) == 'b' || *(libnm+2) == 'B') )
	libnm += 3;
    ret = libnm;
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

#define mGetFn(typ,fn,nm) \
	typ fn = (typ)mFnGettter( handle, getFnName(libnmonly,nm) )


static bool loadPlugin( const char* libnm, int argc, char** argv,
			int inittype )
{
    if ( inittype == PI_AUTO_INIT_NONE ) return false;

#ifdef __win__
    HMODULE handle = LoadLibrary( libnm );
#else
    void* handle = dlopen( libnm, RTLD_GLOBAL | RTLD_NOW );
#endif

    if ( !handle )
	mNotLoadedRet( cerr << dlerror() << endl )

    const BufferString libnmonly = File_getFileName(libnm);
    if ( inittype > 0 )
    {
	mGetFn(VoidIntRetFn,fn,"GetPluginType");
	if ( !fn || inittype != (*fn)() )
	    mNotLoadedRet(;); // not an error: just not auto or not now
    }

    mGetFn(ArgcArgvCCRetFn,fn2,"InitPlugin");
    if ( !fn2 )
	mNotLoadedRet( cerr << "Cannot find "
			    << getFnName(libnmonly,"InitPlugin")
			    << " function in " << libnm << endl )

    const char* ret = (*fn2)(argc,argv);
    if ( ret )
	mNotLoadedRet( cerr << libnm << ": " << ret << endl )

    mGetFn(PluginInfoRetFn,fn3,"GetPluginInfo");
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
	cerr << "Successfully loaded plugin " << libnm << endl;

    return true;
}


extern "C" bool LoadPlugin( const char* libnm, int argc, char** argv )
{
    return loadPlugin(libnm,argc,argv,-1);
}


static void loadPlugins( const char* dirnm, int argc, char** argv,
			 int inittype )
{
    DirList dl( dirnm, -1 );
    dl.sort();
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	BufferString libnm = dl.get(idx);
	if ( File_isDirectory(libnm) || PIM().isLoaded(libnm) )
	    continue;

	BufferString fulllibnm = File_getFullPath( dirnm, libnm );
	loadPlugin( fulllibnm, argc, argv, inittype );
    }
}


static void loadPIs( const char* dirnm, int argc, char** argv, int inittype )
{
    if ( !File_exists(dirnm) )
	return;

    loadPlugins( dirnm, argc, argv, inittype );
    const BufferString prognm( File_getFileName(argv[0]) );
    BufferString specdirnm = File_getFullPath( dirnm, prognm );
    loadPlugins( specdirnm, argc, argv, inittype );
}


static const char* getDefDir( bool instdir )
{
    static BufferString dnm;
    if ( instdir )
    {
	dnm = GetSoftwareDir();
	dnm = File_getFullPath( dnm, plugindir );
    }
    else
    {
	dnm = GetSettingsDir();
	dnm = File_getFullPath( dnm, plugindir );
    }

    dnm = File_getFullPath( dnm, getHDir() );
    return dnm.buf();
}


extern "C" void LoadAutoPlugins( int argc, char** argv, int inittype )
{
    loadPIs( getDefDir(false), argc, argv, inittype );
    loadPIs( getDefDir(true), argc, argv, inittype );
}


static char* errargv[] = { "<not set>", 0 };

PluginManager::PluginManager()
	: argc_(1)
    	, argv_(errargv)
{
}


void PluginManager::getUsrNm( const char* libnm, BufferString& nm ) const
{
    const BufferString libnmonly = File_getFileName(libnm);
    nm = getFnName( libnmonly, "" );
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
	curnm = File_getFileName(lnm);
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
