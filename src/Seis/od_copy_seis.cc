/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "batchprog.h"

#include "seiscopy.h"
#include "seissingtrcproc.h"
#include "seisioobjinfo.h"
#include "iopar.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "moddepmgr.h"

#include "prog.h"

mLoad1Module("Seis")

bool BatchProgram::doWork( od_ostream& strm )
{
    PtrMan<IOPar> inpar = pars().subselect( sKey::Input() );
    if ( !inpar || inpar->isEmpty() )
    {
	strm << "Batch parameters 'Input' empty" << od_endl;
	return false;
    }

    PtrMan<IOObj> inioobj = IOM().get( inpar->find(sKey::ID()) );
    if ( !inioobj )
    {
	strm << "Input object spec is not OK" << od_endl;
	return false;
    }

    SeisIOObjInfo ioobjinfo( *inioobj );
    if ( !ioobjinfo.isOK() )
    {
	strm << "Input data is not OK" << od_endl;
	return false;
    }

    else if ( ioobjinfo.isPS() )
    {
	strm << "Pre-Stack data not supported" << od_endl;
	return false;
    }

    PtrMan<IOPar> outpar = pars().subselect( sKey::Output() );
    if ( !outpar || outpar->isEmpty() )
    {
	strm << "Batch parameters 'Ouput' empty" << od_endl;
	return false;
    }

    PtrMan<IOObj> outioobj = IOM().get( outpar->find(sKey::ID()) );
    if ( !outioobj )
    {
	strm << "Output object spec is not OK" << od_endl;
	return false;
    }

    const bool newformat = pars().isPresent( sKey::Geometry() );
    const IOPar& procpars = newformat ? *outpar.ptr() : pars();

    if ( ioobjinfo.is2D() )
    {
	Seis2DCopier copier( *inioobj, *outioobj, procpars );
	return copier.go( &strm, false, true );
    }

    PtrMan<SeisSingleTraceProc> stp = new SeisSingleTraceProc( *inioobj,
			*outioobj, "Copying 3D Cube", nullptr,
			uiString::emptyString() );
    if ( !stp->isOK() )
    {
	strm << stp->errMsg();
	return false;
    }

    stp->setProcPars( procpars, false );
    int compnr = -1; // all components
    inpar->get( sKey::Component(), compnr );
    SeisCubeCopier copier( stp.release(), compnr );
    return copier.go( &strm, false, true );
}
