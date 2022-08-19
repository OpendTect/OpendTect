/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "volprocprocessor.h"

#include "executor.h"
#include "iopar.h"
#include "keystrs.h"
#include "multiid.h"
#include "od_ostream.h"
#include "survgeom.h"
#include "trckeyzsampling.h"
#include "volprocchainoutput.h"
#include "volproctrans.h"

VolProc::Processor::Processor( const IOPar& par )
    : procpars_(par)
{
}


bool VolProc::Processor::run( od_ostream& strm, JobCommunic* comm )
{
    PtrMan<IOPar> subselpar =
	procpars_.subselect( IOPar::compKey(sKey::Output(),sKey::Subsel()) );
    if ( !subselpar )
	return false;

    if ( subselpar->hasKey(sKey::NrGeoms()) )
	strm << "This job is not compatibe with this version of OpendTect.\n"
	    << "Please run a new job" << od_endl;

    PtrMan<IOPar> alllinepars = subselpar->subselect( sKey::Line() );
    if ( !alllinepars ) // 3D
    {
	VolProc::ChainOutput vco;
	vco.usePar( procpars_ );
	vco.setJobCommunicator( comm );
	return vco.go( strm );
    }

    // 2D
    int lineidx = 0;
    while ( true )
    {
	PtrMan<IOPar> linepar = alllinepars->subselect( lineidx++ );
	if ( !linepar )
	    break;

	Pos::GeomID geomid;
	if ( !linepar->get(sKey::GeomID(),geomid) )
	    break;

	TrcKeyZSampling tkzs; tkzs.set2DDef();
	StepInterval<int> trcrg;
	if ( !linepar->get(sKey::TrcRange(),trcrg)
		|| !linepar->get(sKey::ZRange(),tkzs.zsamp_) )
	    break;

	tkzs.hsamp_.set( geomid, trcrg );
	const BufferString linename = Survey::GM().getName( geomid );
	strm << "\nProcessing on Line " << linename << od_endl;

	VolProc::ChainOutput vco;
	vco.usePar( procpars_ );
	vco.setTrcKeyZSampling( tkzs );
	vco.setJobCommunicator( comm );
	if ( !vco.go(strm) )
	    return false;
    }

    return true;
}


bool VolProc::Processor::run( TaskRunner* tskr )
{
    PtrMan<IOPar> subselpar =
	procpars_.subselect( IOPar::compKey(sKey::Output(),sKey::Subsel()) );
    if ( !subselpar )
	return false;

    PtrMan<IOPar> alllinepars = subselpar->subselect( sKey::Line() );
    if ( !alllinepars ) // 3D
    {
	VolProc::ChainOutput vco;
	vco.usePar( procpars_ );
	return TaskRunner::execute( tskr, vco );
    }

    // 2D
    ExecutorGroup taskgrp( "Volume Processing" );
    int lineidx = 0;
    while ( true )
    {
	PtrMan<IOPar> linepar = alllinepars->subselect( lineidx++ );
	if ( !linepar )
	    break;

	Pos::GeomID geomid;
	if ( !linepar->get(sKey::GeomID(),geomid) )
	    break;

	TrcKeyZSampling tkzs; tkzs.set2DDef();
	StepInterval<int> trcrg;
	if ( !linepar->get(sKey::TrcRange(),trcrg)
		|| !linepar->get(sKey::ZRange(),tkzs.zsamp_) )
	    break;

	tkzs.hsamp_.set( geomid, trcrg );
	const BufferString linename = Survey::GM().getName( geomid );

	VolProc::ChainOutput* vco = new VolProc::ChainOutput;
	vco->usePar( procpars_ );
	vco->setTrcKeyZSampling( tkzs );
	taskgrp.add( vco );
    }

    return TaskRunner::execute( tskr, taskgrp );
}
