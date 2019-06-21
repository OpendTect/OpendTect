/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : May 2016
-*/

#include "volprocprocessor.h"

#include "executor.h"
#include "iopar.h"
#include "keystrs.h"
#include "dbkey.h"
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

    int nrgeoms = 0;
    if ( !subselpar->get(sKey::NrGeoms(),nrgeoms) )
    {
	//The IOPar is from OD6.0 or older
	VolProc::ChainOutput vco;
	vco.usePar( procpars_ );
	return vco.go( strm );
    }

    DBKey chainid, outid;
    PtrMan<IOPar> chainpar = procpars_.subselect( sKey::Chain() );
    if ( !chainpar &&
	 !procpars_.get(VolProcessingTranslatorGroup::sKeyChainID(),chainid) )
    {
	strm << "\nNo Volume Processing chain found\n";
	return false;
    }

    if ( !procpars_.get("Output.0.Seismic.ID",outid) )
    {
	strm << "\nNo Output ID found\n";
	return false;
    }

    if ( nrgeoms > 1 )
	strm << "\nNumber of Geometries to process: " << nrgeoms << od_endl;

    for ( int idx=0; idx<nrgeoms; idx++ )
    {
	PtrMan<IOPar> geompar = subselpar->subselect( idx );
	if ( !geompar && nrgeoms>1 )
	    return false;

	TrcKeyZSampling tkzs;
	tkzs.usePar( geompar ? *geompar : *subselpar );
	const BufferString geomname = tkzs.hsamp_.getGeomID().name();
	if ( tkzs.is2D() )
	    strm << "\nProcessing on Line " << geomname << od_endl;
	else if ( nrgeoms > 1 )
	    strm << "\nProcessing for 3D survey " << geomname << od_endl;

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

    int nrgeoms = 0;
    if ( !subselpar->get(sKey::NrGeoms(),nrgeoms) )
    {
	//The IOPar is from OD6.0 or older
	VolProc::ChainOutput vco;
	vco.usePar( procpars_ );
	return TaskRunner::execute( tskr, vco );
    }

    DBKey chainid, outid;
    if ( !procpars_.get(VolProcessingTranslatorGroup::sKeyChainID(),chainid)
	    || !procpars_.get("Output.0.Seismic.ID",outid) )
	return false;

    ExecutorGroup taskgrp( "Volume Processing" );
    for ( int idx=0; idx<nrgeoms; idx++ )
    {
	PtrMan<IOPar> geompar = subselpar->subselect( idx );
	if ( !geompar )
	    return false;

	TrcKeyZSampling tkzs;
	tkzs.usePar( *geompar );
	VolProc::ChainOutput* vco = new VolProc::ChainOutput;
	vco->setChainID( chainid );
	vco->setOutputID( outid );
	vco->setTrcKeyZSampling( tkzs );
	taskgrp.add( vco );
    }

    return nrgeoms ? TaskRunner::execute( tskr, taskgrp ) : false;
}
