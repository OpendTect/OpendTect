#include "seistrc.h"
#include "seiscbvs.h"
#include "seistrcsel.h"
#include "binidselimpl.h"
#include "conn.h"
#include "iostrm.h"
#include "separstr.h"
#include "strmprov.h"
#include "filegen.h"
#include <fstream>
#include <math.h>

#include "prog.h"
defineTranslatorGroup(SeisTrc,"Seismic Data");
defineTranslator(CBVS,SeisTrc,"CBVS");


int main( int argc, char** argv )
{
    if ( argc < 3 )
    {
	cerr << "Usage: " << argv[0]
	     << " inpfile outpfile "
	     << "[inl1`inl2`inlstep`crl1`crl2`crlstep[`startz`stepz`nrz]]\n";
	cerr << "Format input: CBVS ; Format ouput: x y z v [v ...]" << endl;
	return 1;
    }
    else if ( !File_exists(argv[1]) )
    {
        cerr << argv[1] << " does not exist" << endl;
        return 1;
    }

    FileNameString fname( argv[1] );
    if ( !File_isAbsPath(argv[1]) )
    {
	fname = File_getCurrentDir();
	fname = File_getFullPath( fname, argv[1] );
    }
    CBVSSeisTrcTranslator tri;
    if ( !tri.initRead( new StreamConn(fname,Conn::Read) ) )
	{ cerr << tri.errMsg() << endl;  return 1; }

    fname = argv[2];
    ofstream outstrm( argv[2] );
    if ( !outstrm )
        { cerr << "Cannot open output file" << endl;  return 1; }

    if ( argc > 3 )
    {
	FileMultiString fms( argv[3] );
	SeisTrcSel tsel;
	BinIDSampler* bidsmpl = new BinIDSampler;
	tsel.bidsel = bidsmpl;
	bidsmpl->start = BinID( atoi(fms[0]), atoi(fms[3]) );
	bidsmpl->stop = BinID( atoi(fms[1]), atoi(fms[4]) );
	bidsmpl->step = BinID( atoi(fms[2]), atoi(fms[5]) );
	tri.setTrcSel( &tsel );
	if ( fms.size() > 6 )
	{
	    SamplingData<float> sd( atof(fms[6]), atof(fms[7]) );
	    int nrz = atoi( fms[8] );
	    ObjectSet<SeisTrcTranslator::TargetComponentData>& ci
		    = tri.componentInfo();
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
    while ( tri.read(trc) )
    {
	const int nrcomps = trc.data().nrComponents();
	const int nrsamps = trc.size( 0 );
	Coord coord = trc.info().coord;
	for ( int isamp=0; isamp<nrsamps; isamp++ )
	{
	    for ( int icomp=0; icomp<nrcomps; icomp++ )
	    {
		outstrm << coord.x << ' ' << coord.y << ' '
		        << trc.samplePos(isamp,icomp) << ' '
			<< trc.get(isamp,icomp) << '\n';
		nrlwr++;
	    }
	}
	nrwr++;
    }

    cerr << nrwr << " traces written to " << nrlwr << " lines." << endl;
    return nrwr ? 0 : 1;
}
