/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 2000
-*/

static const char* rcsID = "$Id";

#include "seistrc.h"
#include "seiscbvs.h"
#include "seistrcsel.h"
#include "binidselimpl.h"
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


int main( int argc, char** argv )
{
    if ( argc < 4 )
    {
	std::cerr << "Usage: " << argv[0]
	     << " inl1,inl2,inlstep,crl1,crl2,crlstep,startz,stopz,stepz"
	     << " inpfile outpfile [min_val,max_val]" << std::endl;
	std::cerr << "Format: CBVS." << std::endl;
	exitProgram( 1 );
    }
    else if ( !File_exists(argv[2]) )
    {
	std::cerr << argv[2] << " does not exist" << std::endl;
	exitProgram( 1 );
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
        { std::cerr << tri->errMsg() << std::endl; exitProgram(1); }

    fname = argv[3];
    fp.set( fname );
    if ( !fp.isAbsolute() )
    {
	fp.insert( File_getCurrentDir() );
	fname = fp.fullPath();
    }
    PtrMan<CBVSSeisTrcTranslator> tro = CBVSSeisTrcTranslator::getInstance();

    SeparString fms( argv[1], ',' );
    SeisSelData tsel;
    tsel.inlrg_.start = atoi(fms[0]); tsel.inlrg_.stop = atoi(fms[1]);
    tsel.inlrg_.step = atoi(fms[2]);
    tsel.crlrg_.start = atoi(fms[3]); tsel.crlrg_.stop = atoi(fms[4]);
    tsel.crlrg_.step = atoi(fms[5]);
    tsel.zrg_.start = atof(fms[6]); tsel.zrg_.stop = atof(fms[7]);
    tsel.zrg_.step = atof(fms[8]);
    tri->setSelData( &tsel );

    bool haverange = false;
    Interval<float> rg( -mUndefValue, mUndefValue );
    if ( argc > 4 )
    {
	fms = argv[4];
	if ( fms.size() > 1 )
	{
	    haverange = true;
	    rg.start = atof( fms[0] );
	    rg.stop = atof( fms[1] );
	}
	rg.sort();
    }
    const float replval = (rg.start + rg.stop) * .5;

    SeisTrc trc;
    int nrwr = 0;
    while ( tri->read(trc) )
    {
	if ( haverange )
	{
	    const int nrcomp = trc.data().nrComponents();
	    for ( int icomp=0; icomp<nrcomp; icomp++ )
	    {
		const int sz = trc.size( icomp );
		for ( int idx=0; idx<sz; idx++ )
		{
		    float v = trc.get( idx, icomp );
		    if ( !isFinite(v) ) v = replval;
		    if ( v < rg.start ) v = rg.start;
		    if ( v > rg.stop ) v = rg.stop;
		    trc.set( idx, v, icomp );
		}
	    }
	}

	if ( !tro->write(trc) )
	    { std::cerr << "Cannot write!" << std::endl; exitProgram(1); }
	nrwr++;
    }

    std::cerr << nrwr << " traces written." << std::endl;
    exitProgram( nrwr ? 0 : 1 ); return 0;
}
