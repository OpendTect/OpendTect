/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 2000
-*/

static const char* rcsID = "$Id";

#include "seistrc.h"
#include "seiscbvs.h"
#include "seistrcsel.h"
#include "seisresampler.h"
#include "cubesampling.h"
#include "conn.h"
#include "iostrm.h"
#include "separstr.h"
#include "strmprov.h"
#include "filegen.h"
#include "filepath.h"
#include "ptrman.h"
#include <iostream>
#include <math.h>

#include "prog.h"
#include "seisfact.h"


static int doWork( int argc, char** argv )
{
    if ( argc < 4 )
    {
	std::cerr << "Usage: " << argv[0]
	     << " inl1,inl2,inlstep,crl1,crl2,crlstep,startz,stopz,stepz"
	     << " inpfile outpfile [min_val,max_val]" << std::endl;
	std::cerr << "Format: CBVS." << std::endl;
	return 1;
    }
    else if ( !File_exists(argv[2]) )
    {
	std::cerr << argv[2] << " does not exist" << std::endl;
	return 1;
    }

    BufferString fname( argv[2] );
    FilePath fp( fname );
    if ( !fp.isAbsolute() )
    {
	fp.insert( File_getCurrentDir() );
	fname = fp.fullPath();
    }
    PtrMan<CBVSSeisTrcTranslator> tri = CBVSSeisTrcTranslator::getInstance();
    if ( !tri->initRead(new StreamConn(fname,Conn::Read)) )
        { std::cerr << tri->errMsg() << std::endl; return 1; }

    fname = argv[3];
    fp.set( fname );
    if ( !fp.isAbsolute() )
    {
	fp.insert( File_getCurrentDir() );
	fname = fp.fullPath();
    }
    PtrMan<CBVSSeisTrcTranslator> tro = CBVSSeisTrcTranslator::getInstance();

    SeparString fms( argv[1], ',' );
    CubeSampling cs;
    cs.hrg.start.inl = atoi(fms[0]); cs.hrg.stop.inl = atoi(fms[1]);
    cs.hrg.step.inl = atoi(fms[2]);
    if ( cs.hrg.step.inl < 0 ) cs.hrg.step.inl = -cs.hrg.step.inl;
    cs.hrg.start.crl = atoi(fms[3]); cs.hrg.stop.crl = atoi(fms[4]);
    cs.hrg.step.crl = atoi(fms[5]);
    if ( cs.hrg.step.crl < 0 ) cs.hrg.step.crl = -cs.hrg.step.crl;
    cs.zrg.start = atof(fms[6]); cs.zrg.stop = atof(fms[7]);
    cs.zrg.step = atof(fms[8]);
    if ( cs.zrg.step < 0 ) cs.zrg.step = -cs.zrg.step;
    cs.normalise();

    SeisSelData tsel; tsel.type_ = SeisSelData::Range;
    tsel.inlrg_.start = cs.hrg.start.inl; tsel.inlrg_.stop = cs.hrg.stop.inl;
    tsel.crlrg_.start = cs.hrg.start.crl; tsel.crlrg_.stop = cs.hrg.stop.crl;
    assign( tsel.zrg_, cs.zrg );
    tri->setSelData( &tsel );

    bool haverange = false;
    Interval<float> rg( -mUndefValue, mUndefValue );
    Interval<float>* userg = 0;
    if ( argc > 4 )
    {
	fms = argv[4];
	if ( fms.size() > 1 )
	{
	    userg = &rg;
	    rg.start = atof( fms[0] );
	    rg.stop = atof( fms[1] );
	}
    }

    SeisResampler rsmplr( cs, userg );
    SeisTrc rdtrc;
    while ( tri->read(rdtrc) )
    {
	SeisTrc* trc = rsmplr.get( rdtrc );
	if ( !trc ) continue;

	if ( rsmplr.nrPassed() == 1
	  && !tro->initWrite(new StreamConn(fname,Conn::Write),*trc) )
	    { std::cerr << tro->errMsg() << std::endl; return 1; }
	if ( !tro->write(*trc) )
	    { std::cerr << "Cannot write!" << std::endl; return 1; }
    }

    std::cerr << rsmplr.nrPassed() << " traces written." << std::endl;
    return rsmplr.nrPassed() ? 0 : 1;
}


int main( int argc, char** argv )
{
    return exitProgram( doWork(argc,argv) );
}
