/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2003
-*/



#include "attribprocessor.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attriboutput.h"
#include "attribprovider.h"
#include "binnedvalueset.h"
#include "cubesubsel.h"
#include "trckey.h"
#include "seistableseldata.h"
#include "seisinfo.h"
#include "survgeom2d.h"
#include "survinfo.h"

#include <limits.h>


namespace Attrib
{

Processor::Processor( Desc& desc, uiRetVal& uirv, GeomID gid )
    : Executor("Attribute Processor")
    , desc_(desc)
    , provider_(Provider::create(desc,uirv))
    , nriter_(0)
    , nrdone_(0)
    , isinited_(false)
    , useshortcuts_(false)
    , moveonly(this)
    , prevbid_(BinID(-1,-1))
    , sd_(0)
    , showdataavailabilityerrors_(true)
{
    if ( provider_ )
    {
	provider_->ref();
	desc_.ref();

	if ( is2D() )
	    provider_->setGeomID( gid );
    }
}


Processor::~Processor()
{
    if ( provider_ )
	{ provider_->unRef(); desc_.unRef(); }
    deepUnRef( outputs_ );
    delete sd_;
}


bool Processor::is2D() const
{
    return desc_.is2D();
}


bool Processor::isOK() const
{
    return provider_;
}


void Processor::setDesiredSubSel( const FullSubSel& fss )
{
    if ( provider_ )
	provider_->setDesiredSubSel( fss );
}


void Processor::addOutput( Output* output )
{
    if ( !output )
	return;
    output->ref();
    outputs_ += output;
}


void Processor::setGeomID( GeomID gid )
{
    if ( provider_ )
	provider_->setGeomID( gid );
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

    if ( !provider_->errMsg().isOK() )
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

	if ( sd_ && sd_->isTable() )
	    firstpos = sd_->asTable()->binidValueSet().firstBinID();
	else
	{
	    const auto& desss = provider_->desiredSubSel();
	    if ( is2D() )
		firstpos.inl() = desss.geomID(0).getI();
	    else
		firstpos.inl() = desss.cubeSubSel().inlSubSel().posStart();
	    firstpos.crl() = desss.trcNrRange().start;
	}
	provider_->resetMoved();
	res = provider_->moveToNextTrace( firstpos, true );

	if ( res < 0 )
	    { errmsg_ = tr("Error during data read"); return; }
    }

    provider_->updateCurrentInfo();
    const SeisTrcInfo* curtrcinfo = provider_->getCurrentTrcInfo();
    if ( !curtrcinfo && provider_->needStoredInput() )
	{ errmsg_ = tr("No trace info available"); return; }

    if ( res == 0 && !nrdone_ )
    {
	provider_->setDataUnavailableFlag( true );
	errmsg_ = tr("No positions processed.\n"
	"Most probably, your input data is not available in the \n"
	"selected region or the required stepout traces are not available");
	return;
    }
    else if ( res != 0 )
	fullProcess( curtrcinfo );
}


void Processor::fullProcess( const SeisTrcInfo* curtrcinfo )
{
    BinID curbid = provider_->getCurrentPosition();
    SeisTrcInfo mytrcinfo;
    if ( !curtrcinfo )
    {
	if ( is2D() )
	{
	    const Bin2D b2d( provider_->geomID(), curbid.crl() );
	    mytrcinfo.setPos( b2d );
	    mytrcinfo.coord_ = b2d.coord();
	}
	else
	{
	    mytrcinfo.setPos( curbid );
	    mytrcinfo.coord_ = curbid.coord();
	}
	curtrcinfo = &mytrcinfo;
    }

    TypeSet< Interval<int> > localintervals;
    bool isset = setZIntervals( localintervals, curtrcinfo->trcKey(),
				curtrcinfo->coord_ );

    for ( int idi=0; idi<localintervals.size(); idi++ )
    {
	const SamplingData<float>& trcsd = curtrcinfo->sampling_;
	const float nrsteps = trcsd.start / trcsd.step;
	const float inrsteps = (float)mNINT32( nrsteps );
	float outz0shifthack = 0.f;
	if ( std::abs(nrsteps-inrsteps) > Seis::cDefSampleSnapDist() )
	    outz0shifthack = (nrsteps-inrsteps) * trcsd.step;

	const DataHolder* data = isset ?
				provider_->getData( BinID(0,0), idi ) : 0;
	if ( data )
	{
	    for ( int idx=0; idx<outputs_.size(); idx++ )
	    {
		Output* outp = outputs_[idx];
		mDynamicCastGet( SeisTrcStorOutput*, trcstoroutp, outp );
		if ( trcstoroutp )
		    trcstoroutp->writez0shift_ = outz0shifthack;
		outputs_[idx]->collectData( *data, provider_->refZStep(),
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
	{ errmsg_ = uiStrings::phrErrDuringRead(); return; }
    if ( res == 0 && !nrdone_ )
    {
	provider_->setDataUnavailableFlag( true );
	errmsg_ = tr("The input contains no data in the selected area");
	return;
    }

    if ( res == 0 ) return;

    for ( int idx=0; idx<outputs_.size(); idx++ )
	provider_->fillDataPackWithTrc(
			outputs_[idx]->getDataPack(provider_->refZStep()) );

    nrdone_++;
}


void Processor::init()
{
    TypeSet<int> globaloutputinterest;
    FullSubSel globalfss;
    globalfss.setToAll( is2D() );
    defineGlobalOutputSpecs( globaloutputinterest, globalfss );
    if ( is2D() )
    {
	provider_->adjust2DLineStoredSubSel();
	provider_->compDistBetwTrcsStats();
	mDynamicCastGet( Trc2DVarZStorOutput*, trcvarzoutp, outputs_[0] );
	mDynamicCastGet( TableOutput*, taboutp, outputs_[0] );
	if ( trcvarzoutp || taboutp )
	{
	    float maxdist = provider_->getDistBetwTrcs(true);
	    if ( trcvarzoutp ) trcvarzoutp->setMaxDistBetwTrcs( maxdist );
	    if ( taboutp ) taboutp->setMaxDistBetwTrcs( maxdist );
	}

    }
    computeAndSetRefZStepAndZ0();
    provider_->prepPriorToBoundsCalc();

    prepareForTableOutput();

    for ( int idx=0; idx<globaloutputinterest.size(); idx++ )
	provider_->enableOutput(globaloutputinterest[idx], true );

    //Special case for attributes (like PreStack) which inputs are not treated
    //as normal input cubes and thus not delivering adequate cs automaticly
    provider_->updateSSIfNeeded( globalfss );

    computeAndSetPosAndDesSubSel( globalfss );
    for ( int idx=0; idx<outputs_.size(); idx++ )
	outputs_[idx]->setPossibleSubSel( provider_->possibleSubSel() );

    mDynamicCastGet(const DataPackOutput*,dpoutput,outputs_[0]);
    if ( dpoutput && provider_->getDesc().isStored() )
	useshortcuts_ = true;
    else
	provider_->prepareForComputeData();

    isinited_ = true;
}


void Processor::defineGlobalOutputSpecs( TypeSet<int>& globaloutputinterest,
					 FullSubSel& globalfss )
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
	FullSubSel fss; fss.setToAll( is2D() );
	if ( !outputs_[idx]->getDesiredSubSel(fss) )
	{
	    outputs_[idx]->unRef();
	    outputs_.removeSingle(idx);
	    idx--;
	    continue;
	}

	if ( !idx )
	    globalfss = fss;
	else
	    globalfss.merge( fss );

	for ( int idy=0; idy<outpinterest_.size(); idy++ )
	{
	    if ( !globaloutputinterest.isPresent(outpinterest_[idy]) )
		globaloutputinterest += outpinterest_[idy];
	}
	outputs_[idx]->setDesiredOutputs( outpinterest_ );

	mDynamicCastGet( SeisTrcStorOutput*, storoutp, outputs_[0] );
	if ( storoutp )
	{
	    TypeSet<DataType> outptypes;
	    for ( int ido=0; ido<outpinterest_.size(); ido++ )
		outptypes += provider_->getDesc().dataType(outpinterest_[ido]);

	    for ( int idoutp=0; idoutp<outputs_.size(); idoutp++ )
		((SeisTrcStorOutput*)outputs_[idoutp])->setOutpTypes(outptypes);
	}
    }
}


void Processor::computeAndSetRefZStepAndZ0()
{
    provider_->computeRefZStep();
    const float zstep = provider_->refZStep();
    provider_->setRefZStep( zstep );

    provider_->computeRefZ0();
    const float z0 = provider_->refZ0();
    provider_->setRefZ0( z0 );
}


void Processor::prepareForTableOutput()
{
    auto* output0 = outputs_.first();
    if ( output0 && output0->getSelData().isTable() )
    {
	delete sd_;
	sd_ = output0->getSelData().clone();
	for ( int idx=1; idx<outputs_.size(); idx++ )
	    sd_->include( outputs_[idx]->getSelData() );
    }

    if ( sd_ && sd_->isTable() )
    {
	provider_->setSelData( sd_ );
	mDynamicCastGet( LocationOutput*, locoutp, outputs_[0] );
	mDynamicCastGet( TableOutput*, taboutp, outputs_[0] );
	if ( locoutp || taboutp )
	{
	    Interval<float> extraz( -2*provider_->refZStep(),
				    2*provider_->refZStep() );
	    provider_->setExtraZ( extraz );
	    provider_->setNeedInterpol( true );
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


void Processor::computeAndSetPosAndDesSubSel( FullSubSel& globalfss )
{
    if ( provider_->getInputs().isEmpty() && !provider_->getDesc().isStored() )
	provider_->setPossibleSubSel( globalfss );
    else
    {
	if ( !provider_->calcPossibleSubSel( -1, globalfss ) )
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

	provider_->resetDesiredSubSel();
	globalfss.limitTo( provider_->possibleSubSel() );
    }
    provider_->setDesiredSubSel( globalfss );
}


bool Processor::setZIntervals( TypeSet< Interval<int> >& localintervals,
			       const TrcKey& curtrckey, const Coord& curcoords )
{
    //TODO: Smarter way if output's intervals don't intersect
    bool isset = false;
    const BinID& curbid = curtrckey.binID();
    TypeSet<float> exactz;
    mDynamicCastGet( Trc2DVarZStorOutput*, trc2dvarzoutp, outputs_[0] );
    for ( int idx=0; idx<outputs_.size(); idx++ )
    {
	const bool usecoords =
			outputs_[idx]->useCoords( curtrckey.geomSystem() );
	bool wantout = usecoords ? outputs_[idx]->wantsOutput(curcoords)
				 : outputs_[idx]->wantsOutput(curbid);

	if ( trc2dvarzoutp && is2D() )		//tmp patch
	    wantout = true;

	if ( !wantout || (curbid == prevbid_ && !is2D()) )
							//!is2d = tmp patch
	    continue;

	const float refzstep = provider_->refZStep();
	TypeSet< Interval<int> > localzrange = usecoords
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
    return provider_ ? provider_->totalNrPos() : 0;
}


od_int64 Processor::nrDone() const
{ return nrdone_; }


uiString Processor::message() const
{ return errmsg_.isEmpty() ? uiStrings::sProcessing() : errmsg_; }


void Processor::addOutputInterest( int sel )
{ outpinterest_.addIfNew( sel ); }


const char* Processor::getAttribName() const
{
    return desc_.attribName();
}


const char* Processor::getAttribUserRef() const
{
    return desc_.userRef();
}


void Processor::setRdmPaths( const TypeSet<BinID>& truepath,
			     const TypeSet<BinID>& snappedpath )
{
    if ( provider_ )
	provider_->setRdmPaths( truepath, snappedpath );
}


void Processor::showDataAvailabilityErrors( bool yn )
{ showdataavailabilityerrors_ = yn; }


bool Processor::isHidingDataAvailabilityError() const
{ return !showdataavailabilityerrors_ && provider_->getDataUnavailableFlag(); }


}; // namespace Attrib
