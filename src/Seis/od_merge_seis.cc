/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "batchprog.h"

#include "seismerge.h"
#include "iopar.h"
#include "keystrs.h"
#include "moddepmgr.h"
#include "scaler.h"

#include "prog.h"

mLoad1Module("Seis")

bool BatchProgram::doWork( od_ostream& strm )
{
    PtrMan<IOPar> allinpars = pars().subselect( sKey::Input() );
    if ( !allinpars || allinpars->isEmpty() )
    {
	strm << "Batch parameters 'Input' empty" << od_endl;
	return false;
    }

    ObjectSet<IOPar> inpars;
    for ( int idx=0; ; idx++ )
    {
	IOPar* inpar = allinpars->subselect( idx );
	if ( !inpar )
	    break;

	inpars += inpar;
    }

    if ( inpars.size() < 2 )
    {
	strm << "At least two input cubes are required" << od_endl;
	deepErase( inpars );
	return false;
    }

    PtrMan<IOPar> outpar = pars().subselect( sKey::Output() );
    if ( !outpar || outpar->isEmpty() )
    {
	strm << "Batch parameters 'Output' empty" << od_endl;
	deepErase( inpars );
	return false;
    }

    SeisMerger merger( inpars, *outpar, false );
    deepErase( inpars );

    bool stacktrcs = true;
    pars().getYN( "Stack", stacktrcs );
    merger.stacktrcs_ = stacktrcs;
    merger.setScaler( Scaler::get(outpar->find(sKey::Scale())) );
    return merger.go( &strm, false, true );
}
