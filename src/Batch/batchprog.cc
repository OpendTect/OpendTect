/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 14-9-1998
 * FUNCTION : Batch Program 'driver'
-*/
 
static const char* rcsID = "$Id: batchprog.cc,v 1.25 2003-01-03 17:51:26 bert Exp $";

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
{
    const char* fn = argv_[1];
    if ( fn && !strcmp(fn,"-bg") )
    {
	inbg_ = YES;
	argshift_++;
	fn = argv_[2];
    }
    
    if ( *pargc_ < argshift_ )
    {
        cerr << progName() << ": "
	     << "No parameter file name specified" << endl;
	return;
    }

    setName( fn );
    ifstream strm( fn );
    if ( !strm.good() )
    {
        cerr << name() << ": Cannot open parameter file" << endl;
        return;
    }
 
    IOParList parlist( strm );
    strm.close();
    if ( parlist.size() == 0 )
    {
        cerr << argv_[0] << ": Invalid input file: " << fn << endl;
        return;
    }

    iopar_ = new IOPar( *parlist[0] );

    usesock_ = iopar_->get( "Master host", masterhost_ )
	    && iopar_->get( "Master port", masterport_ ); 
    iopar_->get( "Job ID", jobid_ ); 

    const char* res = iopar_->find( "Log file" );
    if ( !res )
	iopar_->set( "Log file", StreamProvider::sStdErr );

    killNotify( true );

    stillok_ = true;
}


BatchProgram::~BatchProgram()
{
    if( exitstat_ == mSTAT_UNDEF ) 
	exitstat_ = stillok_ ? mSTAT_FINISHED : mSTAT_ERROR;

    writeStatus( mEXIT_STATUS, exitstat_, true );

    killNotify( false );

    sdout_.close();
    delete &sdout_;
}


const char* BatchProgram::progName() const
{
    static UserIDString ret;
    ret = File_getFileName( fullpath_ );
    return ret;
}


void BatchProgram::progKilled( CallBacker* )
{
    exitstat_ = mSTAT_KILLED;
    writeStatus( mEXIT_STATUS, exitstat_, true );
    killNotify( false );
}


void BatchProgram::killNotify( bool yn )
{
    CallBack cb( mCB(this,BatchProgram,progKilled) );

    if ( yn )
	SignalHandling::startNotify( SignalHandling::Kill, cb );
    else
	SignalHandling::stopNotify( SignalHandling::Kill, cb );
}


bool BatchProgram::writeStatus( char tag , int status, bool force )
{
    if( !usesock_ ) return true;

    if( !force && (Time_getMilliSeconds() - timestamp_ < 1000 ) )
	return true;

    timestamp_ = Time_getMilliSeconds();

    if( Socket* sock = mkSocket() )
    {
	sock->writetag( tag, jobid_, status );

	bool ret = true;

	char masterinfo;
	ret = sock->readtag( masterinfo );
	if ( masterinfo != mRSP_ACK )
	{
	    // TODO : handle requests from master
	    if( masterinfo == mRSP_REQ_STOP ) exit( 0 );

	    ret = false;
	}

	delete sock;
	return ret;
    }

    return false;

}


Socket* BatchProgram::mkSocket()
{
    if( !usesock_ ) return 0;

    return new Socket( masterhost_, masterport_ );
}

bool BatchProgram::initOutput()
{
    stillok_ = false;
    if ( !writeStatus( mPID_TAG, getPID(), true ) )
	exit( 0 );

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
