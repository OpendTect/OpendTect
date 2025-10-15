/*+
________________________________________________________________________

 Copyright:	(C) 1995-2025 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "tutmod.h"

#include "applicationdata.h"
#include "commandlineparser.h"
#include "filepath.h"
#include "iopar.h"
#include "moddepmgr.h"
#include "oddirs.h"
#include "plugins.h"
#include "prog.h"
#include "timer.h"


static const char* argkey = "arg";

static void printBatchUsage()
{
    od_ostream& strm = logStream();
    strm << "Usage: " << "tut_simple_prog [-h] "
			 "[--arg ARG]\n\n";
    strm << "Simple tutorial non-gui application with an optional argument\n\n";
    strm << "optional arguments:\n";
    strm << "  -h, --help\t\tshow this help message and exit\n\n";
    strm << "  --" << argkey << " ARG\t\tSome argument (default: empty)\n";
    strm << od_endl;
}

class Worker : public CallBacker
{
public:
    Worker( const char* argstr )
	: argstr_(argstr)
    {
	mAttachCB( timer_.tick, Worker::timerTickCB );
	timer_.start( 1000, false );
    }

    ~Worker()
    {
	detachAllNotifiers();
	CallBack::removeFromThreadCalls( this );
    }

private:

    void timerTickCB( CallBacker* )
    {
	exitstatus_ = doWork() ? 0 : 1;
	CallBack::addToMainThread( mCB(this,Worker,closeApp) );
    }

    bool doWork()
    {
	return true;
    }

    void closeApp( CallBacker* )
    {
	//Cleanup all class members here
	ApplicationData::exit( exitstatus_ );
    }

    const BufferString	argstr_;
    Timer		timer_;
    int			exitstatus_	= -1;

};


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


int mProgMainFnName( int argc, char** argv )
{
    const bool needprojectdata = true;
    SetProgramArgs( argc, argv, needprojectdata );
    mInitProg( OD::RunCtxt::BatchProgCtxt );
    ApplicationData app;

    OD::ModDeps().ensureLoaded( "Network" );
    if ( needprojectdata )
	OD::ModDeps().ensureLoaded( "Geometry" );

    //Execute simple work, like parsing arguments
    const CommandLineParser parser( argc, argv );
    if ( parser.hasKey("help") || parser.hasKey("h") )
    {
	printBatchUsage();
	return 1;
    }

    parser.setKeyHasValue( argkey );
    BufferString argstr;
    const bool mUnusedVar hasarg = parser.getVal( argkey, argstr );

    /* Plugins from this (Tutorial) project are loaded from the alo files
       using PIM().loadAuto, plugins from the OpendTect SDK must be
       loaded explicitely using a function like loadOpendTectPlugins */
    PIM().loadAuto( false );
    loadOpendTectPlugins( "ODHDF5" );
    OD::ModDeps().ensureLoaded( "VolumeProcessing" );
			     //Highest dependency of this (Tut) module
    Worker worker( argstr.buf() );
    // PIM().loadAuto( true ); Typically not required for non-gui programs

    const int ret = app.exec();
    return ret;
}
