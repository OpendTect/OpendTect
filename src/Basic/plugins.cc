/*+
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID = "$Id: plugins.cc,v 1.2 2003-08-21 15:48:42 bert Exp $";

#include "plugins.h"
#include "filegen.h"
#include "dirlist.h"

#include <dlfcn.h>
#include <iostream>

static ObjectSet<BufferString> loaded;

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


static bool loadPlugin( const char* libnm, int* pargc, char** argv,
			int inittype )
{
    if ( inittype == PI_AUTO_INIT_NONE ) return false;

    void* handle = dlopen( libnm, RTLD_LAZY );
    if ( !handle )
    {
	cerr << dlerror() << endl;
	return false;
    }

    const BufferString libnmonly = File_getFileName(libnm);
    if ( inittype > 0 )
    {
	VoidIntRetFn fn = (VoidIntRetFn)dlsym( handle,
				getFnName(libnmonly,"GetPluginType") );
	if ( !fn || inittype != (*fn)() )
	    { dlclose(handle); return false; }
    }

    ArgcArgvCCRetFn fn2 = (ArgcArgvCCRetFn)dlsym( handle,
				getFnName(libnmonly,"InitPlugin") );
    bool rv = false;
    if ( !fn2 )
	cerr << "Cannot find InitPlugin() function in "
	     << libnm << endl;
    else
    {
	const char* ret = (*fn2)(pargc,argv);
	if ( ret )
	    cerr << libnm << ": " << ret << endl;
	else
	{
	    cerr << "Successfully loaded plugin "
		 << libnmonly << endl;
	    loaded += new BufferString( libnmonly );
	    rv = true;
	}
    }

    dlclose(handle);
    return rv;
}


bool LoadPlugin( const char* libnm, int* pargc, char** argv )
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
	if ( indexOf( loaded, libnm ) >= 0 )
	    continue;

	BufferString fulllibnm = File_getFullPath( dirnm, libnm );
	loadPlugin(fulllibnm,pargc,argv,inittype);
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
    const BufferString homedir( getenv("HOME") );
    BufferString dirnm = File_getFullPath( homedir, ".dgbPlugins" );
    loadPIs( dirnm, pargc, argv, inittype );
    const BufferString swdir( GetSoftwareDir() );
    dirnm = File_getFullPath( swdir, "plugins" );
    loadPIs( dirnm, pargc, argv, inittype );
}
