#include "seistrc.h"
#include "seiscbvs.h"
#include "seistrcsel.h"
#include "binidselimpl.h"
#include "conn.h"
#include "iostrm.h"
#include "separstr.h"
#include "strmprov.h"
#include <iostream.h>
#include <math.h>

#include "prog.h"
defineTranslatorGroup(SeisTrc,"Seismic Data");
defineTranslator(CBVS,SeisTrc,"CBVS");


int main( int argc, char** argv )
{
    if ( argc < 2 )
    {
	cerr << "Usage: " << argv[0] << " in1`inl2`crl1`crl2[`startz`stepz`nrz]"
	     << endl;
	cerr << "input/output are stdin and stdout. Format: CBVS." << endl;
	return 1;
    }

    CBVSSeisTrcTranslator tri;
    StreamConn inconn( cin );
    IOStream inioobj( "cin" );
    inioobj.setFileName( StreamProvider::sStdIO );
    inconn.ioobj = &inioobj;
    if ( !tri.initRead(inconn) )
	{ cerr << tri.errMsg() << endl;  return 1; }

    FileMultiString fms( argv[1] );
    SeisTrcSel tsel;
    BinIDRange* bidrg = new BinIDRange;
    tsel.bidsel = bidrg;
    bidrg->start = BinID( atoi(fms[0]), atoi(fms[2]) );
    bidrg->stop = BinID( atoi(fms[1]), atoi(fms[3]) );
    tri.setTrcSel( &tsel );

    if ( fms.size() > 4 )
    {
	SamplingData<float> sd( atof(fms[4]), atof(fms[5]) );
	int nrz = atoi( fms[6] );
	ObjectSet<SeisTrcTranslator::TargetComponentData>& ci
		= tri.componentInfo();
	const int nrincomp = ci.size();
	for ( int idx=0; idx<nrincomp; idx++ )
	{
	    ci[idx]->sd = sd;
	    ci[idx]->nrsamples = nrz;
	}
    }

    CBVSSeisTrcTranslator tro;
    StreamConn outconn( cout );
    IOStream iostrm( "cout" );
    iostrm.setFileName( StreamProvider::sStdIO );
    outconn.ioobj = &iostrm;

    SeisTrc trc;
    int nrwr = 0;
    while ( tri.read(trc) )
    {
	if ( !nrwr && !tro.initWrite( outconn, trc ) )
	    { cerr << "Cannot start write!" << endl;  return 1; }

	if ( !tro.write(trc) )
	    { cerr << "Cannot write!" << endl;  return 1; }
	nrwr++;
    }

    cerr << nrwr << " traces written.";
    return nrwr ? 0 : 1;
}
