/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 14-9-1998
 * FUNCTION : Batch Program 'driver'
-*/
 
static const char* rcsID = "$Id: batchprog.cc,v 1.31 2003-02-26 08:56:15 arend Exp $";

#include "batchprog.h"
#include "ioparlist.h"
#include "strmprov.h"
#include "strmdata.h"
#include "filegen.h"
#include "timefun.h"
#include "sighndl.h"
#include "socket.h"
#include "mmdefs.h"
#ifndef __msvc__
#include <unistd.h>
#include <stdlib.h>
#endif
#include <fstream>


BatchProgram* BatchProgram::inst_;

int Execute_batch( int* pargc, char** argv )
{
    BatchProgram::inst_ = new BatchProgram( pargc, argv );
    if ( !BP().stillok_ )
	return 1;

    if ( BP().inbg_ )
    {
#ifndef __msvc__
	switch ( fork() )
	{
	case -1:
	    cerr << argv[0] <<  "cannot fork:\n" << errno_message() << endl;
	case 0: break;
	default:
	    Time_sleep( 0.1 );
	    exit( 0 );
	break;
	}
#endif
    }

    BatchProgram& bp = *BatchProgram::inst_;
    bool allok = bp.initOutput() && bp.go( *bp.sdout_.ostrm );
    bp.stillok_ = allok;
    delete BatchProgram::inst_;
    return allok ? 0 : 1;
}


BatchProgram::BatchProgram( int* pac, char** av )
	: UserIDObject("")
	, pargc_(pac)
	, argv_(av)
	, argshift_(2)
	, stillok_(false)
	, fullpath_(av[0])
	, inbg_(NO)
	, sdout_(*new StreamData)
    	, masterport_(0)
	, usesock_( false )
	, timestamp_( Time_getMilliSeconds() )
	, exitstat_( mSTAT_UNDEF )
{
    const char* fn = argv_[1];
    while ( fn && *fn == '-' )
    {
	if ( !strcmp(fn,"-bg") )
	    inbg_ = YES;
	else if ( !strncmp(fn,"-masterhost",11) )
	{
	    argshift_++;
	    fn = argv_[ argshift_ - 1 ];
	    masterhost_ = fn;
	}
	else if ( !strncmp(fn,"-masterport",11) )
	{
	    argshift_++;
	    fn = argv_[ argshift_ - 1 ];
	    masterport_ = atoi(fn);
	}
	else if ( *(fn+1) )
	    opts_ += new BufferString( fn+1 );



	argshift_++;
	fn = argv_[ argshift_ - 1 ];
    }

    usesock_ = masterhost_.size() && masterport_ > 0; // both must be set.
     
    if ( !fn || !*fn )
    {
	BufferString msg( progName() );
	msg += ": No parameter file name specified";

	if ( !writeErrorMsg( msg ) ) cerr << msg << endl;
	return;
    }

    setName( fn );
    ifstream strm( fn );
    if ( !strm.good() )
    {
	BufferString msg( name() );
	msg += ": Cannot open parameter file: ";
	msg += fn;

	if ( !writeErrorMsg( msg ) ) cerr << msg << endl;
	return;
    }
 
    IOParList parlist( strm );
    strm.close();
    if ( parlist.size() == 0 )
    {
	BufferString msg( argv_[0] );
	msg += ": Invalid input file: ";
	msg += fn;

	if ( !writeErrorMsg( msg ) ) cerr << msg << endl;
        return;
    }

    iopar_ = new IOPar( *parlist[0] );

    iopar_->get( "Job ID", jobid_ ); 

    const char* res = iopar_->find( "Log file" );
    if ( !res )
	iopar_->set( "Log file", StreamProvider::sStdErr );

    killNotify( true );

    stillok_ = true;
}


BatchProgram::~BatchProgram()
{
    if ( sdout_.ostrm )
    {
	*sdout_.ostrm << "Exiting BatchProgram." << endl;
	sdout_.ostrm->flush();
    }

    if ( exitstat_ == mSTAT_UNDEF ) 
	exitstat_ = stillok_ ? mSTAT_DONE : mSTAT_ERROR;

    writeStatus( mEXIT_STATUS, exitstat_ );

    killNotify( false );

    sdout_.close();
    delete &sdout_;
    deepErase( opts_ );
}


const char* BatchProgram::progName() const
{
    static UserIDString ret;
    ret = File_getFileName( fullpath_ );
    return ret;
}


void BatchProgram::progKilled( CallBacker* )
{
    if ( sdout_.ostrm )
    {
	*sdout_.ostrm << "BatchProgram Killed." << endl;
	sdout_.ostrm->flush();
    }

    exitstat_ = mSTAT_KILLED;
    writeStatus( mEXIT_STATUS, exitstat_ );
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


bool BatchProgram::writeStatus_( char tag , int status, bool force )
{
    if ( !usesock_ ) return true;

    int elapsed = Time_getMilliSeconds() - timestamp_;
    if ( !force && elapsed > 0 && elapsed < 1000  )
	return true;

    timestamp_ = Time_getMilliSeconds();

    if ( Socket* sock = mkSocket() )
    {
	sock->writetag( tag, jobid_, status );

	bool ret = true;

	char masterinfo;
	ret = sock->readtag( masterinfo );
	if ( masterinfo != mRSP_ACK )
	{
	    // TODO : handle requests from master
	    if ( masterinfo == mRSP_REQ_STOP ) 
	    {
		if ( sdout_.ostrm )
		{
		    *sdout_.ostrm << "Exiting on request of Master." << endl;
		    sdout_.ostrm->flush();
		}
		exit( -1 );
	    }

	    ret = false;
	}

	delete sock;

	if ( !ret && sdout_.ostrm )
	{
	    *sdout_.ostrm << "Error writing status to Master." << endl;
	    sdout_.ostrm->flush();
	}

	return ret;
    }

    return false;

}


bool BatchProgram::writeErrorMsg( const char* msg )
{
    if ( !usesock_ ) return false;

    if ( Socket* sock = mkSocket() )
    {
	return sock->writeErrorMsg( msg );
    }

    return false;
}


Socket* BatchProgram::mkSocket()
{
    if ( !usesock_ ) return 0;

    return new Socket( masterhost_, masterport_ );
}

bool BatchProgram::initOutput()
{
    stillok_ = false;
    if ( !writeStatus( mPID_TAG, getPID() ) )
    {
	if ( sdout_.ostrm )
	{
	    *sdout_.ostrm << "Could not write status. Exiting." << endl;
	    sdout_.ostrm->flush();
	}
	exit( 0 );
    }

    const char* res = pars()["Log file"];
    if ( !*res || !strcmp(res,"stdout") ) res = 0;
 
#ifndef __win__
    if ( res && !strcmp(res,"window") )
    {
	FileNameString comm( "@view_progress " );
	comm += getPID();
	StreamProvider sp( comm );
	sdout_ = sp.makeOStream();
	if ( !sdout_.usable() )
	{
	    cerr << name() << ": Cannot open window for output" << endl;
	    cerr << "Using std output instead" << endl;
	    res = 0;
	}
    }
#endif
 
    if ( !res || strcmp(res,"window") )
    {
	StreamProvider spout( res );
	sdout_ = spout.makeOStream();
	if ( !sdout_.ostrm )
	{
	    cerr << name() << ": Cannot open log file" << endl;
	    cerr << "Using std output instead" << endl;
	    sdout_.ostrm = &cout;
	}
    }

    stillok_ = sdout_.usable();
    return stillok_;
}
