/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Mar 2000
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



defineTranslatorGroup(AttribDescSet,"Attribute definitions");
mDefSimpleTranslatorSelector(AttribDescSet);

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
    {
	errorMsg(toUiString("\nCannot execute pre-3.2 par files"));
	return false;
    }

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

    PtrMan<IOPar> outputs = pars().subselect( sKey::Output() );
    if ( !outputs )
	mRetJobErr( "No outputs found" )

    int outidx = 0;
    while ( true )
    {
	strm << od_newline;
	strm << "Preparing processing";

	PtrMan<IOPar> outpar = outputs->subselect( outidx );
	if ( !outpar ) break;

	const char* seisid = outpar->find( "Seismic.ID" );
	if ( !seisid ) break;
	DBKey seisdbky = DBKey::getFromString( seisid );

	PtrMan<IOObj> seisioobj = DBM().get( seisdbky );
	if ( !seisioobj )
	{
	    BufferString msg( "Cannot find output Seismic Object with ID '" );
	    msg += seisid; msg += "' ..."; mRetHostErr( msg );
	}

	File::Path seisfp( seisioobj->fullUserExpr(false) );
	if ( !seisfp.isAbsolute() )
	{
	    seisfp.set( DBM().survDir() );
	    seisfp.add( seisioobj->dirName() );
	    seisfp.add( seisioobj->fullUserExpr(false) );
	}

	BufferString dirnm = seisfp.pathOnly();
	seisioobj->setDirName( dirnm.buf() );
	const bool isdir = File::isDirectory( dirnm );
	if ( !isdir || !File::isWritable(dirnm) )
	{
	    BufferString fdesc("Output directory for '");
	    fdesc += seisioobj->name(); fdesc += "'";
	    mRetFileProb(fdesc,dirnm,
			 isdir ? "is not writeable" : "does not exist")
	}

	strm << " of '" << seisioobj->name() << "'.\n" << od_endl;

	Attrib::DescSet attribset( false );
	const char* setid = pars().find( "Attribute Set" );
	if ( setid && *setid )
	{
	    PtrMan<IOObj> ioobj = DBM().get( DBKey::getFromString(setid) );
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

	PtrMan<Attrib::EngineMan> attrengman = new Attrib::EngineMan();
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
		int lidx = 0;
		Pos::GeomID geomid = Survey::GeometryManager::cUndefGeomID();
		while ( true )
		{
		    PtrMan<IOPar> linepar = subselpar->subselect(
					IOPar::compKey(sKey::Line(),lidx++) );
		    if ( !linepar || !linepar->get(sKey::GeomID(),geomid) )
			break;

		    const FixedString linename = Survey::GM().getName( geomid );
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
	    PtrMan<IOObj> dsobj = DBM().get( dsid );
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

		const FixedString linename = Survey::GM().getName( geomid );
		if ( linename.isEmpty() )
		    break;

		procpar.removeSubSelection( subselkey );
		procpar.mergeComp( *linepar, subselkey );
	    }

	    proc = attrengman->usePar( procpar, attribset,
				       alllinenames.get(idx), errmsg, outidx );
	    if ( !proc )
		mRetJobErr( errmsg.getFullString() );

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
		    }

		    const int res = proc->nextStep();

		    if ( nriter == 0 && !is2d )
		    {
			strm << "Estimated number of positions to be processed"
			     << " (assuming regular input): " << proc->totalNr()
			     << od_endl << od_endl;
			progressmeter.setTotalNr( proc->totalNr() );
		    }
		    if ( nriter == 0 && is2d && alllinenames.size()>1 )
		    {
			strm << "\nProcessing attribute on line "
			     << alllinenames.get(idx).buf()
			     << " (" << idx+1 << "/" << alllinenames.size()
			     << ")" << "\n" << od_endl;
		    }

		    if ( res > 0 )
		    {
			if ( comm_ && !comm_->updateProgress( nriter + 1 ) )
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
			    mRetJobErr(
				BufferString("Cannot reach next position",
					     ":\n",
					     proc->uiMessage().getFullString()))
			break;
		    }

		    if ( res >= 0 )
		    {
			nriter++;
			if ( !proc->outputs_[0]->writeTrc() )
			    mRetJobErr(
				BufferString("Cannot write output trace",
					     ":\n",
			    mFromUiStringTodo(proc->outputs_[0]->errMsg())) )
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
		if ( is2d && alllinenames.size()>1 )
		    strm << "\n\nProcessing on " << alllinenames.get(idx)
			 << " finished.\n\n\n";
	    }

	    mDestroyWorkers
	}

	PtrMan<IOObj> ioobj = DBM().get( seisdbky );
	if ( ioobj )
	{
	    File::Path fp( ioobj->fullUserExpr() );
	    fp.setExtension( "proc" );
	    pars().write( fp.fullPath(), sKey::Pars() );
	}

	// It is VERY important workers are destroyed
	// BEFORE the last sendState!!!
	progressmeter.setFinished();
	outidx++;
    }

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
