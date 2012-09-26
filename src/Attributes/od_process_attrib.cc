/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Mar 2000
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "batchprog.h"

#include "attribdescset.h"
#include "attribdescsettr.h"
#include "attribengman.h"
#include "attriboutput.h"
#include "attribprocessor.h"
#include "attribstorprovider.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "hostdata.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "progressmeter.h"
#include "ptrman.h"
#include "seisjobexecprov.h"
#include "seis2dline.h"
#include "separstr.h"
#include "thread.h"
#include "jobcommunic.h"
#include "moddepmgr.h"


#define mDestroyWorkers \
	{ delete proc; proc = 0; }

defineTranslatorGroup(AttribDescSet,"Attribute definitions");


#define mRetError(s) \
{ errorMsg(s); mDestroyWorkers; return false; }

#define mRetHostErr(s) \
	{  \
	    if ( comm ) comm->setState( JobCommunic::HostError ); \
	    mRetError(s) \
	}

#define mRetJobErr(s) \
	{  \
	    if ( comm ) comm->setState( JobCommunic::JobError ); \
	    mRetError(s) \
	}

#define mRetFileProb(fdesc,fnm,s) \
	{ \
	    BufferString msg(fdesc); \
	    msg += " ("; msg += fnm; msg += ") "; msg += s; \
	    mRetHostErr( msg ); \
	}

#define mStrmWithProcID(s) \
    strm << "\n[" << process_id << "]: " << s << "." << std::endl

#define mSetCommState(State) \
	if ( comm ) \
	{ \
	    comm->setState( JobCommunic::State ); \
	    if ( !comm->updateState() ) \
		mRetHostErr( comm->errMsg() ) \
	}

bool BatchProgram::go( std::ostream& strm )
{
    strm << "Processing on " << HostData::localHostName()  << '.' << std::endl;

    float vsn = 0;
    if ( !parversion_.isEmpty() )
    {
	vsn = toFloat( parversion_.buf() );
	if ( vsn < 3.2 )
	    { errorMsg("\nCannot execute pre-3.2 par files"); return false; }
    }

    OD::ModDeps().ensureLoaded( "PreStackProcessing" );
    OD::ModDeps().ensureLoaded( "Attributes" );
    
    const int process_id = GetPID();
    Attrib::Processor* proc = 0;
    const char* tempdir = pars().find(sKey::TmpStor());
    if ( tempdir && *tempdir )
    {
	if ( !File::exists(tempdir) )
	    mRetFileProb(sKey::TmpStor(),tempdir,"does not exist")
	else if ( !File::isDirectory(tempdir) )
	    mRetFileProb(sKey::TmpStor(),tempdir,"is not a directory")
	else if ( !File::isWritable(tempdir) )
	    mRetFileProb(sKey::TmpStor(),tempdir,"is not writeable")
    }

    Seis2DLineSet::installPreSet( pars(), SeisJobExecProv::sKeyOutputLS(),
	    			  SeisJobExecProv::sKeyWorkLS() );

    const char* selspec = pars().find( "Output.1.In-line range" );
    if ( selspec && *selspec )
    {
	FileMultiString fms( selspec );
	int lnr = toInt( fms[0] );
	if ( lnr == toInt( fms[1] ) )
	    strm << "Calculating for in-line " << lnr << '.' << std::endl;
    }
    strm << std::endl;

    strm << "Preparing processing"; strm.flush();
    const char* seisid = pars().find( "Output.0.Seismic.ID" );
    if ( !seisid )
	seisid = pars().find( "Output.1.Seismic ID" );

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

    Attrib::DescSet attribset( false );
    const char* setid = pars().find("Attribute Set");
    if ( setid && *setid )
    {
	PtrMan<IOObj> ioobj = IOM().get( setid );
	if ( !ioobj )
	    mRetHostErr( "Cannot find provided attrib set ID" )
	BufferString msg;
	if ( !AttribDescSetTranslator::retrieve(attribset,ioobj,msg) )
	    mRetJobErr( msg );
    }
    else
    {
	PtrMan<IOPar> attribs = pars().subselect("Attributes");
	if ( !attribset.usePar( *attribs, vsn ) )
	    mRetJobErr( attribset.errMsg() )
    }

    PtrMan<IOPar> outputs = pars().subselect("Output");
    if ( !outputs )
	mRetJobErr( "No outputs found" )

    PtrMan<Attrib::EngineMan> attrengman = new Attrib::EngineMan();
    int indexoutp = 0; BufferString linename;
    while ( true )
    {
        BufferString multoutpstr = IOPar::compKey( "Output", indexoutp );
        PtrMan<IOPar> output = pars().subselect( multoutpstr );
        if ( !output )
        {
            if ( !indexoutp )
		{ indexoutp++; continue; }
	    else
	        break;
	}
	linename = output->find(sKey::LineKey());
	indexoutp++;
    }
    BufferString errmsg;
    proc = attrengman->usePar( pars(), attribset, linename, errmsg );
    if ( !proc )
	mRetJobErr( errmsg );

    mSetCommState(Working);

    double startup_wait = 0;
    pars().get( "Startup delay time", startup_wait );
    Threads::sleep( startup_wait );

    const char* attrtypstr = pars().find( "Attributes.Type" );
    const bool is2d = attrtypstr && *attrtypstr == '2';
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

	    if ( nriter == 0 && !is2d )
	    {
		strm << "\nEstimated number of positions to be processed"
		     <<"(assuming regular input): "<< proc->totalNr()
		     << "\nLoading cube data ..." << std::endl;
		progressmeter.setTotalNr( proc->totalNr() );
	    }

	    if ( res > 0 )
	    {
		if ( loading )
		{
		    loading = false;
		    mStrmWithProcID( "Processing started" );
		}

		if ( comm && !comm->updateProgress( nriter + 1 ) )
		    mRetHostErr( comm->errMsg() )

		if ( proc->nrDone()>nrdone )
		{
		    nrdone++;
		    ++progressmeter;
		}
	    }
	    else
	    {
		if ( res == -1 )
		    mRetJobErr( BufferString("Cannot reach next position",
					     ": ",proc->message()) )
		break;
	    }
	    
	    if ( res >= 0 )
	    {
		nriter++;
		proc->outputs_[0]->writeTrc();
	    }
	}
    }

    bool closeok = true;
    if ( nriter )
	closeok = proc->outputs_[0]->finishWrite();

    if ( !closeok )
    { mStrmWithProcID( "Could not close output data." ); }
    else
    { mStrmWithProcID( "Processing done; Closing down" ); }

    // It is VERY important workers are destroyed BEFORE the last sendState!!!
    mDestroyWorkers
    progressmeter.setFinished();
    mStrmWithProcID( "Threads closed; Writing finish status" );

    if ( !comm ) return true;

    comm->setState( JobCommunic::Finished );
    bool ret = comm->sendState();

    if ( ret )
	mStrmWithProcID( "Successfully wrote finish status" );
    else
	mStrmWithProcID( "Could not write finish status" );
    return ret;
}


