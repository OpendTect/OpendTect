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
#include "filegen.h"
#include "filepath.h"
#include "ptrman.h"
#include "strmprov.h"
#include <iostream>
#include <math.h>

#include "prog.h"
#include "seisfact.h"


int main( int argc, char** argv )
{
    if ( argc < 3 )
    {
	std::cerr << "Usage: " << argv[0]
	     << " inpfile outpfile "
	     << "[inl1,inl2,inlstep,crl1,crl2,crlstep[,startz,stepz,nrz]]\n";
	std::cerr << "Format input: CBVS ; Format ouput: x y z v [v ...]"
		  << std::endl;
	exitProgram( 1 );
    }
    else if ( !File_exists(argv[1]) )
    {
	std::cerr << argv[1] << " does not exist" << std::endl;
	exitProgram( 1 );
    }

    BufferString fname( argv[1] );
    FilePath fp( fname );
    if ( !fp.isAbsolute() )
    {
	fp.insert( File_getCurrentDir() );
	fname = fp.fullPath();
    }
    PtrMan<CBVSSeisTrcTranslator> tri = CBVSSeisTrcTranslator::getInstance();
    if ( !tri->initRead( new StreamConn(fname,Conn::Read) ) )
	{ std::cerr << tri->errMsg() << std::endl; exitProgram( 1 ); }

    fname = argv[2];
    StreamData outsd = StreamProvider( argv[2] ).makeOStream();
    if ( !outsd.usable() )
        { std::cerr << "Cannot open output file" << std::endl; exitProgram(1); }

    if ( argc > 3 )
    {
	SeparString fms( argv[3], ',' );
	SeisTrcSel tsel;
	BinIDSampler* bidsmpl = new BinIDSampler;
	tsel.bidsel = bidsmpl;
	bidsmpl->start = BinID( atoi(fms[0]), atoi(fms[3]) );
	bidsmpl->stop = BinID( atoi(fms[1]), atoi(fms[4]) );
	bidsmpl->step = BinID( atoi(fms[2]), atoi(fms[5]) );
	tri->setTrcSel( &tsel );
	if ( fms.size() > 6 )
	{
	    SamplingData<float> sd( atof(fms[6]), atof(fms[7]) );
	    int nrz = atoi( fms[8] );
	    ObjectSet<SeisTrcTranslator::TargetComponentData>& ci
		    = tri->componentInfo();
	    const int nrincomp = ci.size();
	    for ( int idx=0; idx<nrincomp; idx++ )
	    {
		ci[idx]->sd = sd;
		ci[idx]->nrsamples = nrz;
	    }
	}
    }

    SeisTrc trc;
    int nrwr = 0;
    int nrlwr = 0;
    while ( tri->read(trc) )
    {
	const int nrcomps = trc.data().nrComponents();
	const int nrsamps = trc.size( 0 );
	Coord coord = trc.info().coord;
	for ( int isamp=0; isamp<nrsamps; isamp++ )
	{
	    for ( int icomp=0; icomp<nrcomps; icomp++ )
	    {
		*outsd.ostrm << coord.x << ' ' << coord.y << ' '
			     << trc.samplePos(isamp,icomp) << ' '
			     << trc.get(isamp,icomp) << '\n';
		nrlwr++;
	    }
	}
	nrwr++;
    }

    std::cerr << nrwr << " traces written to " << nrlwr << " lines."
		<< std::endl;
    exitProgram( nrwr ? 0 : 1 ); return 0;
}
