/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "batchprog.h"
#include "velocityvolumeconversion.h"

#include "ioman.h"
#include "iopar.h"
#include "ioobj.h"
#include "multiid.h"
#include "moddepmgr.h"
#include "progressmeter.h"
#include "seisioobjinfo.h"
#include "survinfo.h"
#include "uistrings.h"
#include "unitofmeasure.h"
#include "veldesc.h"

#include "prog.h"

#define mErrRet( msg ) { strm << msg; return false; }

mLoad1Module("Velocity")

bool BatchProgram::doWork( od_ostream& strm )
{
    MultiID inputmid;
    if ( !pars().get( Vel::VolumeConverter::sKeyInput(), inputmid) )
	mErrRet( "Cannot read input volume id" )

    PtrMan<IOObj> inputioobj = IOM().get( inputmid );
    if ( !inputioobj )
	mErrRet( "Cannot read input volume object" )

    TrcKeySampling tks;
    if ( !tks.usePar(pars()) )
    {
	const SeisIOObjInfo seisinfo( inputioobj );
	if ( !seisinfo.isOK() )
	    mErrRet( "Cannot determine input dataset range" )

	if ( seisinfo.is2D() )
	    { pFreeFnErrMsg("Probably incorrect, must provide a GeomID"); }

	TrcKeyZSampling cs;
	seisinfo.getRanges( cs );
	tks = cs.hsamp_;
    }

    MultiID outputmid;
    if ( !pars().get( Vel::VolumeConverter::sKeyOutput(), outputmid ) )
	mErrRet( "Cannot read output volume id" )

    PtrMan<IOObj> outputioobj = IOM().get( outputmid );
    if ( !outputioobj )
	mErrRet( "Cannot read output volume object" )

    VelocityDesc veldesc;
    if ( !veldesc.usePar(pars()) )
	mErrRet( "Cannot read output velocity definition" )

    const double srd = SI().seismicReferenceDatum();
    const UnitOfMeasure* srduom = UnitOfMeasure::surveyDefSRDStorageUnit();
    Vel::VolumeConverterNew conv( *inputioobj, *outputioobj, tks, veldesc,
				  srd, srduom );
    TextStreamProgressMeter progressmeter( strm );
    ((Task&)conv).setProgressMeter( &progressmeter );

    if ( !conv.execute() )
	mErrRet( ::toString(conv.uiMessage()) )

    return true;
}
