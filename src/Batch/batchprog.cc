/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 14-9-1998
 * FUNCTION : Batch Program 'driver'
-*/
 
static const char* rcsID = "$Id: batchprog.cc,v 1.1.1.2 1999-09-16 09:32:49 arend Exp $";

#include "batchprog.h"
#include "ioparlist.h"
#include "strmprov.h"
#include "strmdata.h"
#include "filegen.h"

#include <stdiostream.h>
#include <unistd.h>


BatchProgram* BatchProgram::inst_;

int Execute_batch( int* pargc, char** argv )
{
    BatchProgram::inst_ = new BatchProgram( pargc, argv );
    if ( !BP().stillok_ )
	return 1;

    if ( BP().inbg_ )
    {
	switch ( fork() )
	{
	case -1:
	    cerr << argv[0] <<  "cannot fork:\n" << errno_message() << endl;
	case 0: break;
	default:
	    return 0;
	break;
	}
    }

    int res = BatchProgram::inst_->go(
		*BatchProgram::inst_->sdout_.ostrm ) ? 0 : 1;
    delete BatchProgram::inst_;
    return res;
}


BatchProgram::BatchProgram( int* pac, char** av )
	: UserIDObject("")
	, pargc_(pac)
	, argv_(av)
	, argshift_(2)
	, stillok_(NO)
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
    stillok_ = initOutput();
}


BatchProgram::~BatchProgram()
{
    sdout_.close();
    delete &sdout_;
}


const char* BatchProgram::progName() const
{
    static UserIDString ret;
    ret = File_getFileName( fullpath_ );
    return ret;
}


bool BatchProgram::initOutput()
{
    const char* res = pars()["Log file"];
    if ( !*res || !strcmp(res,"stdout") ) res = 0;
 
    if ( res && !strcmp(res,"window") )
    {
	FileNameString comm( "disp_text " );
	comm += name();
	sdout_.fp = popen( (char*)comm, "w" );
	if ( !sdout_.fp )
	{
	    cerr << name() << ": Cannot open window for output" << endl;
	    cerr << "Using std output instead" << endl;
	    res = 0;
	}
	else
	{
	    sdout_.sb = new stdiobuf( sdout_.fp );
	    sdout_.ostrm = new ostream( sdout_.sb );
	}
    }
 
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
