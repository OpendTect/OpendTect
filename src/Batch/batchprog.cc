/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 14-9-1998
 * FUNCTION : Batch Program 'driver'
-*/
 
static const char* rcsID = "$Id: batchprog.cc,v 1.116 2012/08/22 12:37:25 cvshelene Exp $";

#include "batchprog.h"
#include "envvars.h"
#include "file.h"
#include "ioman.h"
#include "iodir.h"
#include "iopar.h"
#include "oddirs.h"
#include "strmprov.h"
#include "strmdata.h"
#include "filepath.h"
#include "sighndl.h"
#include "hostdata.h"
#include "plugins.h"
#include "strmprov.h"
#include "ctxtioobj.h"
#include "jobcommunic.h"
#include "keystrs.h"
#include "ascstream.h"
#include "debugmasks.h"

#ifndef __msvc__
# include <unistd.h>
# include <stdlib.h>
#endif

#include <iostream>

#ifndef __win__
# include "_execbatch.h"
#endif

#if defined(__mac__) || defined(__msvc__)
# include "oddirs.h"
#endif


BatchProgram* BatchProgram::inst_ = 0;

BatchProgram& BP()
{
    if ( !BatchProgram::inst_ )
	BatchProgram::inst_ = new BatchProgram;
    return *BatchProgram::inst_;
}


void BatchProgram::deleteInstance()
{
    delete BatchProgram::inst_;
}


BatchProgram::BatchProgram()
	: NamedObject("")
	, pargc(0)
	, argv_(0)
	, argshift(2)
	, stillok(false)
	, fullpath("")
	, finishmsg_("Finished batch processing.")
	, inbg(false)
	, sdout(*new StreamData)
	, iopar(new IOPar)
	, comm(0)
{}


void BatchProgram::init( int* pac, char** av )
{
    pargc = pac;
    argv_ = av;
    fullpath = argv_[0];

    od_putProgInfo( *pargc, argv_ );

    BufferString masterhost;
    int masterport = -1;
    
    BufferString fn = argv_[1];
    while ( fn && *fn == '-' )
    {
	if ( fn=="-bg" )
	    inbg = true;
	else if ( !strncmp(fn,"-masterhost",11) )
	{
	    argshift++;
	    fn = argv_[ argshift - 1 ];
	    masterhost = fn;
	}
	else if ( !strncmp(fn,"-masterport",11) )
	{
	    argshift++;
	    fn = argv_[ argshift - 1 ];
	    masterport = toInt(fn);
	}
	else if ( !strncmp(fn,"-jobid",6) )
	{
	    argshift++;
	    fn = argv_[ argshift - 1 ];
	    jobid = toInt(fn);
	}
	else if ( *(fn+1) )
	    opts += new BufferString( fn+1 );

	argshift++;
	fn = argv_[ argshift - 1 ];
    }

    if ( masterhost.size() && masterport > 0 )  // both must be set.
	comm = new JobCommunic( masterhost, masterport, jobid, sdout );
   
    if ( !fn || !*fn )
    {
	BufferString msg( progName() );
	msg += ": No parameter file name specified";

	errorMsg( msg );
	return;
    }


    FilePath parfp( fn );
    
    static BufferString parfilnm; parfilnm = parfp.fullPath();
    replaceCharacter(parfilnm.buf(),'%',' ');
    fn = parfilnm;

    setName( fn );

#ifdef __win__

    int count=10;

    while ( !File::exists(fn) && count-->0 )
	Threads::sleep(1);

#endif

    StreamData sd = StreamProvider( fn ).makeIStream();
    if ( !sd.usable() )
    {
	errorMsg( BufferString( name(), ": Cannot open parameter file: ", fn ) );
	return;
    }
 
    ascistream aistrm( *sd.istrm, true );
    if ( aistrm.fileType()!=sKey::Pars )
    {
	errorMsg( BufferString("Input file ",fn," is not a parameter file") );
	std::cerr << aistrm.fileType() << std::endl;
	return;
    }
    parversion_ = aistrm.version();
    iopar->getFrom( aistrm );
    sd.close();

    if ( iopar->size() == 0 )
    {
	errorMsg( BufferString(argv_[0],": Invalid input file: ",fn) ); 
        return;
    }


    BufferString res = iopar->find( sKey::LogFile ).str();
    if ( !res )
	iopar->set( sKey::LogFile, StreamProvider::sStdErr() );

    res = iopar->find( sKey::DataRoot ).str();
    if ( !res.isEmpty() && File::exists(res) )
	SetEnvVar( "DTECT_DATA", res );

    res = iopar->find( sKey::Survey ).str();
    if ( res.isEmpty() )
	IOMan::newSurvey();
    else
    {
	if ( DBG::isOn(DBG_PROGSTART) )
	{
	    const char* oldsnm = IOM().surveyName();
	    if ( !oldsnm ) oldsnm = "<empty>";
	    if ( res!=oldsnm )
	    {
		BufferString msg( "Using survey from par file: ", res,
				  ". was: " ); msg += oldsnm;
		infoMsg( msg );
	    }
	}
	IOMan::setSurvey( res );
    }

    killNotify( true );

    stillok = true;
}


BatchProgram::~BatchProgram()
{
    infoMsg( finishmsg_ );
    IOM().applClosing();

    if ( comm )
    {

	JobCommunic::State s = comm->state();

	bool isSet =  s == JobCommunic::AllDone 
	           || s == JobCommunic::JobError
		   || s == JobCommunic::HostError;

	if ( !isSet )
	    comm->setState( stillok ? JobCommunic::AllDone
				    : JobCommunic::HostError );

       	bool ok = comm->sendState( true );

	if ( ok )	infoMsg( "Successfully wrote final status" );
	else		infoMsg( "Could not write final status" );

	comm->disConnect();
    }

    killNotify( false );

    sdout.close();
    delete &sdout;

    // Do an explicit exitProgram() here, so we are sure the program
    // is indeed ended and we won't get stuck while cleaning up things
    // that we don't care about.
    ExitProgram( stillok ? 0 : 1 );

    // These will never be reached...
    delete iopar;
    delete comm; 
    deepErase( opts );
}


const char* BatchProgram::progName() const
{
    static BufferString ret;
    ret = FilePath( fullpath ).fileName();
    return ret;
}


void BatchProgram::progKilled( CallBacker* )
{
    infoMsg( "BatchProgram Killed." );

    if ( comm ) 
    {
	comm->setState( JobCommunic::Killed );
	comm->sendState( true );
    }

    killNotify( false );

#ifdef __debug__
    abort();
#endif
}


void BatchProgram::killNotify( bool yn )
{
    CallBack cb( mCB(this,BatchProgram,progKilled) );

    if ( yn )
	SignalHandling::startNotify( SignalHandling::Kill, cb );
    else
	SignalHandling::stopNotify( SignalHandling::Kill, cb );
}


bool BatchProgram::pauseRequested() const
    { return comm ? comm->pauseRequested() : false; }


bool BatchProgram::errorMsg( const char* msg, bool cc_stderr )
{
    if ( sdout.ostrm )
	*sdout.ostrm << '\n' << msg << '\n' << std::endl;

    if ( !sdout.ostrm || cc_stderr )
	std::cerr << '\n' << msg << '\n' << std::endl;

    if ( comm && comm->ok() ) return comm->sendErrMsg(msg);

    return true;
}


bool BatchProgram::infoMsg( const char* msg, bool cc_stdout )
{
    if ( sdout.ostrm )
	*sdout.ostrm << '\n' << msg << '\n' << std::endl;

    if ( !sdout.ostrm || cc_stdout )
	std::cout << '\n' << msg << '\n' << std::endl;

    return true;
}



bool BatchProgram::initOutput()
{
    stillok = false;
    if ( comm && !comm->sendPID(GetPID()) )
    {
	errorMsg( "Could not contact master. Exiting.", true );
	exit( 0 );
    }

    BufferString res = pars().find( sKey::LogFile ).str();
    if ( res == "stdout" ) res.setEmpty();
 
    bool hasviewprogress = true;
#ifdef __cygwin__
    hasviewprogress = false;
#endif

    if ( hasviewprogress && res && res=="window" )
    {
	BufferString cmd = "@";
	cmd += FilePath(GetBinPlfDir()).add("od_ProgressViewer").fullPath();
	cmd += " ";

	cmd += GetPID();
	StreamProvider sp( cmd );
	sdout = sp.makeOStream();
	if ( !sdout.usable() )
	{
	    std::cerr << name() << ": Cannot open window for output"<<std::endl;
	    std::cerr << "Using std output instead" << std::endl;
	    res = 0;
	}
    }

    if ( res != "window" )
    {
	if ( res.isEmpty() )
	    res = StreamProvider::sStdErr();
	sdout = StreamProvider(res).makeOStream();
	if ( !sdout.ostrm )
	{
	    std::cerr << name() << ": Cannot open log file" << std::endl;
	    std::cerr << "Using stderror instead" << std::endl;
	    sdout.ostrm = &std::cerr;
	}
    }

    stillok = sdout.usable();
    if ( stillok )
	PIM().loadAuto( true );
    return stillok;
}


IOObj* BatchProgram::getIOObjFromPars(	const char* bsky, bool mknew,
					const IOObjContext& ctxt,
       					bool msgiffail ) const
{
    const BufferString basekey( bsky );
    BufferString iopkey( basekey ); iopkey += ".";
    iopkey += "ID";
    BufferString res = pars().find( iopkey ).str();
    if ( res.isEmpty() )
    {
	iopkey = basekey; res = pars().find( iopkey ).str();
	if ( res.isEmpty() )
	{
	    iopkey += ".Name"; res = pars().find( iopkey ).str();
	    if ( res.isEmpty() )
	    {
		if ( msgiffail )
		    *sdout.ostrm << "Please specify '" << iopkey
				  << "'" << std::endl;
		return 0;
	    }
	}
	if ( !IOObj::isKey(res.buf()) )
	{
	    CtxtIOObj ctio( ctxt );
	    IOM().to( ctio.ctxt.getSelKey() );
	    const IOObj* ioob = (*(const IODir*)(IOM().dirPtr()))[res.buf()];
	    if ( ioob )
		res = ioob->key();
	    else if ( mknew )
	    {
		ctio.setName( res );
		IOM().getEntry( ctio );
		if ( ctio.ioobj )
		{
		    IOM().commitChanges( *ctio.ioobj );
		    return ctio.ioobj;
		}
	    }
	}
    }

    IOObj* ioobj = IOM().get( MultiID(res.buf()) );
    if ( !ioobj )
	*sdout.ostrm << "Cannot find the specified '" << basekey << "'"
	    		<< std::endl;
    return ioobj;
}

