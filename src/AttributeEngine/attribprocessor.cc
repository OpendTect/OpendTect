/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attribprocessor.h"

#include "attribdesc.h"
#include "attriboutput.h"
#include "attribprovider.h"
#include "binidvalset.h"
#include "seisinfo.h"
#include "seisselectionimpl.h"

#include <limits.h>


namespace Attrib
{

Processor::Processor( Desc& desc , const char* lk, uiString& err )
    : Executor("Attribute Processor")
    , moveonly(this)
    , desc_(&desc)
    , provider_(Provider::create(desc,err))
    , prevbid_(BinID::udf())
{
    if ( !provider_ )
	return;

    is2d_ = desc_->is2D();
    if ( is2d_ )
	provider_->setCurLineName( lk );
}


Processor::~Processor()
{
    deepUnRef( outputs_ );

    delete sd_;
}


bool Processor::isOK() const
{
    return provider_ && provider_->isOK();
}


void Processor::addOutput( Output* output )
{
    if ( !output )
	return;

    output->ref();
    outputs_ += output;
}


void Processor::setLineName( const char* lnm )
{
    if ( provider_ && is2d_ )
	provider_->setCurLineName( lnm );
}


#define mErrorReturnValue() \
    ( isHidingDataAvailabilityError() ? Finished() : ErrorOccurred() )


int Processor::nextStep()
{
    if ( !provider_ || outputs_.isEmpty() )
	return ErrorOccurred();

    if ( !isinited_ )
	init();

    if ( !errmsg_.isEmpty() )
	return mErrorReturnValue();

    if ( !provider_->errMsg().isEmpty() )
    {
	errmsg_ = provider_->errMsg();
	return mErrorReturnValue();
    }

    if ( useshortcuts_ )
	provider_->setUseSC();

    int res;
    res = provider_->moveToNextTrace();
    if ( res < 0 || !nriter_ )
    {
	errmsg_ = provider_->errMsg();
	if ( !errmsg_.isEmpty() )
	    return mErrorReturnValue();
    }
    useshortcuts_ ? useSCProcess( res ) : useFullProcess( res );
    if ( !errmsg_.isEmpty() )
	return mErrorReturnValue();

    provider_->resetMoved();
    provider_->resetZIntervals();
    nriter_++;
    return res;
}


void Processor::useFullProcess( int& res )
{
    if ( res < 0 )
    {
	BinID firstpos;

	if ( sd_ && sd_->type() == Seis::Table )
	{
	    mDynamicCastGet(Seis::TableSelData*,tsd,sd_)
	    firstpos = tsd->binidValueSet().firstBinID();
	}
	else
	{
	    const BinID step = provider_->getStepoutStep();
	    firstpos.inl() = step.inl()/abs(step.inl())>0 ?
			   provider_->getDesiredVolume()->hsamp_.start_.inl() :
			   provider_->getDesiredVolume()->hsamp_.stop_.inl();
	    firstpos.crl() = step.crl()/abs(step.crl())>0
		? provider_->getDesiredVolume()->hsamp_.start_.crl()
		: provider_->getDesiredVolume()->hsamp_.stop_.crl();
	}
	provider_->resetMoved();
	res = provider_->moveToNextTrace( firstpos, true );

	if ( res < 0 )
	{
	    errmsg_ = tr("Error during data read");
	    return;
	}
    }

    provider_->updateCurrentInfo();
    const SeisTrcInfo* curtrcinfo = provider_->getCurrentTrcInfo();
    if ( !curtrcinfo && provider_->needStoredInput() )
    {
	errmsg_ = tr("No trace info available");
	return;
    }

    if ( res == 0 && !nrdone_ )
    {
	provider_->setDataUnavailableFlag( true );
	errmsg_ = tr("No positions processed.\n"
	"Most probably, your input volume(s) are not available in the \n"
	"selected region or the required stepout traces are not available");
	return;
    }
    else if ( res != 0 )
	fullProcess( curtrcinfo );
}


void Processor::fullProcess( const SeisTrcInfo* curtrcinfo )
{
    mDynamicCastGet( TableOutput*, taboutp, outputs_[0] );
    BinID curbid = provider_->getCurrentPosition();
    const Pos::GeomID geomid = provider_->getGeomID();
    const bool issynth = Survey::isSynthetic( geomid );
    if ( is2d_ && curtrcinfo )
    {
	mDynamicCastGet( LocationOutput*, locoutp, outputs_[0] );
	if ( locoutp || taboutp )
	    curbid = curtrcinfo->binID();
	else
	{
	    curbid.row() = Survey::is2DGeom(geomid) ? geomid.asInt() : 0;
	    curbid.trcNr() = curtrcinfo->trcNr();
	}
    }

    SeisTrcInfo mytrcinfo;
    if ( !curtrcinfo )
    {
	if ( is2d_ || issynth )
	    mytrcinfo.setGeomID( geomid ).setTrcNr( curbid.trcNr() );
	else
	    mytrcinfo.setPos( curbid );

	mytrcinfo.calcCoord();
    }

    if ( !curtrcinfo )
	curtrcinfo = &mytrcinfo;

    const TrcKey& tracekey = curtrcinfo->trcKey();
    TypeSet< Interval<int> > localintervals;
    bool isset = setZIntervals( localintervals, curbid,
					 curtrcinfo->coord, tracekey );

    for ( int idi=0; idi<localintervals.size(); idi++ )
    {
	const SamplingData<float>& trcsd = curtrcinfo->sampling;
	const float nrsteps = trcsd.start / trcsd.step;
	const float inrsteps = (float)mNINT32( nrsteps );
	float outz0shifthack = 0.f;
	if ( std::abs(nrsteps-inrsteps) > 0.0001f )
	    outz0shifthack = (nrsteps-inrsteps) * trcsd.step;

	const DataHolder* data = isset ?
			provider_->getData( BinID::noStepout(), idi ) : nullptr;
	if ( data )
	{
	    for ( int idx=0; idx<outputs_.size(); idx++ )
	    {
		Output* outp = outputs_[idx];
		mDynamicCastGet( SeisTrcStorOutput*, trcstoroutp, outp );
		if ( trcstoroutp )
		    trcstoroutp->writez0shift_ = outz0shifthack;

		outputs_[idx]->collectData( *data, provider_->getRefStep(),
						*curtrcinfo );
	    }
	}

	if ( isset )
	    nrdone_++;
    }

    prevbid_ = curbid;
}


void Processor::useSCProcess( int& res )
{
    if ( res < 0 )
    {
	errmsg_ = tr("Error during data read");
	return;
    }
    if ( res == 0 && !nrdone_ )
    {
	provider_->setDataUnavailableFlag( true );
	errmsg_ = tr("This stored cube contains no data in selected area.\n");
	return;
    }

    if ( res == 0 ) return;

    for ( int idx=0; idx<outputs_.size(); idx++ )
	provider_->fillDataPackWithTrc(
			outputs_[idx]->getDataPack(provider_->getRefStep()) );

    nrdone_++;
}


void Processor::init()
{
    TypeSet<int> globaloutputinterest;
    TrcKeyZSampling globalcs;
    defineGlobalOutputSpecs( globaloutputinterest, globalcs );
    if ( is2d_ )
    {
	provider_->adjust2DLineStoredVolume();
	provider_->compDistBetwTrcsStats();
	float maxdist = provider_->getDistBetwTrcs(true);
	mDynamicCastGet( Trc2DVarZStorOutput*, trcvarzoutp, outputs_[0] );
	if ( trcvarzoutp )
	    if ( trcvarzoutp ) trcvarzoutp->setMaxDistBetwTrcs( maxdist );
	mDynamicCastGet( TableOutput*, taboutp, outputs_[0] );
	if ( taboutp )
	{
	    taboutp->setMaxDistBetwTrcs( maxdist );
	    float mediandist = provider_->getDistBetwTrcs(false);
	    taboutp->setMedianDistBetwTrcs( mediandist );
	}

    }
    computeAndSetRefZStepAndZ0();
    provider_->prepPriorToBoundsCalc();

    prepareForTableOutput();

    for ( int idx=0; idx<globaloutputinterest.size(); idx++ )
	provider_->enableOutput(globaloutputinterest[idx], true );

    //Special case for attributes (like PreStack) which inputs are not treated
    //as normal input cubes and thus not delivering adequate cs automaticly
    provider_->updateCSIfNeeded(globalcs);

    computeAndSetPosAndDesVol( globalcs );
    for ( int idx=0; idx<outputs_.size(); idx++ )
	outputs_[idx]->adjustInlCrlStep( *provider_->getPossibleVolume() );

    mDynamicCastGet(const DataPackOutput*,dpoutput,outputs_[0]);
    if ( dpoutput && provider_->getDesc().isStored() )
	useshortcuts_ = true;
    else
	provider_->prepareForComputeData();

    isinited_ = true;
}


void Processor::defineGlobalOutputSpecs( TypeSet<int>& globaloutputinterest,
					 TrcKeyZSampling& globalcs )
{
    bool ovruleoutp = provider_->prepPriorToOutputSetup();
    if ( ovruleoutp )
    {
	for ( int idx=0; idx<provider_->nrOutputs(); idx++ )
	    if ( provider_->isOutputEnabled(idx) )
		outpinterest_.addIfNew( idx );
    }

    for ( int idx=0; idx<outputs_.size(); idx++ )
    {
	TrcKeyZSampling cs;
	cs.zsamp_.start = 0;	//cover up for synthetics
	if ( !outputs_[idx]->getDesiredVolume(cs) )
	{
	    outputs_[idx]->unRef();
	    outputs_.removeSingle(idx);
	    idx--;
	    continue;
	}

	if ( !idx )
	    globalcs = cs;
	else
	{
	    globalcs.hsamp_.include(cs.hsamp_.start_);
	    globalcs.hsamp_.include(cs.hsamp_.stop_);
	    globalcs.zsamp_.include(cs.zsamp_);
	}

	for ( int idy=0; idy<outpinterest_.size(); idy++ )
	{
	    if ( !globaloutputinterest.isPresent(outpinterest_[idy]) )
		globaloutputinterest += outpinterest_[idy];
	}
	outputs_[idx]->setDesiredOutputs( outpinterest_ );

	mDynamicCastGet( SeisTrcStorOutput*, storoutp, outputs_[0] );
	if ( storoutp )
	{
	    TypeSet<Seis::DataType> outptypes;
	    for ( int ido=0; ido<outpinterest_.size(); ido++ )
		outptypes += provider_->getDesc().dataType(outpinterest_[ido]);

	    for ( int idoutp=0; idoutp<outputs_.size(); idoutp++ )
		((SeisTrcStorOutput*)outputs_[idoutp])->setOutpTypes(outptypes);
	}
    }
}


void Processor::computeAndSetRefZStepAndZ0()
{
    provider_->computeRefStep();
    const float zstep = provider_->getRefStep();
    provider_->setRefStep( zstep );

    provider_->computeRefZ0();
    const float z0 = provider_->getRefZ0();
    provider_->setRefZ0( z0 );
}


void Processor::prepareForTableOutput()
{
    if ( outputs_.size() && outputs_[0]->getSelData().type() == Seis::Table )
    {
	for ( int idx=0; idx<outputs_.size(); idx++ )
	{
	    if ( !idx )
		sd_ = outputs_[0]->getSelData().clone();
	    else
		sd_->include( outputs_[idx]->getSelData() );
	}
    }

    if ( sd_ && sd_->type() == Seis::Table )
    {
	provider_->setSelData( sd_ );
	mDynamicCastGet( LocationOutput*, locoutp, outputs_[0] );
	mDynamicCastGet( TableOutput*, taboutp, outputs_[0] );
	if ( locoutp || taboutp )
	{
	    Interval<float> extraz( -2*provider_->getRefStep(),
				    2*provider_->getRefStep() );
	    provider_->setExtraZ( extraz );
	    provider_->setNeedInterpol(true);
	}
    }

    if ( outputs_.size() >1 )
    {
	for ( int idx=0; idx<outputs_.size(); idx++ )
	{
	    mDynamicCastGet( LocationOutput*, locoutp, outputs_[0] );
	    if ( locoutp )
		locoutp->setPossibleBinIDDuplic();
	    else
	    {
		mDynamicCastGet( TableOutput*, taboutp, outputs_[0] );
		if ( taboutp )
		    taboutp->setPossibleBinIDDuplic();
	    }
	}
    }
}


void Processor::computeAndSetPosAndDesVol( TrcKeyZSampling& globalcs )
{
    if ( provider_->getInputs().isEmpty() && !provider_->getDesc().isStored() )
    {
	provider_->setDesiredVolume( globalcs );
	provider_->setPossibleVolume( globalcs );
    }
    else
    {
	TrcKeyZSampling possvol;
	possvol.zsamp_.step = globalcs.zsamp_.step;
	if ( !possvol.includes(globalcs) )
	    possvol = globalcs;

	if ( is2d_ && possvol == globalcs )
	{
	    const Pos::GeomID geomid = provider_->getGeomID();
	    const int lineid =
		geomid==Survey::GM().cUndefGeomID() ? 0 : geomid.asInt();
	    possvol.hsamp_.setLineRange( StepInterval<int>(lineid,lineid,1) );
	    possvol.hsamp_.setTrcRange( Interval<int>(0,INT_MAX/3*2) );
					    //unlikely;still, a limitation.
	    const Survey::Geometry* geom = Survey::GM().getGeometry( geomid );
	    if ( geom )
		possvol.hsamp_.setTrcRange( geom->sampling().hsamp_.trcRange());
	    globalcs = possvol;
	}

	provider_->setDesiredVolume( possvol );
	if ( !provider_->getPossibleVolume( -1, possvol ) )
	{
	    errmsg_ = provider_->errMsg();
	    if ( errmsg_.isEmpty() )
	    {
		provider_->setDataUnavailableFlag( true );
		errmsg_ = tr("Not possible to output required attribute"
                         " in this area.\nPlease confront stepouts/timegates"
		         " with available data");
	    }
	    return;
	}

	provider_->resetDesiredVolume();
	globalcs.adjustTo( possvol );
	provider_->setDesiredVolume( globalcs );
    }
}


bool Processor::setZIntervals(
			TypeSet< Interval<int> >& localintervals,
			const BinID& curbid, const Coord& curcoords,
			const TrcKey& tkey )
{
    //TODO: Smarter way if output's intervals don't intersect
    bool isset = false;
    TypeSet<float> exactz;
    mDynamicCastGet( Trc2DVarZStorOutput*, trc2dvarzoutp, outputs_[0] );

    for ( int idx=0; idx<outputs_.size(); idx++ )
    {
	mDynamicCastGet( TableOutput*, taboutp, outputs_[idx] );
	mDynamicCastGet( LocationOutput*, locoutp, outputs_[idx] );
	bool wantsout = !tkey.isUdf() && taboutp && is2d_
			   ? taboutp->wantsOutput(tkey)
			   : outputs_[idx]->useCoords() || locoutp
				? outputs_[idx]->wantsOutput(curcoords)
				: outputs_[idx]->wantsOutput(curbid);

	if ( trc2dvarzoutp && is2d_ )		//tmp patch -> ??
	    wantsout = true;

	if ( !wantsout || (curbid == prevbid_ && !is2d_) ) //!is2d = tmp patch
	    continue;

	const float refzstep = provider_->getRefStep();
	TypeSet< Interval<int> > localzrange = !tkey.isUdf() && taboutp && is2d_
	    ? taboutp->getLocalZRanges( tkey, refzstep, exactz )
	    : outputs_[idx]->useCoords() || locoutp
		? outputs_[idx]->getLocalZRanges( curcoords, refzstep, exactz )
		: outputs_[idx]->getLocalZRanges( curbid, refzstep, exactz );

	if ( isset )
	    localintervals.append ( localzrange );
	else
	{
	    localintervals = localzrange;
	    isset = true;
	}
    }

    if ( isset )
    {
	provider_->addLocalCompZIntervals( localintervals );
	if ( !exactz.isEmpty() )
	    provider_->setExactZ( exactz );
    }

    return isset;
}


od_int64 Processor::totalNr() const
{
    return provider_ ? provider_->getTotalNrPos(is2d_) : 0;
}


od_int64 Processor::nrDone() const
{
    return nrdone_;
}


uiString Processor::uiMessage() const
{
    return errmsg_.isEmpty() ? tr("Processing") : errmsg_;
}


void Processor::addOutputInterest( int sel )
{
    outpinterest_.addIfNew( sel < 0 ? 0 : sel );
}


const char* Processor::getAttribName() const
{
    return desc_->attribName();
}


const char* Processor::getAttribUserRef() const
{
    return desc_->userRef();
}


void Processor::setRdmPaths( const TypeSet<BinID>& truepath,
			     const TypeSet<BinID>& snappedpath )
{
    if ( !provider_ ) return;

    provider_->setRdmPaths( truepath, snappedpath );
}


void Processor::showDataAvailabilityErrors( bool yn )
{ showdataavailabilityerrors_ = yn; }


bool Processor::isHidingDataAvailabilityError() const
{
    return !showdataavailabilityerrors_ && provider_->getDataUnavailableFlag();
}


} // namespace Attrib
