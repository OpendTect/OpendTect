/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 14-9-1998
 * FUNCTION : Batch Program 'driver'
-*/
 
static const char* rcsID = "$Id: batchprog.cc,v 1.12 2002-04-15 15:29:33 bert Exp $";

#include "batchprog.h"
#include "ioparlist.h"
#include "strmprov.h"
#include "strmdata.h"
#include "filegen.h"
#include "timefun.h"
#include "sighndl.h"
#ifndef __msvc__
#include <unistd.h>
#endif


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
	    return 0;
	break;
	}
#endif
    }

    bool allok = BatchProgram::inst_->initOutput();
    if ( allok )
    {
	allok = BatchProgram::inst_->go( *BatchProgram::inst_->sdout_.ostrm );
    }
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
    StreamProvider spin( fn );
    StreamData sdin = spin.makeIStream();
    if ( !sdin.istrm )
    {
        cerr << name() << ": Cannot open parameter file" << endl;
        return;
    }
 
    IOParList parlist( *sdin.istrm );
    if ( parlist.size() == 0 )
    {
        cerr << argv_[0] << ": Invalid input file" << endl;
        return;
    }
    sdin.close();

    iopar_ = new IOPar( *parlist[0] );
    stillok_ = true;
}


BatchProgram::~BatchProgram()
{
    writePid( -1 );
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
    writePid( -1 );
}


bool BatchProgram::writePid( int pid )
{
    const char* res = pars()[ "Process info file" ];
    if ( *res )
    {
	StreamData sd = StreamProvider(res).makeOStream();
	if ( !sd.usable() )
	{
	    cerr << name() << ": Cannot write process ID file:\n"
		 << res << endl;
	    return false;
	}
	*sd.ostrm << pid << endl;
	sd.close();
	CallBack cb( mCB(this,BatchProgram,progKilled) );
	if ( pid != -1 )
	    SignalHandling::startNotify( SignalHandling::Kill, cb );
	else
	    SignalHandling::stopNotify( SignalHandling::Kill, cb );
    }
    return true;
}


bool BatchProgram::initOutput()
{
    if ( !writePid(getPID()) )
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

    return sdout_.usable();
}
