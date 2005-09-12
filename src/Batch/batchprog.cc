/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 14-9-1998
 * FUNCTION : Batch Program 'driver'
-*/
 
static const char* rcsID = "$Id: batchprog.cc,v 1.79 2005-09-12 13:44:25 cvsarend Exp $";

#include "batchprog.h"
#include "ioparlist.h"
#include "ioman.h"
#include "iodir.h"
#include "strmprov.h"
#include "strmdata.h"
#include "filepath.h"
#include "sighndl.h"
#include "hostdata.h"
#include "plugins.h"
#include "strmprov.h"
#include "ctxtioobj.h"
#include "mmsockcommunic.h"
#include "keystrs.h"

#ifndef __msvc__
#include <unistd.h>
#include <stdlib.h>
#endif
#include <iostream>

#ifdef __win__
#include "filegen.h"
#else
#include "_execbatch.h"
#endif

#ifdef __mac__
#include "oddirs.h"
#endif


BatchProgram* BatchProgram::inst;

BatchProgram::BatchProgram( int* pac, char** av )
	: UserIDObject("")
	, pargc(pac)
	, argv_(av)
	, argshift(2)
	, stillok(false)
	, fullpath(av[0])
	, inbg(NO)
	, sdout(*new StreamData)
	, comm(0)
{
    od_putProgInfo( *pargc, argv_ );

    BufferString masterhost;
    int masterport = -1;
    
    const char* fn = argv_[1];
    while ( fn && *fn == '-' )
    {
	if ( !strcmp(fn,"-bg") )
	    inbg = YES;
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
	    masterport = atoi(fn);
	}
	else if ( !strncmp(fn,"-jobid",6) )
	{
	    argshift++;
	    fn = argv_[ argshift - 1 ];
	    jobid = atoi(fn);
	}
	else if ( *(fn+1) )
	    opts += new BufferString( fn+1 );

	argshift++;
	fn = argv_[ argshift - 1 ];
    }

    if ( masterhost.size() && masterport > 0 )  // both must be set.
	comm = new MMSockCommunic( masterhost, masterport, jobid, sdout );
     
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

    while ( !File_exists(fn) && count-->0 )
	Time_sleep(1);

#endif

    StreamData sd = StreamProvider( fn ).makeIStream();
    if ( !sd.usable() )
    {
	BufferString msg( name() );
	msg += ": Cannot open parameter file: ";
	msg += fn;

	errorMsg( msg );
	return;
    }
 
    IOParList parlist( *sd.istrm );
    sd.close();
    if ( parlist.size() == 0 )
    {
	BufferString msg( argv_[0] );
	msg += ": Invalid input file: ";
	msg += fn;

	errorMsg( msg ); 
        return;
    }

    iopar = new IOPar( *parlist[0] );

    const char* res = iopar->find( sKey::LogFile );
    if ( !res )
	iopar->set( sKey::LogFile, StreamProvider::sStdErr );
    res = iopar->find( sKey::Survey );
    if ( !res || !*res )
	IOMan::newSurvey();
    else
    {
#ifdef __debug__
	const char* oldsnm = IOM().surveyName();
	if ( !oldsnm ) oldsnm = "<empty>";
	std::cerr << "Using survey from par file: " << res << ". was: "
	    	  << oldsnm << std::endl;
#endif
	IOMan::setSurvey( res );
    }

    killNotify( true );

    stillok = true;
}


BatchProgram::~BatchProgram()
{
    infoMsg( "Finished batch processing." );

    if( comm )
    {
	MMSockCommunic::State s = comm->state();

	bool isSet =  s == MMSockCommunic::AllDone 
	           || s == MMSockCommunic::JobError
		   || s == MMSockCommunic::HostError;

	if ( !isSet )
	    comm->setState( stillok ? MMSockCommunic::AllDone
				    : MMSockCommunic::HostError );

       	comm->sendState( true );
    }

    killNotify( false );

    sdout.close();
    delete &sdout;
    deepErase( opts );
    delete iopar;
    delete comm; 
}


const char* BatchProgram::progName() const
{
    static UserIDString ret;
    ret = FilePath( fullpath ).fileName();
    return ret;
}


void BatchProgram::progKilled( CallBacker* )
{
    infoMsg( "BatchProgram Killed." );

    if ( comm ) 
    {
	comm->setState( MMSockCommunic::Killed );
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


bool BatchProgram::infoMsg( const char* msg, bool cc_stdout)
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

    const char* res = pars()["Log file"];
    if ( !*res || !strcmp(res,"stdout") ) res = 0;
 
#ifndef __win__
    if ( res && !strcmp(res,"window") )
    {

#ifdef __mac__ 
        // Mac requires full path in order to support GUI apps
	BufferString comm( "@'" );
	comm += FilePath(GetBinDir()).add("mac")
			    .add("view_progress").fullPath();
	comm += "' ";
#else
	BufferString comm( "@view_progress " );
#endif
	comm += GetPID();
	StreamProvider sp( comm );
	sdout = sp.makeOStream();
	if ( !sdout.usable() )
	{
	    std::cerr << name() << ": Cannot open window for output"<<std::endl;
	    std::cerr << "Using std output instead" << std::endl;
	    res = 0;
	}
    }
#endif
 
    if ( !res || strcmp(res,"window") )
    {
	StreamProvider spout( res );
	sdout = spout.makeOStream();
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
    BufferString res = pars().find( iopkey );
    if ( res == "" )
    {
	iopkey = basekey; res = pars().find( iopkey );
	if ( res == "" )
	{
	    iopkey += ".Name"; res = pars().find( iopkey );
	    if ( res == "" )
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
	    IOM().to( ctio.ctxt.stdSelKey() );
	    const IOObj* ioob = (*(const IODir*)(IOM().dirPtr()))[res];
	    if ( ioob )
		res = ioob->key();
	    else if ( mknew )
	    {
		ctio.setName( res );
		IOM().getEntry( ctio );
		if ( ctio.ioobj );
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

