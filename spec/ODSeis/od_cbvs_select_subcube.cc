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
#include "ptrman.h"
#include <iostream>
#include <math.h>

#include "prog.h"
#include "seisfact.h"


int main( int argc, char** argv )
{
    if ( argc < 4 )
    {
	cerr << "Usage: " << argv[0]
	     << " inl1,inl2,inlstep,crl1,crl2,crlstep[,startz,stepz,nrz]"
	     << " inpfile outpfile [min_val,max_val]" << endl;
	cerr << "Format: CBVS." << endl;
	cerr << "Parameters between brackets [] are optional, do not type the brackets!" << endl;
	exitProgram( 1 );
    }
    else if ( !File_exists(argv[2]) )
    {
        cerr << argv[2] << " does not exist" << endl;
	exitProgram( 1 );
    }

    FileNameString fname( argv[2] );
    if ( !File_isAbsPath(argv[2]) )
    {
	fname = File_getCurrentDir();
	fname = File_getFullPath( fname, argv[2] );
    }
    PtrMan<CBVSSeisTrcTranslator> tri = CBVSSeisTrcTranslator::getInstance();
    if ( !tri->initRead(new StreamConn(fname,Conn::Read)) )
        { cerr << tri->errMsg() << endl; exitProgram(1); }

    fname = argv[3];
    if ( !File_isAbsPath(argv[3]) )
    {
	fname = File_getCurrentDir();
	fname = File_getFullPath( fname, argv[3] );
    }
    PtrMan<CBVSSeisTrcTranslator> tro = CBVSSeisTrcTranslator::getInstance();

    SeparString fms( argv[1], ',' );
    SeisTrcSel tsel;
    BinIDSampler* bidsmpl = new BinIDSampler;
    tsel.bidsel = bidsmpl;
    bidsmpl->start = BinID( atoi(fms[0]), atoi(fms[3]) );
    bidsmpl->stop = BinID( atoi(fms[1]), atoi(fms[4]) );
    bidsmpl->step = BinID( atoi(fms[2]), atoi(fms[5]) );
    // tri->setTrcSel( &tsel );
    ObjectSet<SeisTrcTranslator::TargetComponentData>& ci
	    = tri->componentInfo();
    const int nrincomp = ci.size();
    if ( fms.size() > 6 )
    {
	SamplingData<float> sd( atof(fms[6]), atof(fms[7]) );
	int nrz = atoi( fms[8] );
	for ( int idx=0; idx<nrincomp; idx++ )
	{
	    ci[idx]->sd = sd;
	    ci[idx]->nrsamples = nrz;
	}
    }

    bool haverange = false;
    Interval<float> rg( -mUndefValue, mUndefValue );
    if ( argc > 4 )
    {
	fms = argv[4];
	const int sz = fms.size();
	if ( sz > 0 ) { haverange = true; rg.start = atof( fms[0] ); }
	if ( sz > 1 ) { rg.stop = atof( fms[1] ); }
	rg.sort();
    }
    const float replval = mIsUndefined(rg.stop) ? rg.start
			: (rg.start + rg.stop) * .5;

    SeisTrc trc;
    int nrwr = 0;
    while ( tri->read(trc) )
    {
	if ( bidsmpl->excludes(trc.info().binid) )
	    continue;

	if ( !nrwr )
	{
	    tro->packetInfo() = tri->packetInfo();
	    if ( !tro->initWrite(new StreamConn(fname,Conn::Write),trc) )
		{ cerr << "Cannot start write!" << endl; exitProgram(1); }
	    for ( int idx=0; idx<nrincomp; idx++ )
		tro->componentInfo()[idx]->setName( ci[idx]->name() );
	}

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
	    { cerr << "Cannot write!" << endl; exitProgram(1); }
	nrwr++;
    }

    cerr << nrwr << " traces written." << endl;
    exitProgram( nrwr ? 0 : 1 ); return 0;
}
