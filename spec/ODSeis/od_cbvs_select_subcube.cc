#include "seistrc.h"
#include "seiscbvs.h"
#include "seistrcsel.h"
#include "binidselimpl.h"
#include "conn.h"
#include "iostrm.h"
#include "separstr.h"
#include "strmprov.h"
#include "filegen.h"
#include <iostream.h>
#include <math.h>

#include "prog.h"
defineTranslatorGroup(SeisTrc,"Seismic Data");
defineTranslator(CBVS,SeisTrc,"CBVS");


int main( int argc, char** argv )
{
    if ( argc < 4 )
    {
	cerr << "Usage: " << argv[0]
	     << " inl1`inl2`inlstep`crl1`crl2`crlstep[`startz`stepz`nrz]"
	     << " inpfile outpfile" << endl;
	cerr << "Format: CBVS." << endl;
	return 1;
    }
    else if ( !File_exists(argv[2]) )
    {
        cerr << argv[2] << " does not exist" << endl;
        return 1;
    }

    FileNameString fname( argv[2] );
    if ( !File_isAbsPath(argv[2]) )
        fname = File_getFullPath( ".", argv[2] );
    CBVSSeisTrcTranslator tri;
    StreamConn inconn( fname, Conn::Read );
    IOStream ioobj( "tmp" );
    ioobj.setFileName( fname );
    inconn.ioobj = &ioobj;
    if ( !tri.initRead(inconn) )
        { cerr << tri.errMsg() << endl;  return 1; }

    fname = argv[3];
    if ( !File_isAbsPath(argv[3]) )
        fname = File_getFullPath( ".", argv[3] );
    CBVSSeisTrcTranslator tro;
    StreamConn outconn( fname, Conn::Write );
    ioobj.setFileName( fname );
    outconn.ioobj = &ioobj;

    FileMultiString fms( argv[1] );
    SeisTrcSel tsel;
    BinIDSampler* bidsmpl = new BinIDSampler;
    tsel.bidsel = bidsmpl;
    bidsmpl->start = BinID( atoi(fms[0]), atoi(fms[3]) );
    bidsmpl->stop = BinID( atoi(fms[1]), atoi(fms[4]) );
    bidsmpl->step = BinID( atoi(fms[2]), atoi(fms[5]) );
    tri.setTrcSel( &tsel );
    ObjectSet<SeisTrcTranslator::TargetComponentData>& ci
	    = tri.componentInfo();
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

    SeisTrc trc;
    int nrwr = 0;
    while ( tri.read(trc) )
    {
	if ( !nrwr )
	{
	    tro.packetInfo() = tri.packetInfo();
	    if ( !tro.initWrite( outconn, trc ) )
		{ cerr << "Cannot start write!" << endl;  return 1; }
	    for ( int idx=0; idx<nrincomp; idx++ )
		tro.componentInfo()[idx]->setName( ci[idx]->name() );
	}

	if ( !tro.write(trc) )
	    { cerr << "Cannot write!" << endl;  return 1; }
	nrwr++;
    }

    cerr << nrwr << " traces written.";
    return nrwr ? 0 : 1;
}
