/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Helene Huck
 Date:		March 2016
________________________________________________________________________

-*/

#include "tutvolproc.h"

#include "arrayndimpl.h"
#include "keystrs.h"
#include "seisdatapack.h"


#define mTypeSquare		0
#define mTypeShift		1

namespace VolProc
{

TutOpCalculator::TutOpCalculator()
    : Step()
{
}


TutOpCalculator::~TutOpCalculator()
{
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
			       || !par.get(sKey::StepCrl(),shift_.crl()) ) )
	return false;
    else if ( type_ == mTypeSquare )
	shift_ = BinID( 0, 0 );

    return true;
}


bool TutOpCalculator::prepareWork( int )
{
    if ( !Step::prepareWork() )
	return false;

    const RegularSeisDataPack* inputdatapack = inputs_[0];
    RegularSeisDataPack* outputdatapack = getOutput( getOutputSlotID(0));

    outputdatapack->setComponentName( inputdatapack->getComponentName(0) );

    return true;
}


ReportingTask* TutOpCalculator::createTask()
{
    if ( !prepareWork() )
	return 0;

    const RegularSeisDataPack* input = getInput( getInputSlotID(0) );
    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );

    const TrcKeyZSampling tkzsin( input->subSel() );
    const TrcKeyZSampling tkzsout( output->subSel() );
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
    totalnr_ = output.getSize(0) * output.getSize(1);
}


bool TutOpCalculatorTask::doWork( od_int64 start, od_int64 stop, int )
{
    const int incr = mCast( int, stop-start+1 );
    const int nrinlines = input_.getSize( 0 );
    const int nrcrosslines = input_.getSize( 1 );
    const int nrsamples = input_.getSize( 2 );
    TrcKeySamplingIterator iter;
    iter.setSampling( tkzsout_.hsamp_ );
    BinID curbid = iter.curBinID();
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
	if ( !iter.next() )
	{
	    addToNrDone( idx );
	    return true;
	}
	curbid = iter.curBinID();
    }

    addToNrDone( incr );
    return true;
}


uiString TutOpCalculatorTask::message() const
{
    return tr("Calculating Tutorial Operations");
}

}//namespace
