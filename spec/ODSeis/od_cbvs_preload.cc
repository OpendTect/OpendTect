/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 2000
 * RCS      : $Id$
-*/

#include "seistrc.h"
#include "seiscbvs.h"
#include "seisselectionimpl.h"
#include "dbman.h"
#include "ioobj.h"
#include "moddepmgr.h"
#include "ptrman.h"
#include "timefun.h"

#include <iostream>
#include <math.h>

#include "prog.h"
#include "seisbuf.h"
#include "seisioobjinfo.h"
#include "seisloader.h"
#include "seisprovider.h"


int main( int argc, char** argv )
{
    SetProgramArgs( argc, argv );
    if ( argc < 2 )
    {
	std::cerr << "Usage: " << argv[0]
	     << " objectid method\n";
	std::cerr << "method: 0-parallel 1-sequential 2-trcbuf"
		  << std::endl;
	ExitProgram( 1 );
    }

    OD::ModDeps().ensureLoaded( "Seis" );
    const DBKey seismid = DBKey::getFromString( argv[1] );
    PtrMan<IOObj> ioobj = DBM().get( seismid );
    if ( !ioobj )
    {
	std::cerr << "Cannot read seismic data with ID " << seismid.toString()
		  << std::endl;
	ExitProgram( 1 );
    }

    std::cerr << "Preloading " << ioobj->name() << std::endl;
    SeisIOObjInfo info( *ioobj );
    TrcKeyZSampling tkzs; tkzs.init();
    info.getRanges( tkzs );

    const int method = atoi( argv[2] );

    Time::Counter counter;
    counter.start();
    if ( method==0 )
    {
	Seis::ParallelFSLoader3D rdr( *ioobj, tkzs );
	rdr.execute();
    }
    else if ( method==1 )
    {
	Seis::SequentialFSLoader rdr( *ioobj, &tkzs );
	rdr.execute();
    }
    else
    {
	SeisTrcBuf sbuf( true );
	uiRetVal uirv;
	Seis::Provider* prov = Seis::Provider::create( ioobj->key(), &uirv );
	if ( !prov )
	{
	    std::cerr << uirv.getText().buf() << std::endl;
	    ExitProgram( 1 );
	}

	prov->setSelData( new Seis::RangeSelData(tkzs) );
	SeisBufReader bufrdr( *prov, sbuf );
	bufrdr.execute();
    }

    std::cerr << "Total time: " << counter.elapsed() << std::endl;

    ExitProgram( 0 ); return 0;
}

