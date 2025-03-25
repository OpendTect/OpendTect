/*+
________________________________________________________________________

 Copyright:	(C) 1995-2025 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "tutmod.h"

#include "batchprog.h"
#include "executor.h"
#include "filepath.h"
#include "iopar.h"
#include "moddepmgr.h"
#include "oddirs.h"
#include "plugins.h"


mLoad1Module("VolumeProcessing") //Highest dependency of this (Tut) plugin

static bool loadOpendTectPlugins( const char* piname )
{
    BufferString libnm; libnm.setMinBufSize( 32 );
    SharedLibAccess::getLibName( piname, libnm.getCStr(), libnm.bufSize() );
    const FilePath libfp( GetLibPlfDir(), libnm );
    return libfp.exists()
	? PIM().load( libfp.fullPath(), PluginManager::Data::AppDir,
		      PI_AUTO_INIT_EARLY )
	: false;
}


bool BatchProgram::doWork( od_ostream& strm )
{
    /* Plugins from this (Tutorial) project are loaded from the alo files,
       plugins from the OpendTect SDK must be loaded explicitely */
    if ( !loadOpendTectPlugins("ODHDF5") )
	return false;

    const IOPar& mUnusedVar procpars = pars();
    TextTaskRunner mUnusedVar taskr( strm );

    return true;
}
