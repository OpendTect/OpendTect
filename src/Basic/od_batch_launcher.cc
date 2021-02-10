/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman K Singh
 * DATE     : March 2014
 * FUNCTION : Launches BatchProgram and the ProgressViewer on Linux
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "prog.h"

#include "commandlineparser.h"
#include "filepath.h"
#include "oddirs.h"

#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

const char* sODProgressViewerProgName = "od_ProgressViewer";
const char* sKeyNeedsMonitor = "needmonitor";
const char* sKeyMonitorFileNm = "monitorfnm";

#define mErrExit(s) { std::cerr << argv[1] << ": " << s \
			<< std::endl; return 1; }

int mProgMainFnName( int argc, char** argv )
{
    SetProgramArgs( argc, argv );
    CommandLineParser clp( argc, argv );
    BufferString redirectstr( " > " );
    BufferString monitorfnm;
    const bool needmonitor = clp.hasKey( sKeyNeedsMonitor );
    bool redirectoutput = false;
    if ( needmonitor )
    {
	clp.setKeyHasValue( sKeyMonitorFileNm );
	clp.getVal( sKeyMonitorFileNm, monitorfnm );
	if ( monitorfnm.isEmpty() )
	{
	    monitorfnm = FilePath::getTempFullPath("batch","txt");
	    redirectoutput = true;
	}
    }

    const int pid = fork();
    if ( pid < 0 )
	mErrExit( "Failed to fork process" )
    else if ( pid == 0 )	// Child process
    {
	chdir( GetExecPlfDir() );
	char** childargv = new char*[argc];
	int idx = 0;
	for ( ; idx<argc-1; idx++ )
	{
	    if ( FixedString(argv[idx+1]) == "--needmonitor" )
		break; // These arguments are not for the batch process.

	    childargv[idx] = argv[idx+1];
	}

	if ( redirectoutput )
	{
	    const int fd = open( monitorfnm.buf(), O_RDWR | O_CREAT, 0755 );
	    if ( fd < 0 )
		mErrExit( "Failed to open the log file" );
	    if ( dup2(fd,STDOUT_FILENO) < 0 )
		mErrExit( "Failed to redirect output to log file" );
	    close( fd );
	}

	childargv[idx] = 0;
	execv( argv[1], childargv );

	// We should not reach here if all goes well.
	mErrExit( "Failed to launch process " )
    }
    else if ( needmonitor )	// Parent process
    {
	BufferString progviewercmd( "\"",
		FilePath(GetExecPlfDir(),sODProgressViewerProgName).fullPath());
	progviewercmd.add( "\" --inpfile \"" ).add( monitorfnm )
		     .add( "\" --pid " ).add( pid ).add( " &" );
	if ( system(progviewercmd.buf()) )
	    mErrExit( "Failed to launch progress viewer" )
    }

    return 0;
}
