/*+
 *(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 *Author:	A. Huck
 *Date:		Mar 2018
-*/


#include "volprocudfreplacer.h"

#include "arrayndalgo.h"
#include "cubesubsel.h"
#include "keystrs.h"
#include "seisdatapack.h"
#include "trckeyzsampling.h"


const char* VolProc::UdfReplacer::sKeyPadTraces()
{ return "Pad missing input traces"; }


VolProc::UdfReplacer::UdfReplacer()
    : Step()
{
}


VolProc::UdfReplacer::~UdfReplacer()
{
}


void VolProc::UdfReplacer::fillPar( IOPar& par ) const
{
    Step::fillPar( par );

    par.set( sKey::Value(), replval_ );
    par.setYN( sKeyPadTraces(), padmissingtraces_ );
    par.set( sKey::Component(), comps_ );
}


bool VolProc::UdfReplacer::usePar( const IOPar& par )
{
    if ( !Step::usePar(par) )
	return false;

    par.get( sKey::Value(), replval_ );
    par.getYN( sKeyPadTraces(), padmissingtraces_ );
    par.get( sKey::Component(), comps_ );

    return true;
}


bool VolProc::UdfReplacer::canInputAndOutputBeSame() const
{
   return comps_.isEmpty();
}


int VolProc::UdfReplacer::getNrOutComponents( OutputSlotID slotid,
					      Pos::GeomID geomid ) const
{
    return validOutputSlotID( slotid )
		? getNrInputComponents( getInputSlotID(0), geomid )
		: Step::getNrOutComponents( slotid, geomid );
}


bool VolProc::UdfReplacer::copyComponentsSel( const InputSlotID inpslotid,
					      OutputSlotID& outslotid ) const
{
    const bool correctslot = inpslotid == getInputSlotID( 0 );
    if ( correctslot )
	outslotid = inpslotid;

    return correctslot;
}


bool VolProc::UdfReplacer::prepareWork( int )
{
    if ( !Step::prepareWork() )
	return false;

    if ( canInputAndOutputBeSame() )
	return true;

    const RegularSeisDataPack* input = getInput( getInputSlotID(0) );
    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );
    for ( int icomp=0; icomp<output->nrComponents(); icomp++ )
	output->setComponentName( input->getComponentName(icomp), icomp );

    return true;
}


ReportingTask* VolProc::UdfReplacer::createTask()
{
    if ( !prepareWork() )
	return 0;

    const RegularSeisDataPack* input = getInput( getInputSlotID(0) );
    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );
    if ( padmissingtraces_ )
    {
	const TrcKeySampling tks( output->horSubSel() );
	PosInfo::CubeData* outcd = new PosInfo::CubeData;
	outcd->generate( tks.start_, tks.stop_, tks.step_ );
	output->setTracePositions( outcd );
    }

    TaskGroup* tasks = new TaskGroup;
    const int nrcomps = output->nrComponents();
    for ( int icomp=0; icomp<nrcomps; icomp++ )
    {
	Array3D<float>& out = output->data( icomp );
	if ( !canInputAndOutputBeSame() )
	{
	    auto* copier =
		new CubeArrayCopier<float>( input->data(icomp), out,
		      *input->subSel().asCubeSubSel(),
		      *output->subSel().asCubeSubSel() );
	    tasks->addTask( copier );
	    if ( !comps_.isPresent(icomp) )
		continue;
	}

	auto* task = new ArrayUdfValReplacer<float>( out, nullptr );
	task->setReplacementValue( replval_ );
	task->setSampling( output->horSubSel(), output->tracePositions() );
	tasks->addTask( task );
    }

    return tasks;
}
