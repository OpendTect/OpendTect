/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 23-3-1996
 * FUNCTION : SynthSeis
-*/


#include "raysynthgenerator.h"

#include "keystrs.h"
#include "prestackgather.h"
#include "prestacksynthdataset.h"
#include "raytracerrunner.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "trckey.h"

#define mErrRet(msg, retval) { msg_ = msg; return retval; }
#define mpErrRet(msg, retval) { pErrMsg(msg); return retval; }

RaySynthGenerator::RaySynthGenerator( const SynthSeis::GenParams& gp,
				      RayModelSet& rms )
    : ParallelTask("Synthetic generator")
    , SynthSeis::GenBase()
{
    createDataSet( gp, rms );
}


RaySynthGenerator::RaySynthGenerator( SynthSeis::DataSet& dataset )
    : ParallelTask("Synthetic generator")
    , SynthSeis::GenBase()
    , dataset_(&dataset)
{
}


void RaySynthGenerator::createDataSet( const GenParams& gp,
				       RayModelSet& rms )
{
    if ( gp.type_ == SynthSeis::ZeroOffset )
    {
	auto* dp = new SeisTrcBufDataPack(
			SynthSeis::PostStackDataSet::sDataPackCategory() );
	dataset_ = new SynthSeis::PostStackDataSet( gp, *dp, &rms );
    }
    else
    {
	auto* dp = new GatherSetDataPack();
	dataset_ = new SynthSeis::PreStackDataSet( gp, *dp, rms );
    }
}


RaySynthGenerator::~RaySynthGenerator()
{
}


const SynthSeis::RayModelSet& RaySynthGenerator::rayModels() const
{
    return dataset_->rayModels();
}


uiString RaySynthGenerator::nrDoneText() const
{
    return tr("Models done");
}


od_int64 RaySynthGenerator::nrIterations() const
{
    return dataset_ ? rayModels().size() : 0;
}


bool RaySynthGenerator::doPrepare( int nrthreads )
{
    if ( !dataset_ )
	mErrRet(tr("No Synthetic Dataset available"), false);

    if ( !isInputOK() )
	return false;

    IOPar synthpar( dataset_->genParams().raypars_ );
    if ( wavelet_ )
	synthpar.removeWithKey( sKey::WaveletID() );

    usePar( synthpar );
    if ( applynmo_ && rayModels().hasZeroOffsetOnly() )
	applynmo_ = false;

    //TODO: well tie: Z Range from valid dataset ?
    if ( outputsampling_.isUdf() )
    {
	ObjectSet<ReflectivityModel> reflmodels;
	for ( const auto* rm : rayModels() )
	    reflmodels.append( rm->reflModels() );

	if ( !setSamplingFromModels(reflmodels) )
	    return false;
    }
    else if ( worksampling_.isUdf() )
    {
	if ( applynmo_ )
	    return false;

	worksampling_ = outputsampling_;
    }

    trcsset_.setEmpty();
    for ( int imod=0; imod<dataset_->rayModels().size(); imod ++ )
	trcsset_ += new SeisTrcBuf( true );

    IOPar par;
    fillPar( par );
    deepErase( generators_ );
    for ( int ithread=0; ithread<nrthreads; ithread++ )
    {
	SynthSeis::MultiTraceGenerator* multitracegen = new
					SynthSeis::MultiTraceGenerator;
	multitracegen->usePar( par );
	generators_ += multitracegen;
    }

    msg_ = tr("Generating synthetics");

    return true;
}


bool RaySynthGenerator::doWork( od_int64 start, od_int64 stop, int ithread )
{
    if ( !generators_.validIdx(ithread) )
	return false;

    SynthSeis::MultiTraceGenerator& multitracegen = *generators_.get( ithread );
    for ( int idx=mCast(int,start); idx<=stop; idx++, addToNrDone(1) )
    {
	if ( !shouldContinue() )
	    return false;

	auto& raymod = *dataset_->rayMdls().get( idx );
	auto& trcs = *trcsset_.get( idx );
	const TrcKey tk = TrcKey::getSynth( idx*nrstep_ + 1 );
	multitracegen.set( raymod, trcs, &tk );
	if ( !multitracegen.executeParallel(false) )
	    mErrRet( multitracegen.message(), false )
    }

    return true;
}


bool RaySynthGenerator::doFinish( bool success )
{
    deepErase( generators_ );
    if ( !success )
    {
	dataset_ = 0;
	return false;
    }

    dataset_->updateD2TModels();
    return updateDataPack();
}


bool RaySynthGenerator::updateDataPack()
{
    DataPack& dp = dataset_->dataPack();
    if ( !dataset_->isPS() )
    {
	mDynamicCastGet( SeisTrcBufDataPack*, tbdp, &dp );
	auto* tbuf = new SeisTrcBuf( true );
	for ( int imod=0; imod<trcsset_.size(); imod++ )
	    tbuf->stealTracesFrom( *trcsset_.get(imod) );
	trcsset_.setEmpty();

	tbdp->setBuffer( tbuf, Seis::Line, SeisTrcInfo::TrcNr, 0, true );
	return true;
    }

    RefObjectSet<Gather> gatherset;
    for ( int imod=0; imod<trcsset_.size(); imod++ )
    {
	SeisTrcBuf& trcs = *trcsset_.get( imod );
	RefMan<Gather> gather = new Gather();
	if ( !gather->setFromTrcBuf( trcs, 0 ) )
	    { pErrMsg("Cannot set gather from trcbuf?"); }
	else
	    gatherset += gather;

	gather->setCorrected( applynmo_ );
    }
    trcsset_.setEmpty();

    mDynamicCastGet( GatherSetDataPack*, prestkdp, &dp );
    prestkdp->setGathers( gatherset );
    deepUnRef( gatherset );

    return true;
}


bool RaySynthGenerator::isResultOK() const
{
    // not dataset_->isEmpty() but its datapack
    return dataset_ && !dataset_->dataPack().isEmpty();
}
