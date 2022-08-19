/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seistrc.h"
#include "seiscbvs.h"
#include "seisselectionimpl.h"
#include "ioman.h"
#include "ioobj.h"
#include "moddepmgr.h"
#include "ptrman.h"
#include "timefun.h"

#include <iostream>
#include <math.h>

#include "prog.h"
#include "seisioobjinfo.h"
#include "seisparallelreader.h"


int mProgMainFnName( int argc, char** argv )
{
    SetProgramArgs( argc, argv );
    if ( argc < 2 )
    {
	std::cerr << "Usage: " << argv[0]
	     << " objectid method\n";
	std::cerr << "method: 0-parallel 1-sequential"
		  << std::endl;
	return 1;
    }

    OD::ModDeps().ensureLoaded( "Seis" );
    const MultiID seismid( argv[1] );
    PtrMan<IOObj> ioobj = IOM().get( seismid );
    if ( !ioobj )
    {
	std::cerr << "Cannot read seismic data with ID " << seismid.buf()
		  << std::endl;
	return 1;
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
	Seis::ParallelReader rdr( *ioobj, tkzs );
	rdr.execute();
    }
    else
    {
	Seis::SequentialReader rdr( *ioobj );
	rdr.execute();
    }

    std::cerr << "Total time: " << counter.elapsed() << std::endl;

    return 0;
}
