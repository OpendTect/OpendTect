/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Arnaud
 Date:		August 2014
________________________________________________________________________

-*/

#include "batchprog.h"

#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "hostdata.h"
#include "dbman.h"
#include "ioobj.h"
#include "iopar.h"
#include "jobcommunic.h"
#include "keystrs.h"
#include "moddepmgr.h"
#include "progressmeterimpl.h"
#include "ptrman.h"
#include "seis2ddata.h"
#include "seis2dto3d.h"
#include "seisjobexecprov.h"
#include "seismod.h"
#include "separstr.h"
#include "factory.h"

#define mDestroyWorkers \
{ delete proc; proc = 0; }


#define mRetFileProb(fdesc,fnm,s) \
	{ \
	    BufferString msg(fdesc); \
	    msg += " ("; msg += fnm; msg += ") "; msg += s; \
	    mRetHostErr( msg ); \
	}

mLoad1Module("Seis")

bool BatchProgram::doWork( od_ostream& strm )
{
    const int odversion = pars().odVersion();
    if ( odversion < 600 )
    {
	errorMsg( ::toUiString("\nCannot execute pre-6.0 par files") );
	return false;
    }

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

    const FixedString selspec = pars().find( "Output.Subsel.In-line range" );
    if ( !selspec.isEmpty() )
    {
	FileMultiString fms( selspec );
	const int lnr = fms.getIValue( 0 );
	if ( lnr == fms.getIValue(1) )
	    strm << "Calculating for in-line " << lnr << '.' << od_newline;
    }
    strm << od_newline;

    strm << "Preparing processing";
    const char* seisid = pars().find( SeisJobExecProv::sKeySeisOutIDKey() );
    if ( !seisid )
	strm << " ..." << od_newline;
    else
    {
	PtrMan<IOObj> ioobj = DBKey(seisid).getIOObj();
	if ( !ioobj )
	{
	    BufferString msg( "Cannot find output Seismic Object with ID '" );
	    msg += seisid; msg += "' ..."; mRetHostErr( msg );
	}

	File::Path fp( ioobj->mainFileName() );
	if ( !fp.isAbsolute() )
	{
	    fp.set( DBM().survDir() );
	    fp.add( ioobj->dirName() );
	    fp.add( ioobj->mainFileName() );
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

	strm << " of '" << ioobj->name() << "'.\n" << od_endl;
    }

    PtrMan<IOPar> paramspar = pars().subselect( sKey::Pars() );
    if ( !paramspar )
	mRetJobErr("Cannot find necessary information in parameter file")

    TextStreamProgressMeter progressmeter(strm);
    LoggedTaskRunner taskr( strm );

	BufferString type;
	if (!paramspar->get(Seis2DTo3D::sKeyType(), type) || type.isEmpty())
		mRetJobErr("Cannot determine type of seismic"
					"interpolator from parameter file")

	proc = Seis2DTo3D::factory().create(type);

	if (!proc)
		mRetJobErr("Cannot create seismic interpolator"
		 ",perhaps not all plugins are loaded?")

	proc->setStream(strm);
	proc->setTaskRunner(&taskr);

    if ( !proc->init(pars()) )
	mRetJobErr("Invalid set of input parameters")

    strm << od_newline << "Reading Data";

    mSetCommState(Working);

    double startup_wait = 0;
    pars().get( "Startup delay time", startup_wait );
    sleepSeconds( startup_wait );

    const double pause_sleep_time = GetEnvVarDVal( "OD_BATCH_SLEEP_TIME", 1 );
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
		setResumed();
	    }

	    if ( nriter == 0 )
	    {
		strm << od_newline << "Number of components";
		strm << " to be processed: " << proc->totalNr() << od_newline;
		strm << "Calculating results ..." << od_endl;
		progressmeter.setTotalNr( proc->totalNr() );
	    }

	    const int res = proc->nextStep();

	    if ( res > 0 )
	    {
		strm << "Moving to next component" << od_endl;
		if ( comm_ && !comm_->updateProgress(nriter+1) )
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
		    mRetJobErr( BufferString("Cannot reach next position:\n",
				::toString(proc->message()) ) )
		break;
	    }

	    if ( res >= 0 )
		nriter++;
	}
    }

    bool closeok = true;

    if ( !closeok )
    { mMessage( "Could not close output data." ); }

    mDestroyWorkers

    PtrMan<IOObj> ioobj = DBKey(seisid).getIOObj();
    if ( ioobj )
    {
	File::Path fp( ioobj->mainFileName() );
	fp.setExtension( sProcFileExtension() );
	pars().write( fp.fullPath(), sKey::Pars() );
    }

    // It is VERY important workers are destroyed BEFORE the last sendState!!!
    progressmeter.setFinished();
    mMessage( "Threads closed; Writing finish status" );

    if ( !comm_ ) return true;

    comm_->setState( JobCommunic::Finished );
    bool ret = comm_->sendState();

    if ( ret )
	mMessage( "Successfully wrote finish status" );
    else
	mMessage( "Could not write finish status" );
    return ret;
}
