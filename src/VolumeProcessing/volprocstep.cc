/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : October 2006
-*/


#include "volprocchain.h"
#include "seisdatapack.h"


namespace VolProc
{

mImplFactory( Step, Step::factory );


class BinIDWiseTask : public ParallelTask
{ mODTextTranslationClass(BinIDWiseTask);
public:

		BinIDWiseTask( Step& ro )
		    : step_( ro ), totalnr_( -1 ) { setName(ro.userName()); }

    uiString	message() const	{ return errmsg_; }
    uiString	nrDoneText() const	{ return tr("Positions done"); }

protected:

    bool	doWork(od_int64 start, od_int64 stop, int threadid )
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

    od_int64	nrIterations() const
		{
		    if ( totalnr_==-1 )
		    {
			const TrcKeySampling hrg(
				step_.output_->sampling().hsamp_ );
			totalnr_ = hrg.nrInl() * hrg.nrCrl();
		    }

		    return totalnr_;
		}

    bool	doPrepare( int nrthreads )
		{
		    const bool res = step_.prepareComp( nrthreads );
		    if ( !res ) errmsg_ = step_.errMsg();
		    return res;
		}

    Step&		step_;
    mutable int		totalnr_;
    uiString		errmsg_;
};

} // namespace VolProc


VolProc::Step::Step()
    : chain_( 0 )
    , output_( 0 )
    , id_( cUndefID() )
{
    inputs_.allowNull();
}


VolProc::Step::~Step()
{
    deepUnRef( inputs_ );
}


void VolProc::Step::resetInput()
{
    deepUnRef( inputs_ );

    inputslotids_.erase();
    for ( int idx=0; idx<getNrInputs(); idx++ )
    {
	if ( inputs_.validIdx(idx) )
	    inputs_.replace( idx, 0 );
	else
	    inputs_ += 0;

	inputslotids_ += idx;
    }
}


void VolProc::Step::releaseData()
{
    output_ = 0;

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
    return hr;
}


StepInterval<int> VolProc::Step::getInputZRg(
				const StepInterval<int>& si ) const
{
    return si;
}


void VolProc::Step::setInput( InputSlotID slotid,
			      const RegularSeisDataPack* dc )
{
    if ( inputs_.isEmpty() )
	resetInput();

    const int idx = inputslotids_.indexOf( slotid );
    if ( !inputs_.validIdx(idx) )
	return;

    unRefPtr( inputs_[idx] );
    inputs_.replace( idx, dc );
    refPtr( inputs_[idx] );
}


VolProc::Step::CVolRef VolProc::Step::getInput( InputSlotID slotid ) const
{
    const int idx = inputslotids_.indexOf( slotid );
    return inputs_.validIdx(idx) ? inputs_[idx] : 0;
}


void VolProc::Step::setOutput( OutputSlotID slotid, RegularSeisDataPack* dc,
		      const TrcKeySampling& hrg,
		      const StepInterval<int>& zrg )
{
    output_ = dc;
    DPM( DataPackMgr::SeisID() ).add( output_ );

    tks_ = hrg;
    zrg_ = zrg;
}


VolProc::Step::VolRef VolProc::Step::getOutput( OutputSlotID slotid )
{
    // TODO: implement using slotid
    return output_;
}


VolProc::Step::CVolRef VolProc::Step::getOutput( OutputSlotID slotid ) const
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




od_int64 VolProc::Step::getBaseMemoryUsage(
	const TrcKeySampling& hsamp, const StepInterval<int>& zsamp )
{
    const od_int64 nrtrcs = hsamp.totalNr();
    return nrtrcs * (zsamp.nrSteps()+1) * sizeof(float);
}


od_int64 VolProc::Step::getExtraMemoryUsage(
	const TrcKeySampling& hsamp, const StepInterval<int>& zsamp,
	const TypeSet<OutputSlotID>& ids ) const
{
    TypeSet<OutputSlotID> tocalc( ids );
    if ( tocalc.isEmpty() )
    {
	const int nroutputs = getNrOutputs();
	for ( int idx=0; idx<nroutputs; idx++ )
	    tocalc += getOutputSlotID( idx );
    }

    od_int64 ret = 0;
    for ( int idx=0; idx<tocalc.size(); idx++ )
	ret += extraMemoryUsage( tocalc[idx], hsamp, zsamp );

    return ret;
}
