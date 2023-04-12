/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

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
#include "seis2ddata.h"
#include "separstr.h"
#include "jobcommunic.h"
#include "moddepmgr.h"



defineTranslatorGroup(AttribDescSet,"Attribute definitions");
mDefSimpleTranslatorSelector(AttribDescSet);

#define mDestroyWorkers \
{ deleteAndNullPtr( proc ); }


#define mRetFileProb(fdesc,fnm,s) \
	{ \
	    BufferString msg(fdesc); \
	    msg += " ("; msg += fnm; msg += ") "; msg += s; \
	    mRetHostErr( msg ); \
	}


mLoad3Modules("EMAttrib","WellAttrib","PreStackProcessing")

bool BatchProgram::doWork( od_ostream& strm )
{
    const int odversion = pars().odVersion();
    if ( odversion < 320 )
    {
	errorMsg( ::toUiString("\nCannot execute pre-3.2 par files") );
	return false;
    }

    Attrib::Processor* proc = nullptr;
    const BufferString tempdir = pars().find(sKey::TmpStor());
    if ( !tempdir.isEmpty() )
    {
	if ( !File::exists(tempdir) )
	    mRetFileProb(sKey::TmpStor(),tempdir,"does not exist")
	else if ( !File::isDirectory(tempdir) )
	    mRetFileProb(sKey::TmpStor(),tempdir,"is not a directory")
	else if ( !File::isWritable(tempdir) )
	    mRetFileProb(sKey::TmpStor(),tempdir,"is not writeable")
    }

    const BufferString selspec = pars().find( "Output.1.In-line range" );
    if ( !selspec.isEmpty() )
    {
	FileMultiString fms( selspec );
	const int lnr = fms.getIValue( 0 );
	if ( lnr == fms.getIValue(1) )
	    strm << "Calculating for in-line " << lnr << '.' << od_newline;
    }

    strm << od_newline;
    strm << "Preparing processing";

    MultiID outmid;
    pars().get( "Output.0.Seismic.ID", outmid );
    if ( outmid.isUdf() )
	pars().get( "Output.1.Seismic ID", outmid );

    if ( outmid.isUdf() )
	strm << " ..." << od_newline;
    else
    {
	PtrMan<IOObj> ioobj = IOM().get( outmid );
	if ( !ioobj )
	{
	    BufferString msg( "Cannot find output Seismic Object with ID '" );
	    msg += outmid.toString().buf(); msg += "' ..."; mRetHostErr( msg );
	}

	FilePath fp( ioobj->fullUserExpr(false) );
	if ( !fp.isAbsolute() )
	{
	    fp.set( IOM().rootDir().fullPath() );
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
    MultiID attribmid;
    pars().get( "Attribute Set", attribmid );
    if ( !attribmid.isUdf() )
    {
	PtrMan<IOObj> ioobj = IOM().get( attribmid );
	if ( !ioobj )
	    mRetHostErr( "Cannot find provided attrib set ID" )
	uiString msg;
	if ( !AttribDescSetTranslator::retrieve(attribset,ioobj,msg) )
	    mRetJobErr( msg );
    }
    else
    {
	PtrMan<IOPar> attribs = pars().subselect("Attributes");
	if ( !attribs )
	    mRetJobErr("No Attribute Definition found")

	if ( !attribset.usePar(*attribs) )
	    mRetJobErr( attribset.errMsg() )
    }

    PtrMan<IOPar> outputs = pars().subselect("Output");
    if ( !outputs )
	mRetJobErr( "No outputs found" )

    int indexoutp = 0; BufferStringSet alllinenames;
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

	output->get( sKey::LineKey(), alllinenames );
	indexoutp++;
    }

    const BufferString subselkey = IOPar::compKey( sKey::Output(),
						   sKey::Subsel() );
    PtrMan<IOPar> subselpar = pars().subselect( subselkey );
    if ( alllinenames.isEmpty() && subselpar )
    {
	if ( !subselpar->get(sKey::LineKey(),alllinenames) )
	{
	    int lidx = 0;
	    Pos::GeomID geomid = Survey::GeometryManager::cUndefGeomID();
	    while ( true )
	    {
		PtrMan<IOPar> linepar =
		    subselpar->subselect( IOPar::compKey(sKey::Line(),lidx++) );
		if ( !linepar || !linepar->get(sKey::GeomID(),geomid) )
		    break;

		const StringView linename = Survey::GM().getName( geomid );
		if ( linename.isEmpty() )
		    break;

		alllinenames.add( linename );
	    }
	}
    }

    const BufferString attrtypstr = pars().find( "Attributes.Type" );
    const bool is2d = attrtypstr.firstChar() == '2';

    //processing dataset on a single machine
    if ( alllinenames.isEmpty() && is2d )
    {
	MultiID dsid;
	pars().get( "Input Line Set", dsid );
	PtrMan<IOObj> dsobj = IOM().get( dsid );
	if ( dsobj )
	{
	    Seis2DDataSet ds(*dsobj);
	    for ( int idx=0; idx<ds.nrLines(); idx++ )
		alllinenames.addIfNew(ds.lineName(idx));
	}
    }

    if ( alllinenames.isEmpty() && !is2d )	//all other cases
	alllinenames.add("");

    TextStreamProgressMeter progressmeter( strm );
    for ( int idx=0; idx<alllinenames.size(); idx++ )
    {
	uiString errmsg;
	IOPar procpar( pars() );
	if ( is2d && subselpar )
	{
	    Pos::GeomID geomid = Survey::GeometryManager::cUndefGeomID();
	    PtrMan<IOPar> linepar =
		subselpar->subselect( IOPar::compKey(sKey::Line(),idx) );
	    if ( !linepar || !linepar->get(sKey::GeomID(),geomid) )
		break;

	    const StringView linename = Survey::GM().getName( geomid );
	    if ( linename.isEmpty() )
		break;

	    procpar.removeSubSelection( subselkey );
	    procpar.mergeComp( *linepar, subselkey );
	}

	Attrib::EngineMan attrengman;
	Attrib::DescSet attribsetlocal( attribset ); //May change
	proc = attrengman.usePar( procpar, attribsetlocal,
				  alllinenames.get(idx), errmsg );
	if ( !proc )
	    mRetJobErr( ::toString(errmsg) );

	progressmeter.setName( proc->name() );
	progressmeter.setMessage( proc->uiMessage() );

	mSetCommState(Working);

	double startup_wait = 0;
	pars().get( "Startup delay time", startup_wait );
	sleepSeconds( startup_wait );
	if ( comm_ )
	    comm_->setTimeBetweenMsgUpdates( 5000 );

	const double pause_sleep_time =
				GetEnvVarDVal( "OD_BATCH_SLEEP_TIME", 1 );
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
			setResumed();
		}

		progressmeter.setNrDone( proc->nrDone() );
		const int res = proc->nextStep();
		if ( nriter == 0 )
		{
		    if ( is2d )
		    {
			strm << "\nProcessing attribute on line "
			     << alllinenames.get(idx).buf()
			     << " (" << idx+1 << "/" << alllinenames.size()
			     << ")\n" << od_endl;
		    }
		    else
		    {
			strm << "Estimated number of positions to be processed"
			     << " (assuming regular input): " << proc->totalNr()
			     << "\n" << od_endl;
		    }

		    progressmeter.setTotalNr( proc->totalNr() );
		    progressmeter.setStarted();
		}

		if ( res > 0 )
		{
		    const int percdone =
				mNINT32( 100*(float)nrdone/proc->totalNr() );
		    if ( comm_ && !comm_->updateProgress(percdone) )
			mRetHostErr( comm_->errMsg().getFullString() )

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
				    ":\n",proc->uiMessage().getFullString()) )
		    break;
		}

		if ( res >= 0 )
		{
		    nriter++;
		    if ( !proc->outputs_[0]->writeTrc() )
			mRetJobErr( BufferString("Cannot write output trace",
			    ":\n",proc->outputs_[0]->errMsg()) )
		}
	    }
	}

	progressmeter.setFinished();
	bool closeok = true;
	if ( nriter )
	    closeok = proc->outputs_[0]->finishWrite();

	if ( !closeok )
	{ mMessage( "Could not close output data." ); }
	else
	{
	    if ( is2d && alllinenames.size()>1 )
		strm << "\nProcessing on " << alllinenames.get(idx)
		     << " finished.\n" << od_endl;
	}

	mDestroyWorkers
    }

    PtrMan<IOObj> ioobj = IOM().get( outmid );
    BufferString finishmsg( "\nFinished processing" );
    if ( ioobj )
    {
	FilePath fp( ioobj->fullUserExpr() );
	fp.setExtension( "proc" );
	pars().write( fp.fullPath(), sKey::Pars() );
	finishmsg.add( " of " ).add( ioobj->name() );
    }

    mMessage( finishmsg );

    if ( !comm_ )
	return true;

    mMessage( "\nWriting finish status: " );
    comm_->setState( JobCommunic::Finished );
    bool ret = comm_->sendState();

    if ( ret )
	mMessage( "Successfully wrote finish status." );
    else
	mMessage( "Could not write finish status." );

    return ret;
}
