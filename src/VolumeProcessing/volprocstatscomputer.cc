/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Helene Huck
 Date:		September 2015
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "volprocstatscomputer.h"

#include "arrayndimpl.h"
#include "flatposdata.h"
#include "iopar.h"
#include "paralleltask.h"
#include "seisdatapack.h"
#include "statruncalc.h"
#include "threadlock.h"

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


TrcKeySampling StatsCalculator::getInputHRg( const TrcKeySampling& hrg ) const
{
    TrcKeySampling res = hrg;
    res.start_.inl() = hrg.start_.inl() - res.step_.inl() * stepout_.row();
    res.start_.crl() = hrg.start_.crl() - res.step_.crl() * stepout_.col();
    res.stop_.inl() = hrg.stop_.inl() + res.step_.inl() * stepout_.row();
    res.stop_.crl() = hrg.stop_.crl() + res.step_.crl() * stepout_.col();
    return res;
}

/*
StepInterval<int> StatsCalculator::getInputZRg(
				const StepInterval<int>& zrg ) const
{
    StepInterval<int> res = zrg;
    res.start = zrg.start - res.step * nzsampextra_;
    res.stop = zrg.stop + res.step * nzsampextra_;
    return res;
}
*/

Task* StatsCalculator::createTask()
{
    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );
    const RegularSeisDataPack* input = getInput( getInputSlotID(0) );
    if ( !input || !output ) return 0;

    const int nrcompsinput = input->nrComponents();
    for ( int idx=0; idx<nrcompsinput; idx++ )
    {
	if ( output->nrComponents()<=idx )
	{
	    if ( ! const_cast<RegularSeisDataPack*>(output)
				->addComponent( input->getComponentName(idx) ) )
		return 0;
	}
	else
	    output->setComponentName( input->getComponentName(idx), idx );
    }

    const TrcKeyZSampling tkzsin = input->sampling();
    const TrcKeyZSampling tkzsout = output->sampling();
    TaskGroup* taskgrp = new TaskGroup();
    for ( int idx=0; idx<nrcompsinput; idx++ )
    {
	Task* task = new StatsCalculatorTask( input->data( idx ), tkzsin,
					      tkzsout, stepout_, nzsampextra_,
					      statstype_,
					      output->data( idx ) );
	taskgrp->addTask( task );
    }
    return taskgrp;
}

//--------- StatsCalculatorTask -------------


StatsCalculatorTask::StatsCalculatorTask( const Array3D<float>& input,
					  const TrcKeyZSampling& tkzsin,
					  const TrcKeyZSampling& tkzsout,
					  BinID stepout, int nzsampextra,
					  BufferString, Array3D<float>& output )
    : input_( input )
    , output_( output )
    , stepout_( stepout )
    , nzsampextra_ ( nzsampextra )
    , tkzsin_( tkzsin )
    , tkzsout_( tkzsout )
{
    shape_ = sKeyEllipse();	//only possible choice for now
    totalnr_ = output.info().getSize(0) * output.info().getSize(1);
    prepareWork();
}


bool StatsCalculatorTask::doWork( od_int64 start, od_int64 stop, int )
{
    //for now only median and shape Ellipse for dip filtering
    BinID curbid;
    const int incr = mCast( int, stop-start+1 );
    const int nrinlines = input_.info().getSize( 0 );
    const int nrcrosslines = input_.info().getSize( 1 );
    const int nrsamples = input_.info().getSize( 2 );
    const int nrpos = positions_.size();
    TrcKeySamplingIterator iter;
    iter.setSampling( tkzsout_.hsamp_ );
    iter.setNextPos( tkzsout_.hsamp_.trcKeyAt(start) );
    iter.next( curbid );
    Stats::CalcSetup rcsetup;
    rcsetup.require( Stats::Median );
    const int statsz = nrpos * (nzsampextra_*2+1);
    Stats::WindowedCalc<double> wcalc( rcsetup, statsz );
    for ( int idx=0; idx<incr; idx++ )
    {
	int inpinlidx = curbid.inl()-tkzsin_.hsamp_.start_.inl();
	int inpcrlidx = curbid.crl()-tkzsin_.hsamp_.start_.crl();

	int outpinlidx = curbid.inl()-tkzsout_.hsamp_.start_.inl();
	int outpcrlidx = curbid.crl()-tkzsout_.hsamp_.start_.crl();

	for ( int idz=0; idz<nrsamples; idz++ )
	{
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
		    wcalc += input_.get( valididxi, valididxc, valididxz );
		}
	    }
	    const float outval = (float) wcalc.getValue(Stats::Median);
	    Threads::Locker locker( datalock_ );
	    output_.set( outpinlidx, outpcrlidx, idz, outval );
	    locker.unlockNow();
	}
	iter.next( curbid );
    }

    addToNrDone( incr );
    return true;
}


uiString StatsCalculatorTask::uiMessage() const
{
    return tr("Computing Statistics");
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
