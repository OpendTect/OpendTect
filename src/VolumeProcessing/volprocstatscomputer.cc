/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Helene Huck
 Date:		September 2015
________________________________________________________________________

-*/


#include "volprocstatscomputer.h"

#include "arrayndimpl.h"
#include "flatposdata.h"
#include "iopar.h"
#include "paralleltask.h"
#include "seisdatapack.h"
#include "statruncalc.h"
#include "trckeyzsampling.h"
#include "keystrs.h"

namespace VolProc
{

StatsCalculator::StatsCalculator()
    : Step()
{}


void StatsCalculator::fillPar( IOPar& par ) const
{
    Step::fillPar( par );

    par.set( sKey::Filter(), statstype_ );
    par.set( sKey::StepOutInl(), stepout_.row() );
    par.set( sKey::StepOutCrl(), stepout_.col() );
    par.set( sZSampMargin(), nzsampextra_ );
}


bool StatsCalculator::usePar( const IOPar& par )
{
    if ( !Step::usePar( par ) )
	return false;

    if ( !par.get( sKey::StepOutInl(), stepout_.row() ) ||
	 !par.get( sKey::StepOutCrl(), stepout_.col() ) ||
	 !par.get( sZSampMargin(), nzsampextra_ ) ||
	 !par.get( sKey::Filter(), statstype_ ) )
	return false;

    return true;
}


bool StatsCalculator::copyComponentsSel( const InputSlotID inpslotid,
					 OutputSlotID& outslotid ) const
{
    const bool correctslot = inpslotid == 0;
    if ( correctslot )
	outslotid = inpslotid;

    return correctslot;
}


ReportingTask* StatsCalculator::createTask()
{
    if ( !prepareWork() )
	return 0;

    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );
    const RegularSeisDataPack* input = getInput( getInputSlotID(0) );

    const int nrcompsinput = input->nrComponents();
    for ( int idx=0; idx<nrcompsinput; idx++ )
	output->setComponentName( input->getComponentName(idx), idx );

    const TrcKeyZSampling tkzsin( input->subSel() );
    const TrcKeyZSampling tkzsout( output->subSel() );
    TaskGroup* taskgrp = new TaskGroup();
    for ( int idx=0; idx<nrcompsinput; idx++ )
    {
	Task* task = new StatsCalculatorTask( input->data( idx ), tkzsin,
					      tkzsout, stepout_, nzsampextra_,
					      statstype_, output->data( idx ) );
	taskgrp->addTask( task );
    }

    return taskgrp;
}


//--------- StatsCalculatorTask -------------


StatsCalculatorTask::StatsCalculatorTask( const Array3D<float>& input,
					  const TrcKeyZSampling& tkzsin,
					  const TrcKeyZSampling& tkzsout,
					  BinID stepout, int nzsampextra,
					  BufferString statstype,
					  Array3D<float>& output )
    : input_( input )
    , output_( output )
    , stepout_( stepout )
    , nzsampextra_ ( nzsampextra )
    , tkzsin_( tkzsin )
    , tkzsout_( tkzsout )
    , statstype_(statstype)
{
    shape_ = sKeyEllipse();	//only possible choice for now
    totalnr_ = output.getSize(0) * output.getSize(1);
    prepareWork();
}


bool StatsCalculatorTask::doWork( od_int64 start, od_int64 stop, int )
{
    //for now only median and average with shape Ellipse for dip filtering
    //We might reconsider the handling of undefs in corners
    const int incr = mCast( int, stop-start+1 );
    const int nrinlines = input_.getSize( 0 );
    const int nrcrosslines = input_.getSize( 1 );
    const int nrsamples = input_.getSize( 2 );
    const int nrpos = positions_.size();
    TrcKeySamplingIterator iter;
    iter.setSampling( tkzsout_.hsamp_ );
    iter.setCurrentPos( start );
    const int statsz = nrpos * (nzsampextra_*2+1);
    std::vector<float> values( statsz, mUdf(float) );
    const bool needmed = statstype_ == sKey::Median();
    const float* inparr = input_.getData();
    float value = mUdf(float);
    for ( int idx=0; idx<incr; idx++, iter.next(), addToNrDone(1) )
    {
	const TrcKey trk( iter.curTrcKey() );
	const int inl = trk.lineNr();
	const int crl = trk.trcNr();
	const int inpinlidx = tkzsin_.lineIdx( inl );
	const int inpcrlidx = tkzsin_.trcIdx( crl );

	const int outpinlidx = tkzsout_.lineIdx( inl );
	const int outpcrlidx = tkzsout_.trcIdx( crl );

	for ( int idz=0; idz<nrsamples; idz++ )
	{
	    int valposidx = 0; float sum = 0;
	    for ( int posidx=0; posidx<nrpos; posidx++ )
	    {
		const int theoricidxi = inpinlidx+positions_[posidx].inl();
		const int valididxi = theoricidxi<0 ? 0
						    : theoricidxi>=nrinlines
							? nrinlines-1
							: theoricidxi;
		const int theoricidxc = inpcrlidx+positions_[posidx].crl();
		const int valididxc = theoricidxc<0 ? 0
						    : theoricidxc>=nrcrosslines
							? nrcrosslines-1
							: theoricidxc;
		for ( int idxz=-nzsampextra_; idxz<=nzsampextra_ ; idxz++ )
		{
		    const int valididxz =
			idxz+idz<0 ? 0 : idxz+idz>=nrsamples ? nrsamples-1
							     : idxz+idz;
		    if ( inparr )
			value = inparr[ valididxz + valididxc*nrsamples
					+ (od_int64) valididxi*nrsamples*nrcrosslines ];
		    else
			value = input_.get( valididxi, valididxc, valididxz);
		    if ( needmed )
			values[valposidx++] = value;
		    else
			sum += value;
		}
	    }

	    float outval;
	    if ( needmed )
	    {
		std::nth_element( values.begin(), values.begin() + statsz/2,
				  values.end() );
		outval = values[statsz/2];
	    }
	    else
		outval = sum / statsz;

	    output_.set( outpinlidx, outpcrlidx, idz, outval );
	}
    }

    return true;
}


uiString StatsCalculatorTask::message() const
{
    return tr("Calculating Statistics");
}


void StatsCalculatorTask::prepareWork()
{
    BinID pos;
    for ( pos.inl()=-stepout_.inl(); pos.inl()<=stepout_.inl(); pos.inl()++ )
    {
	for ( pos.crl()=-stepout_.crl(); pos.crl()<=stepout_.crl(); pos.crl()++)
	{
	    const float relinldist =
			stepout_.inl() ? ((float)pos.inl())/stepout_.inl() : 0;
	    const float relcrldist =
			stepout_.crl() ? ((float)pos.crl())/stepout_.crl() : 0;

	    const float dist2 = relinldist*relinldist + relcrldist*relcrldist;
	    if ( shape_==BufferString(sKeyEllipse()) && dist2>1 )
		continue;

	    positions_ += pos;
	}
    }
}


}//namespace
