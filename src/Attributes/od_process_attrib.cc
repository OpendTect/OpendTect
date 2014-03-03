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
#include "jobcommunic.h"
#include "moddepmgr.h"



defineTranslatorGroup(AttribDescSet,"Attribute definitions");

#define mDestroyWorkers \
{ delete proc; proc = 0; }


#define mRetFileProb(fdesc,fnm,s) \
	{ \
	    BufferString msg(fdesc); \
	    msg += " ("; msg += fnm; msg += ") "; msg += s; \
	    mRetHostErr( msg ); \
	}


bool BatchProgram::go( od_ostream& strm )
{
    const int odversion = pars().odVersion();
    if ( odversion < 320 )
    { errorMsg("\nCannot execute pre-3.2 par files"); return false; }

    OD::ModDeps().ensureLoaded( "Attributes" );
    OD::ModDeps().ensureLoaded( "PreStackProcessing" );

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
	const int lnr = fms.getIValue( 0 );
	if ( lnr == fms.getIValue(1) )
	    strm << "Calculating for in-line " << lnr << '.' << od_newline;
    }
    strm << od_newline;

    strm << "Preparing processing" << od_endl;
    const char* seisid = pars().find( "Output.0.Seismic.ID" );
    if ( !seisid )
	seisid = pars().find( "Output.1.Seismic ID" );

    if ( !seisid )
	strm << " ..." << od_newline;
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

	strm << " of '" << ioobj->name() << "'.\n" << od_endl;
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
	if ( !attribset.usePar(*attribs) )
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
	linename = output->find( sKey::LineKey() );
	indexoutp++;
    }

    PtrMan<IOPar> subselpar = pars().subselect(
	    IOPar::compKey(sKey::Output(),sKey::Subsel()) );
    if ( linename.isEmpty() && subselpar )
	linename = subselpar->find( sKey::LineKey() );

    const char* attrtypstr = pars().find( "Attributes.Type" );
    const bool is2d = attrtypstr && *attrtypstr == '2';
    BufferStringSet alllinenames;
    if ( linename.isEmpty() && is2d ) //processing lineset on a single machine
    {
	MultiID lsid;
	pars().get( "Input Line Set", lsid );
	PtrMan<IOObj> lsobj = IOM().get( lsid );
	if ( lsobj )
	{
	    Seis2DLineSet ls(*lsobj);
	    for ( int idx=0; idx<ls.nrLines(); idx++ )
		alllinenames.addIfNew(ls.lineName(idx));
	}
    }

    if ( alllinenames.isEmpty() )	//all other cases
	alllinenames.add(linename);

    TextStreamProgressMeter progressmeter(strm);
    for ( int idx=0; idx<alllinenames.size(); idx++ )
    {
	BufferString errmsg;
	proc = attrengman->usePar( pars(), attribset, alllinenames.get(idx),
				   errmsg );
	if ( !proc )
	    mRetJobErr( errmsg );

	mSetCommState(Working);

	double startup_wait = 0;
	pars().get( "Startup delay time", startup_wait );
	sleepSeconds( startup_wait );

	const double pause_sleep_time =
				GetEnvVarDVal( "OD_BATCH_SLEEP_TIME", 1 );
	bool loading = true;
	int nriter = 0, nrdone = 0;

	while ( true )
	{
	    bool paused = false;

	    if ( pauseRequested() )
	    {
		paused = true;
		mSetCommState(Paused);
		sleepSeconds( pause_sleep_time );
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
			 << "\nLoading cube data ...\n" << od_endl;
		    progressmeter.setTotalNr( proc->totalNr() );
		}
		if ( nriter == 0 && is2d && alllinenames.size()>1 )
		{
		    strm << "\nComputing on line "
			 << alllinenames.get(idx).buf()<< "\n" << od_endl;
		}

		if ( res > 0 )
		{
		    if ( loading )
		    {
			loading = false;
			mMessage( "Processing started" );
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
	{ mMessage( "Could not close output data." ); }
	else
	{
	    mMessage( "Processing done; Closing down" );
	    if ( is2d && alllinenames.size()>1 )
		strm << "\nComputed line " <<idx+1<< " out of "
		     << alllinenames.size()<<"\n" << od_endl;
	}

	mDestroyWorkers
    }

    PtrMan<IOObj> ioobj = IOM().get( seisid );
    if ( ioobj )
    {
	FilePath fp( ioobj->fullUserExpr() );
	fp.setExtension( "proc" );
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


