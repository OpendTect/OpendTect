/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Arnaud
 Date:		July 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "batchprog.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "hostdata.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "jobcommunic.h"
#include "keystrs.h"
#include "moddepmgr.h"
#include "progressmeter.h"
#include "ptrman.h"
#include "seis2dto3d.h"
#include "seisjobexecprov.h"
#include "seismod.h"
#include "thread.h"

#include <iostream>


#define mDestroyWorkers \
{ delete proc; proc = 0; }


#define mRetFileProb(fdesc,fnm,s) \
	{ \
	    BufferString msg(fdesc); \
	    msg += " ("; msg += fnm; msg += ") "; msg += s; \
	    mRetHostErr( msg ); \
	}

#define mStrmWithProcID(s) \
        strm << "\n[" << process_id << "]: " << s << "." << std::endl

#define mErrRet(msg) { strm << msg << std::endl; return false; }

bool BatchProgram::go( std::ostream& strm )
{
    strm << "Processing on " << HostData::localHostName()  << '.' << std::endl;

    OD::ModDeps().ensureLoaded( "Algo" );
    OD::ModDeps().ensureLoaded( "Seis" );


    const int process_id = GetPID();
    Seis2DTo3D* proc = 0;
    const FixedString tempdir = pars().find(sKey::TmpStor());
    if ( !tempdir.isEmpty() )
    {
	if ( !File::exists(tempdir) )
	    mRetFileProb(sKey::TmpStor(),tempdir,"does not exist")
	else if ( !File::isDirectory(tempdir) )
	    mRetFileProb(sKey::TmpStor(),tempdir,"is not a directory")
	else if ( !File::isWritable(tempdir) )
	    mRetFileProb(sKey::TmpStor(),tempdir,"is not writeable")
    }

    strm << "Preparing processing"; strm.flush();
    const char* seisid = pars().find( SeisJobExecProv::sKeySeisOutIDKey() );
    if ( !seisid )
	strm << " ..." << std::endl;
    else
    {
	PtrMan<IOObj> ioobj = IOM().get( seisid );
	if ( !ioobj )
	{
	    BufferString msg( "Cannot find output Seismic Object with ID '" );
	    msg += seisid; msg += "' ..."; mRetHostErr( msg );
	}

	FilePath fp( ioobj->fullUserExpr(false) );
	if ( !fp.isAbsolute() )
	{
	    fp.set( IOM().rootDir() );
	    fp.add( ioobj->dirName() );
	    fp.add( ioobj->fullUserExpr(false) );
	}

	BufferString dirnm = fp.pathOnly();
	ioobj->setDirName( dirnm.buf() );
	const bool isdir = File::isDirectory( dirnm );
	if ( !isdir || !File::isWritable(dirnm) )
	{
	    BufferString fdesc("Output directory for '");
	    fdesc += ioobj->name(); fdesc += "'";
	    mRetFileProb(fdesc,dirnm,
			 isdir ? "is not writeable" : "does not exist")
	}

	strm << " of '" << ioobj->name() << "'." << std::endl;
	strm.flush();
    }

    PtrMan<IOPar> paramspar = pars().subselect( sKey::Pars() );
    if ( !paramspar )
	mRetJobErr("Cannot find necessary information in parameter file")

    proc = new Seis2DTo3D;
    if ( !proc->init(pars()) )
	mRetJobErr("Invalid set of input parameters")

    mSetCommState(Working);

    double startup_wait = 0;
    pars().get( "Startup delay time", startup_wait );
    Threads::sleep( startup_wait );
    const double pause_sleep_time = GetEnvVarDVal( "OD_BATCH_SLEEP_TIME", 1 );
    TextStreamProgressMeter progressmeter(strm);
    bool loading = true;
    int nriter = 0, nrdone = 0;

    while ( true )
    {
	bool paused = false;

	if ( pauseRequested() )
	{
	    paused = true;
	    mSetCommState(Paused);
	    Threads::sleep( pause_sleep_time );
	}
	else
	{
	    if ( paused )
	    {
		paused = false;
		mSetCommState(Working);
	    }

	    const int res = proc->nextStep();

	    if ( nriter == 0 )
	    {
		strm << "\nEstimated number of inlines to be processed"
		    <<"(assuming regular input): "<< proc->totalNr()
		    << "\nLoading data ..." << std::endl;
		progressmeter.setTotalNr( proc->totalNr() );
	    }

	    if ( res > 0 )
	    {
		if ( loading )
		{
		    loading = false;
		    mStrmWithProcID( "Processing started" );
		}

		if ( comm_ && !comm_->updateProgress( nriter + 1 ) )
		    mRetHostErr( comm_->errMsg() )

		if ( proc->nrDone()>nrdone )
		{
		    nrdone++;
		    ++progressmeter;
		}
	    }
	    else
	    {
		if ( res == -1 )
		    mRetJobErr( BufferString("Cannot reach next inline",
				": ",proc->message()) )
		break;
	    }

	    if ( res >= 0 )
		nriter++;
	}
    }

    { mStrmWithProcID( "Processing done; Closing down" ); }

    // It is VERY important workers are destroyed BEFORE the last sendState!!!
    mDestroyWorkers
    progressmeter.setFinished();
    mStrmWithProcID( "Threads closed; Writing finish status" );

     if ( !comm_ ) return true;

    comm_->setState( JobCommunic::Finished );
    bool ret = comm_->sendState();

    if ( ret )
	mStrmWithProcID( "Successfully wrote finish status" );
    else
	mStrmWithProcID( "Could not write finish status" );
    return ret;
}
