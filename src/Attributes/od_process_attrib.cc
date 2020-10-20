/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Mar 2000
-*/


#include "batchprog.h"

#include "attribdescset.h"
#include "attribengman.h"
#include "attriboutput.h"
#include "attribprocessor.h"
#include "attribstorprovider.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "hostdata.h"
#include "dbman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "progressmeterimpl.h"
#include "ptrman.h"
#include "seisjobexecprov.h"
#include "seis2ddata.h"
#include "separstr.h"
#include "jobcommunic.h"
#include "moddepmgr.h"

#define mDestroyWorkers \
{ deleteAndZeroPtr( proc ); }

#define mRetFileProb(fdesc,fnm,s) \
{ \
    BufferString msg(fdesc); \
    msg += " ("; msg += fnm; msg += ") "; msg += s; \
    mRetHostErr( msg ); \
}


mLoad2Modules("Attributes","PreStackProcessing")

bool BatchProgram::doWork( od_ostream& strm )
{
    const int odversion = pars().odVersion();
    if ( odversion < 320 )
    {
	errorMsg( ::toUiString("\nCannot execute pre-3.2 par files") );
	return false;
    }

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

    PtrMan<IOPar> outputs = pars().subselect( sKey::Output() );
    if ( !outputs )
	mRetJobErr( "No outputs found" )

    int outidx = 0;
    while ( true )
    {
	PtrMan<IOPar> outpar = outputs->subselect( outidx );
	if ( !outpar || outpar->isEmpty() )
	    break;

	strm << od_newline;
	strm << "Preparing processing";

	const char* seisid = outpar->find( "Seismic.ID" );
	if ( !seisid )
	    break;

	const DBKey seisdbky( seisid );
	PtrMan<IOObj> seisioobj = seisdbky.getIOObj();
	if ( !seisioobj )
	{
	    const BufferString msg( "Cannot find output Seismic Object (ID=",
			    seisdbky.isValid() ? seisid : "<invalid>", ")" );
	    mRetHostErr( msg );
	}

	File::Path seisfp( seisioobj->mainFileName() );
	if ( !seisfp.isAbsolute() )
	{
	    seisfp.set( DBM().survDir() );
	    seisfp.add( seisioobj->dirName() )
		  .add( seisioobj->mainFileName() );
	}

	BufferString dirnm = seisfp.pathOnly();
	const bool isdir = File::isDirectory( dirnm );
	if ( !isdir || !File::isWritable(dirnm) )
	{
	    BufferString fdesc("Output directory for '");
	    fdesc += seisioobj->name(); fdesc += "'";
	    mRetFileProb(fdesc,dirnm,
			 isdir ? "is not writeable" : "does not exist")
	}

	seisioobj->setDirName( dirnm );
	strm << " of '" << seisioobj->name() << "'.\n" << od_endl;

	Attrib::DescSet attribset( false );
	const char* setid = pars().find( "Attribute Set" );
	uiRetVal uirv;
	if ( setid && *setid )
	    uirv = attribset.load( DBKey(setid) );
	else
	{
	    PtrMan<IOPar> attribs = pars().subselect("Attributes");
	    if ( !attribs )
		mRetJobErr("No Attribute Definition found")

	    uirv = attribset.usePar( *attribs );
	}
	if ( !uirv.isOK() )
	    mRetJobErr( uirv.getText() );

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

	const BufferString subselkey =
		IOPar::compKey( sKey::Output(), sKey::Subsel() );
	PtrMan<IOPar> subselpar = pars().subselect( subselkey );
	if ( alllinenames.isEmpty() && subselpar )
	{
	    if ( !subselpar->get(sKey::LineKey(),alllinenames) )
	    {
		int lidx = 0; Pos::GeomID geomid;
		while ( true )
		{
		    PtrMan<IOPar> linepar = subselpar->subselect(
					IOPar::compKey(sKey::Line(),lidx++) );
		    if ( !linepar || !linepar->get(sKey::GeomID(),geomid) )
			break;

		    const BufferString linename = geomid.name();
		    if ( linename.isEmpty() )
			break;

		    alllinenames.add( linename );
		}
	    }
	}

	const char* attrtypstr = pars().find(
		IOPar::compKey(sKey::Attributes(),sKey::Type()) );
	const bool is2d = attrtypstr && *attrtypstr == '2';

	//processing dataset on a single machine
	if ( alllinenames.isEmpty() && is2d )
	{
	    DBKey dsid;
	    pars().get( "Input Line Set", dsid );
	    PtrMan<IOObj> dsobj = dsid.getIOObj();
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
	    IOPar procpar( pars() );
	    if ( is2d && subselpar )
	    {
		Pos::GeomID geomid;
		PtrMan<IOPar> linepar =
		    subselpar->subselect( IOPar::compKey(sKey::Line(),idx) );
		if ( !linepar || !linepar->get(sKey::GeomID(),geomid) )
		    break;

		const BufferString linename = geomid.name();
		if ( linename.isEmpty() )
		    break;

		procpar.updateComp( *linepar, subselkey );
	    }

	    Attrib::EngineMan attrengman;
	    Attrib::DescSet attribsetlocal( attribset ); //May change
	    const auto geomid = is2d
			? SurvGeom::getGeomID(alllinenames.get(idx))
			: Pos::GeomID::get3D();
	    proc = attrengman.usePar( procpar, attribsetlocal, uirv,
				      outidx, geomid );
	    if ( !proc )
		mRetJobErr( ::toString(uirv) );

	    progressmeter.setName( proc->name() );
	    progressmeter.setMessage( proc->message() );

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
			    strm
			    << "Estimated number of positions to be processed"
			    << " (assuming regular input): " << proc->totalNr()
			    << od_endl << od_endl;
			}

			progressmeter.setTotalNr( proc->totalNr() );
			progressmeter.setStarted();
		    }

		    if ( res > 0 )
		    {
			if ( comm_ && !comm_->updateProgress( nriter + 1 ) )
			    mRetHostErr( ::toString( comm_->errMsg() ) )

			if ( proc->nrDone()>nrdone )
			{
			    nrdone++;
			    ++progressmeter;
			}
		    }
		    else
		    {
			if ( res == -1 )
			    mRetJobErr(
				BufferString("Cannot reach next position:\n",
						 ::toString(proc->message())) );
			break;
		    }

		    if ( res >= 0 )
		    {
			nriter++;
			if ( !proc->outputs_[0]->writeTrc() )
			    mRetJobErr(
				BufferString("Cannot write output trace:\n",
				    ::toString(proc->outputs_[0]->errMsg())) )
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

	PtrMan<IOObj> ioobj = seisdbky.getIOObj();
	BufferString finishmsg( "\nFinished processing" );
	if ( ioobj )
	{
	    File::Path fp( ioobj->mainFileName() );
	    fp.setExtension( sProcFileExtension() );
	    pars().write( fp.fullPath(), sKey::Pars() );
	    finishmsg.add( " of " ).add( ioobj->name() );
	}

	mMessage( finishmsg );
	outidx++;
    }

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
