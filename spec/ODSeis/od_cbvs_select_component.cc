#include "seistrc.h"
#include "seiscbvs.h"
#include "conn.h"
#include "iostrm.h"
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
	cerr << "Usage: " << argv[0] << " component" << endl
	     << "input/output are stdin and stdout. Format: CBVS." << endl
	     << "set component to -1 for polar dip, -2 for azimuth"
	     << " (2 components required!)" << endl;
	return 1;
    }

    const int selcomp = atoi( argv[1] );

    CBVSSeisTrcTranslator tri;
    StreamConn inconn( cin );
    IOStream inioobj( "cin" );
    inioobj.setFileName( StreamProvider::sStdIO );
    inconn.ioobj = &inioobj;
    if ( !tri.initRead(inconn) )
	{ cerr << tri.errMsg() << endl;  return 1; }

    ObjectSet<SeisTrcTranslator::TargetComponentData>& ci = tri.componentInfo();
    const int nrincomp = ci.size();
    if ( selcomp >= 0 )
	for ( int idx=0; idx<nrincomp; idx++ )
	    ci[idx]->destidx = idx == selcomp ? idx : -1;

    CBVSSeisTrcTranslator tro;
    StreamConn outconn( cout );
    IOStream outioobj( "cout" );
    outioobj.setFileName( StreamProvider::sStdIO );
    outconn.ioobj = &outioobj;

    SeisTrc trc;
    SeisTrc& outtrc = selcomp < 0 ? *new SeisTrc : trc;
    int nrwr = 0;
    while ( tri.read(trc) )
    {
	if ( selcomp < 0 )
	{
	    // assume user knows that this is a dip cube:

	    const int trcsz = trc.size(0);
	    if ( !nrwr ) outtrc.reSize( trcsz, 0 );
	    for ( int idx=0; idx<trcsz; idx++ )
	    {
		float inldip = trc.get(idx,0);
		float crldip = trc.get(idx,1);
		outtrc.set( idx, selcomp == -2 ? atan2(inldip,crldip)
			       : sqrt( inldip*inldip + crldip*crldip ), 0 );
	    }
	}

	if ( selcomp < 0 )
	    outtrc.info() = trc.info();

	if ( !nrwr && !tro.initWrite( outconn, outtrc ) )
	    { cerr << "Cannot start write!" << endl;  return 1; }

	if ( !tro.write(outtrc) )
	    { cerr << "Cannot write!" << endl;  return 1; }

	nrwr++;
    }

    cerr << nrwr << " traces written.";
    return nrwr ? 0 : 1;
}
