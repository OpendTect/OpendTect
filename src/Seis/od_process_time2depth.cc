/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "batchprog.h"
#include "process_time2depth.h"

#include "trckeyzsampling.h"
#include "ioman.h"
#include "iopar.h"
#include "ioobj.h"
#include "multiid.h"
#include "progressmeter.h"
#include "seisioobjinfo.h"
#include "seiszaxisstretcher.h"
#include "survinfo.h"
#include "timedepthconv.h"
#include "moddepmgr.h"
#include "unitofmeasure.h"

#include "prog.h"


mLoad2Modules("Seis","Well")

bool BatchProgram::doWork( od_ostream& strm )
{
    TrcKeyZSampling outputcs;
    if ( !outputcs.hsamp_.usePar( pars() ) )
    { outputcs.hsamp_.init( true ); }

    if ( !pars().get( SurveyInfo::sKeyZRange(), outputcs.zsamp_ ) )
    {
	strm << "Cannot read output sampling";
	return false;
    }


    MultiID inputmid;
    if ( !pars().get( ProcessTime2Depth::sKeyInputVolume(), inputmid) )
    {
	strm << "Cannot read input volume id";
	return false;
    }

    PtrMan<IOObj> inputioobj = IOM().get( inputmid );
    if ( !inputioobj )
    {
	strm << "Cannot read input volume object";
	return false;
    }

    MultiID outputmid;
    if ( !pars().get( ProcessTime2Depth::sKeyOutputVolume(), outputmid ) )
    {
	strm << "Cannot read output volume id";
	return false;
    }

    PtrMan<IOObj> outputioobj = IOM().get( outputmid );
    if ( !outputioobj )
    {
	strm << "Cannot read output volume object";
	return false;
    }

    PtrMan<IOPar> ztranspar =
	pars().subselect( ProcessTime2Depth::sKeyZTransPar() );

    if ( !ztranspar )
    {
	strm << "Cannot find tranformation parameters.";
	return false;
    }

    RefMan<ZAxisTransform> ztransform = ZAxisTransform::create( *ztranspar );
    if ( !ztransform )
    {
	strm << "Cannot construct transform.";
	return false;
    }

    if ( !ztransform->isOK() )
    {
	strm << "Velocity model is not usable";
	return false;
    }


    bool istime2depth;
    if ( !pars().getYN(ProcessTime2Depth::sKeyIsTimeToDepth(),istime2depth) )
    {
	strm << "Cannot read direction";
	return false;
    }

    VelocityDesc veldesc;
    const bool isvel = veldesc.usePar( inputioobj->pars() ) &&
			veldesc.isVelocity();
    const bool isvrms = veldesc.type_ == VelocityDesc::RMS;
    if ( isvel )
    {
	//would we convert Thomsen? nothing prepared for this now
	strm << "\nDetected that the stretching will be done on velocities.\n"
		"Will stretch in z-domain and convert back to velocities.\n";
	if ( isvrms )
	{
	    strm << "\nDetected that the input cube contains RMS velocities.\n"
		"RMS velocities are not present in Depth domain;\n"
		"a conversion to interval velocities will thus be processed.\n";
	}
    }

    const UnitOfMeasure* uom = nullptr;
    if ( !veldesc.velunit_.isEmpty() )
	uom = UoMR().get( veldesc.velunit_ );

    if ( !uom )
	uom = UnitOfMeasure::surveyDefVelUnit();

    ztransform->setVelUnitOfMeasure( &uom->scaler() );
    TaskGroup taskgrp;
    const SeisIOObjInfo seisinfo( inputmid );
    const bool is2d = seisinfo.is2D();
    if ( is2d )
    {
	TypeSet<Pos::GeomID> geomids;
	TypeSet< StepInterval<int> > trcrgs;
	for ( int idx=0; ; idx++ )
	{
	    PtrMan<IOPar> linepar =
		pars().subselect( IOPar::compKey(sKey::Line(),idx) );
	    if ( !linepar )
		break;

	    Pos::GeomID geomid;
	    linepar->get( sKey::GeomID(), geomid );
	    geomids += geomid;

	    StepInterval<int> trcrg;
	    linepar->get( sKey::TrcRange(), trcrg );
	    trcrgs += trcrg;
	}

	for ( int idx=0; idx<geomids.size(); idx++ )
	{
	    outputcs.hsamp_.init( geomids[idx] );
	    outputcs.hsamp_.setTrcRange( trcrgs[idx] );
	    SeisZAxisStretcher* exec =
		new SeisZAxisStretcher( *inputioobj, *outputioobj, outputcs,
					*ztransform, istime2depth, isvel );
	    exec->setGeomID( geomids[idx] );
	    exec->setName( BufferString("Time to depth conversion - ",
					Survey::GM().getName(geomids[idx])) );
	    if ( isvel )
	    {
		exec->setVelTypeIsVint( veldesc.type_==VelocityDesc::Interval );
		if ( isvrms )
		    exec->setVelTypeIsVrms( isvrms );
	    }

	    taskgrp.addTask( exec );
	}
    }
    else
    {
	SeisZAxisStretcher* exec =
		new SeisZAxisStretcher( *inputioobj, *outputioobj, outputcs,
					*ztransform, istime2depth, isvel );
	exec->setName( "Time to depth conversion");
	if ( !exec->isOK() )
	{
	    strm << "Cannot initialize readers/writers";
	    return false;
	}

	if ( isvel )
	{
	    exec->setVelTypeIsVint( veldesc.type_ == VelocityDesc::Interval );
	    if ( isvrms )
		exec->setVelTypeIsVrms( isvrms );
	}

	taskgrp.addTask( exec );
    }

    TextStreamProgressMeter progressmeter( strm );
    ((Task&)taskgrp).setProgressMeter( &progressmeter );
    return taskgrp.execute();
}
