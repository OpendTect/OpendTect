/*+
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID = "$Id: plugins.cc,v 1.8 2003-10-03 15:22:03 bert Exp $";

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
static const char* plugindir = ".odplugins";


extern "C" {

    typedef int (*VoidIntRetFn)(void);
    typedef const char* (*ArgcArgvCCRetFn)(int*,char**);

};

static const char* getHDir()
{
    const char* ret = getenv( "binsubdir" );
    return ret ? ret : getenv( "HDIR" );
}


static const char* getFnName( const char* libnm, const char* fnend )
{
    static BufferString ret;

    if ( *libnm == 'l' && *(libnm+1) == 'i' && *(libnm+2) == 'b' )
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
#else
# define mNotLoadedRet(act) \
	{ act; if ( handle ) dlclose(handle); return false; }
#endif

static bool loadPlugin( const char* libnm, int* pargc, char** argv,
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
#ifdef __win__
	VoidIntRetFn fn = reinterpret_cast <VoidIntRetFn>
	    (GetProcAddress (handle, getFnName(libnmonly,"GetPluginType")) );
#else
	VoidIntRetFn fn = (VoidIntRetFn)dlsym( handle,
				getFnName(libnmonly,"GetPluginType") );
#endif
	if ( !fn || inittype != (*fn)() )
	    mNotLoadedRet(;); // not an error: just not the right time to load
    }

#ifdef __win__
    ArgcArgvCCRetFn fn2 = reinterpret_cast<ArgcArgvCCRetFn>
		( GetProcAddress( handle, getFnName(libnmonly,"InitPlugin") ) );
#else
    ArgcArgvCCRetFn fn2 = (ArgcArgvCCRetFn)dlsym( handle,
				getFnName(libnmonly,"InitPlugin") );
#endif
    if ( !fn2 )
	mNotLoadedRet( cerr << "Cannot find "
			    << getFnName(libnmonly,"InitPlugin")
			    << " function in " << libnm << endl )

    const char* ret = (*fn2)(pargc,argv);
    if ( ret )
	mNotLoadedRet( cerr << libnm << ": " << ret << endl )

    PIM().addLoaded( libnm );
    if ( getenv( "OD_SHOW_PLUGIN_LOAD" ) )
	cerr << "Successfully loaded plugin " << libnm << endl;

    return true;
}


extern "C" bool LoadPlugin( const char* libnm, int* pargc, char** argv )
{
    return loadPlugin(libnm,pargc,argv,-1);
}


static void loadPlugins( const char* dirnm, int* pargc, char** argv,
			 int inittype )
{
    DirList dl( dirnm, -1 );
    dl.sort();
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	BufferString libnm = dl[idx]->name();
	if ( PIM().isLoaded(libnm) )
	    continue;

	BufferString fulllibnm = File_getFullPath( dirnm, libnm );
	loadPlugin( fulllibnm, pargc, argv, inittype );
    }
}


static void loadPIs( const char* dnm, int* pargc, char** argv, int inittype )
{
    BufferString dirnm = File_getFullPath( dnm, getHDir() );
    if ( !File_exists(dirnm) )
	return;

    const BufferString prognm( File_getFileName(argv[0]) );
    BufferString specdirnm = File_getFullPath( dirnm, prognm );
    loadPlugins( specdirnm, pargc, argv, inittype );
    loadPlugins( dirnm, pargc, argv, inittype );
}


extern "C" void LoadAutoPlugins( int* pargc, char** argv, int inittype )
{
    const BufferString homedir( GetHomeDir() );
    BufferString dirnm = File_getFullPath( homedir, plugindir );
    loadPIs( dirnm, pargc, argv, inittype );
    const BufferString swdir( GetSoftwareDir() );
    dirnm = File_getFullPath( swdir, plugindir );
    loadPIs( dirnm, pargc, argv, inittype );
}


void PluginManager::getUsrNm( const char* libnm, BufferString& nm ) const
{
    const BufferString libnmonly = File_getFileName(libnm);
    nm = getFnName( libnmonly, "" );
}


bool PluginManager::isLoaded( const char* nm )
{
    int idx = indexOf( loaded_, nm );
    if ( idx >= 0 ) return true;

    BufferString curnm;
    for ( idx=0; idx<loaded_.size(); idx++ )
    {
	const BufferString& lnm = *loaded_[idx];
	if ( lnm == nm ) return true;
	curnm = File_getFileName(lnm);
	if ( curnm == nm ) return true;
	getUsrNm( lnm, curnm );
	if ( curnm == nm ) return true;
    }
    return false;
}


const char* PluginManager::userName( const char* libnm ) const
{
    static BufferString ret;
    getUsrNm( libnm, ret );
    return ret.buf();
}
