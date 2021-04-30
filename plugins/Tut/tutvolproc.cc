/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Helene Huck
 Date:		March 2016
________________________________________________________________________

-*/


#include "tutvolproc.h"

#include "arrayndimpl.h"
#include "seisdatapack.h"


#define mTypeSquare		0
#define mTypeShift		1

namespace VolProc
{

TutOpCalculator::TutOpCalculator()
    : Step()
{
    setStepParameters();
}


TutOpCalculator::~TutOpCalculator()
{
}


void TutOpCalculator::setStepParameters()
{
    setHStep( shift_ );

    /* Not required for this specific step:

    setVStep( 0 );		//No vertical stepout required by the algorithm
    setInpNrComps( 0, 1 );	// The input datapack needs only one component
    setOutputNrComps( 1 );	// The output datapack needs only one component

    */
}


void TutOpCalculator::fillPar( IOPar& par ) const
{
    Step::fillPar( par );

    par.set( sKeyTypeIndex(), type_ );
    par.set( sKey::StepInl(), shift_.inl() );
    par.set( sKey::StepCrl(), shift_.crl() );
}


bool TutOpCalculator::usePar( const IOPar& par )
{
    if ( !Step::usePar(par) )
	return false;

    if ( !par.get(sKeyTypeIndex(),type_) )
	return false;

    if ( type_ == mTypeShift && ( !par.get(sKey::StepInl(),shift_.inl())
			      ||  !par.get(sKey::StepCrl(),shift_.crl()) ) )
	return false;
    else if ( type_ == mTypeSquare )
	shift_ = BinID( 0, 0 );

    setStepParameters();

    return true;
}


bool TutOpCalculator::prepareWork()
{
    if ( inputs_.isEmpty() )
	return false;

    const RegularSeisDataPack* inputdatapack = inputs_[0];
    if ( !inputdatapack || inputdatapack->nrComponents() !=
			   getNrInputComponents( getInputSlotID(0) ) )
	return false;

    RegularSeisDataPack* outputdatapack = getOutput( getOutputSlotID(0));
    if ( !outputdatapack || outputdatapack->nrComponents() !=
			    getNrOutComponents() )
	return false;
    /*All of the implementation above will be in the base class after 6.2,
      prepareWork will be virtual.
      */

    outputdatapack->setComponentName( inputdatapack->getComponentName(0) );

    return true;
}


Task* TutOpCalculator::createTask()
{
    if ( !prepareWork() )
	return 0;

    const RegularSeisDataPack* input = getInput( getInputSlotID(0) );
    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );

    const TrcKeyZSampling tkzsin = input->sampling();
    const TrcKeyZSampling tkzsout = output->sampling();
    return new TutOpCalculatorTask( input->data(), tkzsin, tkzsout, type_,
				    shift_, output->data() );
}

//--------- TutOpCalculatorTask -------------


TutOpCalculatorTask::TutOpCalculatorTask( const Array3D<float>& input,
					  const TrcKeyZSampling& tkzsin,
					  const TrcKeyZSampling& tkzsout,
					  int optype, BinID shift,
					  Array3D<float>& output )
    : ParallelTask("Tut VolProc::Step Executor")
    , input_( input )
    , output_( output )
    , shift_( shift )
    , tkzsin_( tkzsin )
    , tkzsout_( tkzsout )
    , type_( optype )
{
    totalnr_ = output.info().getSize(0) * output.info().getSize(1);
}


bool TutOpCalculatorTask::doWork( od_int64 start, od_int64 stop, int )
{
    BinID curbid;
    const int incr = mCast( int, stop-start+1 );
    const int nrinlines = input_.info().getSize( 0 );
    const int nrcrosslines = input_.info().getSize( 1 );
    const int nrsamples = input_.info().getSize( 2 );
    TrcKeySamplingIterator iter;
    iter.setSampling( tkzsout_.hsamp_ );
    iter.setNextPos( tkzsout_.hsamp_.trcKeyAt(start) );
    iter.next( curbid );
    for ( int idx=0; idx<incr; idx++ )
    {
	const int inpinlidx = tkzsin_.lineIdx( curbid.inl() );
	const int inpcrlidx = tkzsin_.trcIdx( curbid.crl() );
	const int wantedinlidx = type_ == mTypeShift ? inpinlidx + shift_.inl()
						     : inpinlidx;
	const int wantedcrlidx = type_ == mTypeShift ? inpcrlidx + shift_.crl()
						     : inpcrlidx;
	const int valididxi = wantedinlidx<0 ? 0
					     : wantedinlidx>=nrinlines
						? nrinlines-1
						: wantedinlidx;
	const int valididxc = wantedcrlidx<0 ? 0
					     : wantedcrlidx>=nrcrosslines
						? nrcrosslines-1
						: wantedcrlidx;

	int outpinlidx = tkzsout_.lineIdx( curbid.inl() );
	int outpcrlidx = tkzsout_.trcIdx( curbid.crl() );

	for ( int idz=0; idz<nrsamples; idz++ )
	{
	    const float val = input_.get( valididxi, valididxc, idz );
	    output_.set( outpinlidx, outpcrlidx, idz,
			 type_ == mTypeShift ? val : val*val );
	}
	iter.next( curbid );
    }

    addToNrDone( incr );
    return true;
}


uiString TutOpCalculatorTask::uiMessage() const
{
    return tr("Computing Tutorial Operations");
}

}//namespace
