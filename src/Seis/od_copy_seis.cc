/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2015
-*/


#include "batchprog.h"

#include "seiscopy.h"
#include "seissingtrcproc.h"
#include "seisioobjinfo.h"
#include "iopar.h"
#include "ioobj.h"
#include "keystrs.h"
#include "moddepmgr.h"

#include "prog.h"

mLoad1Module("Seis")

bool BatchProgram::doWork( od_ostream& strm )
{
    PtrMan<IOPar> inpar = pars().subselect( sKey::Input() );
    if ( !inpar || inpar->isEmpty() )
	{ strm << "Batch parameters 'Input' empty" << od_endl; return false; }
    DBKey dbky( inpar->find(sKey::ID()) );
    PtrMan<IOObj> inioobj = dbky.getIOObj();
    if ( !inioobj )
	{ strm << "Input object spec is not OK" << od_endl; return false; }
    SeisIOObjInfo ioobjinfo( *inioobj );
    if ( !ioobjinfo.isOK() )
	{ strm << "Input data is not OK" << od_endl; return false; }
    else if ( ioobjinfo.isPS() )
	{ strm << "Prestack data not supported" << od_endl; return false; }

    PtrMan<IOPar> outpar = pars().subselect( sKey::Output() );
    if ( !outpar || outpar->isEmpty() )
	{ strm << "Batch parameters 'Ouput' empty" << od_endl; return false; }
    dbky = DBKey( outpar->find(sKey::ID()) );
    PtrMan<IOObj> outioobj = dbky.getIOObj();
    if ( !outioobj )
	{ strm << "Output object spec is not OK" << od_endl; return false; }

    if ( ioobjinfo.is2D() )
    {
	Seis2DCopier copier( *inioobj, *outioobj, pars() );
	return copier.go( &strm, false, true );
    }

    SeisSingleTraceProc* stp = new SeisSingleTraceProc( *inioobj, *outioobj,
				"", &pars(), uiString::empty() );
    stp->setProcPars( pars() );
    int compnr = -1; // all components
    inpar->get( sKey::Component(), compnr );
    SeisCubeCopier copier( stp, compnr );
    return copier.go( &strm, false, true );
}
