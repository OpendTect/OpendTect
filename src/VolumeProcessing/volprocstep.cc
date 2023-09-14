/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "volprocchain.h"

#include "keystrs.h"

namespace VolProc
{

mImplFactory( Step, Step::factory );


class BinIDWiseTask : public ParallelTask
{ mODTextTranslationClass(BinIDWiseTask);
public:

		BinIDWiseTask( Step& ro )
		    : step_( ro ), totalnr_( -1 )
		{
		    setName(ro.userName());
		    msg_ = tr("Executing Volume Builder Task");
		}

    uiString	uiMessage() const override	{ return msg_; }
    uiString	uiNrDoneText() const override
		{ return ParallelTask::sTrcFinished(); }

protected:

    bool	doWork(od_int64 start, od_int64 stop, int threadid ) override
		{
		    const TrcKeySampling hrg(
				step_.output_->sampling().hsamp_ );
		    BinID curbid = hrg.start_;

		    const int nrinls = mCast( int, start/hrg.nrCrl() );
		    const int nrcrls = mCast( int, start - nrinls*hrg.nrCrl() );
		    curbid.inl() += nrinls*hrg.step_.inl();
		    curbid.crl() += nrcrls*hrg.step_.crl();

		    for ( int idx=mCast(int,start); idx<=stop; idx++ )
		    {
			if ( !step_.computeBinID( curbid, threadid ) )
			    return false;

			addToNrDone( 1 );

			if ( idx>=stop )
			    break;

			if ( !shouldContinue() )
			    return false;

			curbid.crl() += hrg.step_.crl();
			if ( curbid.crl()>hrg.stop_.crl() )
			{
			    curbid.crl() = hrg.start_.crl();
			    curbid.inl() += hrg.step_.inl();
			    if ( curbid.inl()>hrg.stop_.inl() )
			    {
				pErrMsg("Going outside range");
				return false;
			    }
			}
		    }

		    return true;
		}

    od_int64	nrIterations() const override
		{
		    if ( totalnr_==-1 )
		    {
			const TrcKeySampling hrg(
				step_.output_->sampling().hsamp_ );
			totalnr_ = hrg.nrInl() * hrg.nrCrl();
		    }

		    return totalnr_;
		}

    bool	doPrepare( int nrthreads ) override
		{
		    const bool res = step_.prepareComp( nrthreads );
		    if ( !res ) msg_ = step_.errMsg();
		    return res;
		}

    Step&		step_;
    mutable int		totalnr_;
    uiString		msg_;
};

} // namespace VolProc



mStartAllowDeprecatedSection

VolProc::Step::Step()
    : chain_( 0 )
    , output_( 0 )
    , id_( cUndefID() )
    , tks_(false)
    , zrg_(StepInterval<int>::udf())
    , hstep_(0,0)
{
    nrinpcomps_ += 0;
    inputs_.allowNull();
}


VolProc::Step::~Step()
{
    deepUnRef( inputs_ );
}

mStopAllowDeprecatedSection

void VolProc::Step::setInpNrComps( InputSlotID slotid, int nr )
{
    for ( int idx=0; idx<=slotid; idx++ )
    {
	if (nrinpcomps_.size() <= idx )
        nrinpcomps_ += 0;
    }

    nrinpcomps_[slotid] = nr;
}


int VolProc::Step::getNrInputComponents( InputSlotID slotid ) const
{
    return nrinpcomps_[slotid];
}


void VolProc::Step::resetInput()
{
    deepUnRef( inputs_ );

    inputslotids_.setEmpty();
    for ( int idx=0; idx<getNrInputs(); idx++ )
    {
	if ( inputs_.validIdx(idx) )
	    inputs_.replace( idx, 0 );
	else
	    inputs_ += nullptr;

	inputslotids_ += idx;
    }
}


void VolProc::Step::releaseData()
{
    output_ = nullptr;

    resetInput();
}


VolProc::Chain& VolProc::Step::getChain()
{
    return *chain_;
}

const VolProc::Chain& VolProc::Step::getChain() const
{
    return const_cast<Step*>(this)->getChain();
}


void VolProc::Step::setChain( VolProc::Chain& c )
{
    if ( chain_ )
    {
	pErrMsg("Can only add to chain once");
	return;
    }

    chain_ = &c;
    if ( mIsUdf(id_) )
	id_ = c.getNewStepID();
}


const char* VolProc::Step::userName() const
{
    return username_.isEmpty() ? 0 : username_.buf();
}


void VolProc::Step::setUserName( const char* nm )
{
    username_ = nm;
}


bool VolProc::Step::is2D() const
{
    return chain_ && chain_->is2D();
}


int VolProc::Step::getNrInputs() const
{
    return isInputPrevStep() ? 1 : 0;
}


int VolProc::Step::getInputSlotID( int idx ) const
{
    if ( !isInputPrevStep() && !needsInput() )
	return cUndefSlotID();

    if ( idx<0 || idx>=getNrInputs() )
    {
	pErrMsg("Invalid input slot");
	return cUndefSlotID();
    }

    return inputslotids_.validIdx(idx) ? inputslotids_[idx] : idx;
}


void VolProc::Step::getInputSlotName( InputSlotID slotid,
					BufferString& res ) const
{
    res.set( "Input " ).add( slotid );
}


int VolProc::Step::getOutputSlotID( int idx ) const
{
    if ( idx<0 || idx>=getNrOutputs() )
	{ pErrMsg("Invalid output slot"); return cUndefSlotID(); }

    return idx;
}


bool VolProc::Step::validInputSlotID( InputSlotID slotid ) const
{
    for ( int idx=0; idx<getNrInputs(); idx++ )
    {
	if ( getInputSlotID(idx)==slotid )
	    return true;
    }

    return false;
}


bool VolProc::Step::validOutputSlotID( OutputSlotID slotid ) const
{
    for ( int idx=0; idx<getNrOutputs(); idx++ )
    {
	if ( getOutputSlotID(idx)==slotid )
	    return true;
    }

    return false;
}


TrcKeySampling VolProc::Step::getInputHRg( const TrcKeySampling& hr ) const
{
    TrcKeySampling res( hr );
    res.expand( hstep_.inl(), hstep_.crl() );

    return res;
}


StepInterval<int> VolProc::Step::getInputZRg(
					const StepInterval<int>& zrg ) const
{ return zrg; }


StepInterval<int> VolProc::Step::getInputZRgWithGeom(
						const StepInterval<int>& zrg,
						Pos::GeomID ) const
{ return zrg; }


StepInterval<float> VolProc::Step::getInputZSamp(
					const StepInterval<float>& zsamp ) const
{
    StepInterval<float> res( zsamp );
    res.widen( res.step * vstep_ );

    return res;
}


TrcKeyZSampling VolProc::Step::getInputSampling(
					const TrcKeyZSampling& tkzs ) const
{
    TrcKeyZSampling res;
    res.hsamp_ = Step::getInputHRg( tkzs.hsamp_ );
    res.zsamp_ = Step::getInputZSamp( tkzs.zsamp_ );

    const Survey::Geometry* geom = Survey::GM().getGeometry(
						    res.hsamp_.getGeomID() );
    if ( geom )
    {
	res.hsamp_.limitTo( geom->sampling().hsamp_ );
	if ( res.zsamp_.isCompatible(geom->sampling().zsamp_) )
	    res.zsamp_.limitTo( geom->sampling().zsamp_ );
	else
	{
	    const float geomzstart = geom->sampling().zsamp_.start;
	    const float geomzstop = geom->sampling().zsamp_.stop;
	    if ( res.zsamp_.includes(geomzstart,true) )
		res.zsamp_.start =
		    res.zsamp_.snap( geomzstart, OD::SnapDownward );
	    if ( res.zsamp_.includes(geomzstop,true) )
		res.zsamp_.stop = res.zsamp_.snap( geomzstop, OD::SnapUpward );
	}
    }

    return res;
}


od_uint64 VolProc::Step::extraMemoryUsage( OutputSlotID slotid,
					   const TrcKeyZSampling& tkzs ) const
{
    const_cast<Step&>(*this).zsamp_ = tkzs.zsamp_;
    StepInterval<int> unusedzrg;

    return extraMemoryUsage( slotid, tkzs.hsamp_, unusedzrg );
}


od_uint64 VolProc::Step::getComponentMemory( const TrcKeySampling& tks,
					     bool input ) const
{
    TrcKeyZSampling usedtkzs;
    usedtkzs.hsamp_ = tks;
    usedtkzs.zsamp_ = zsamp_;

    return getComponentMemory( usedtkzs, input );
}


od_uint64 VolProc::Step::getComponentMemory( const TrcKeyZSampling& tkzs,
					     bool input ) const
{
    const TrcKeyZSampling usedtkzs = input ? getInputSampling( tkzs ) : tkzs;

    return getBaseMemoryUsage( usedtkzs );
}


void VolProc::Step::setInput( InputSlotID slotid,
			      const RegularSeisDataPack* dc )
{
    if ( inputs_.isEmpty() )
	resetInput();

    const int idx = inputslotids_.indexOf( slotid );
    if ( !inputs_.validIdx(idx) )
	return;

    if ( inputs_[idx] )
	DPM( DataPackMgr::SeisID() ).unRef( inputs_[idx]->id() );

    inputs_.replace( idx, dc );
    if ( inputs_[idx] )
	DPM( DataPackMgr::SeisID() ).ref( inputs_[idx]->id() );

}


const RegularSeisDataPack* VolProc::Step::getInput( InputSlotID slotid ) const
{
    const int idx = inputslotids_.indexOf( slotid );
    return inputs_.validIdx(idx) ? inputs_[idx] : 0;
}


void VolProc::Step::setOutput( OutputSlotID slotid, RegularSeisDataPack* dc,
			       const TrcKeySampling&, const StepInterval<int>& )
{
    output_ = dc;
}


RegularSeisDataPack* VolProc::Step::getOutput( OutputSlotID slotid )
{
    // TODO: implement using slotid
    return output_;
}


const RegularSeisDataPack* VolProc::Step::getOutput( OutputSlotID slotid ) const
{
    return const_cast<Step*>(this)->getOutput( slotid );
}


void VolProc::Step::enableOutput( OutputSlotID slotid )
{
    outputslotids_.addIfNew( slotid );
}


int VolProc::Step::getOutputIdx( OutputSlotID slotid ) const
{
    return outputslotids_.indexOf( slotid );
}


void VolProc::Step::fillPar( IOPar& par ) const
{
    if ( !username_.isEmpty() )
	par.set( sKey::Name(), username_.buf() );

    par.set( sKey::ID(), id_ );
}


bool VolProc::Step::usePar( const IOPar& par )
{
    username_.empty();
    par.get( sKey::Name(), username_ );
    if ( !par.get(sKey::ID(),id_) && chain_ )
	id_ = chain_->getNewStepID();

    return true;
}


Task* VolProc::Step::createTask()
{
    if ( areSamplesIndependent() && prefersBinIDWise() )
	return new BinIDWiseTask( *this );

    return 0;
}


Task* VolProc::Step::createTaskWithProgMeter( ProgressMeter* )
{
    return createTask();
}


/* mDeprecated */ od_int64 VolProc::Step::getOuputMemSize( int outputidx ) const
{
    // Ouch. We thought we'd ask the datapack here, but it's not there yet
    // Therefore, we added getExtraMemoryUsage() for after 6.0
    return 0;
}


od_int64 VolProc::Step::getBaseMemoryUsage(
	const TrcKeySampling& hsamp, const StepInterval<int>& zrg )
{
    const od_int64 nrtrcs = hsamp.totalNr();
    return nrtrcs * (zrg.nrSteps()+1) * sizeof(float);
}


od_uint64 VolProc::Step::getBaseMemoryUsage( const TrcKeyZSampling& tkzs )
{
    return tkzs.totalNr() * sizeof(float);
}
