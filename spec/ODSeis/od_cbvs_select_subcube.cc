/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 2000
 * RCS      : $Id: od_cbvs_select_subcube.cc,v 1.30 2010-04-23 05:27:20 cvsnanne Exp $
-*/

static const char* rcsID = "$Id: od_cbvs_select_subcube.cc,v 1.30 2010-04-23 05:27:20 cvsnanne Exp $";

#include "seistrc.h"
#include "seiscbvs.h"
#include "seisselectionimpl.h"
#include "seisresampler.h"
#include "cubesampling.h"
#include "conn.h"
#include "iostrm.h"
#include "separstr.h"
#include "strmprov.h"
#include "file.h"
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

    FilePath fp( argv[2] );
    
    if ( !File::exists(fp.fullPath()) )
    {
        std::cerr << fp.fullPath() << " does not exist" << std::endl;
        return 1;
    }
    
    if ( !fp.isAbsolute() )
    {
        fp.insert( File::getCurrentPath() );
    }

    BufferString fname=fp.fullPath();

    PtrMan<CBVSSeisTrcTranslator> tri = CBVSSeisTrcTranslator::getInstance();
    if ( !tri->initRead(new StreamConn(fname,Conn::Read)) )
        { std::cerr << tri->errMsg() << std::endl; return 1; }

    fp.set( argv[3] ); 
    if ( !fp.isAbsolute() ) { fp.insert( File::getCurrentPath() ); }
    fname = fp.fullPath();

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

    Seis::RangeSelData seldata( cs );
    tri->setSelData( &seldata );

    bool haverange = false;
    Interval<float> rg( -mUdf(float), mUdf(float) );
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

    SeisResampler rsmplr( cs, false, userg );
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
    return ExitProgram( doWork(argc,argv) );
}
