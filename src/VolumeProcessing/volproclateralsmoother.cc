/*+
 *(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 *Author:	K. Tingdahl
 *Date:		Feb 2008
-*/

static const char* rcsID = "$Id: volproclateralsmoother.cc,v 1.5 2010-04-20 22:03:25 cvskris Exp $";

#include "volproclateralsmoother.h"

#include "arrayndslice.h"
#include "arrayndimpl.h"
#include "keystrs.h"
#include "survinfo.h"


namespace VolProc
{


class LateralSmootherTask : public ParallelTask
{
public:
	LateralSmootherTask(const Array3D<float>& input,
		int i0, int i1, int i2,
		Array3D<float>& output,
		int o0, int o1, int o2, const Array2DFilterPars& pars )
	    : input_( input )
	    , output_( output )
	    , i0_( i0 ), i1_( i1 ), i2_( i2 )
	    , o0_( o0 ), o1_( o1 ), o2_( o2 )
	    , pars_( pars )
	    , totalsz_( output.info().getSize( 2 ) )
	{}

    od_int64			nrIterations() const { return totalsz_; }
    const char*			message() const { return "Smothing laterally"; }
    const char*			nrDoneText() const
    				{ return "Timeslices processed"; }

private:
    bool doWork( od_int64 start, od_int64 stop, int )
    {
	Array2DImpl<float> inputslice( input_.info().getSize(0),
				       input_.info().getSize(1) );
	if ( !inputslice.isOK() )
	    return false;

	Array2DSlice<float> output( output_ );
	output.setDimMap( 0, 0 );
	output.setDimMap( 1, 1 );

	for ( od_int64 idx=start; idx<=stop && shouldContinue();
	      idx++, addToNrDone( 1 ) )
	{
	    const int depthindex = o2_+idx;
	    const int inputdepth = depthindex-i2_;

	    for ( int idy=inputslice.info().getSize(0)-1; idy>=0; idy-- )
	    {
		for ( int idz=inputslice.info().getSize(1)-1; idz>=0; idz-- )
		    inputslice.set( idy, idz, input_.get(idy,idz,inputdepth) );
	    }

	    PtrMan<Array2DFilterer<float> > filter =
		new Array2DFilterer<float>( inputslice, pars_ );
	    if ( !filter->execute() )
		return false;

	    output.setPos( 2, idx );

	    if ( !output.init() )
		return false;

	    for ( int idy=output.info().getSize(0)-1; idy>=0; idy-- )
	    {
		const int input0 = o0_+idy - i0_;
		for ( int idz=output.info().getSize(1)-1; idz>=0; idz-- )
		{
		    const int input1 = o1_+idz - i1_;
		    output.set( idy, idz, inputslice.get( input0, input1 ) );
		}
	    }
	}

	return true;
    }
		
    od_int64			totalsz_;

    const Array3D<float>&	input_;
    int				i0_, i1_, i2_;

    Array3D<float>&		output_;
    int				o0_, o1_, o2_;
    const Array2DFilterPars&	pars_;
};


void LateralSmoother::initClass()
{
    VolProc::PS().addCreator( create, LateralSmoother::sKeyType(),
	    LateralSmoother::sUserName() );
}


const char* LateralSmoother::type() const
{ return sKeyType(); }


bool LateralSmoother::needsInput(const HorSampling&) const
{ return true; }
    
    
LateralSmoother::LateralSmoother(Chain& pc)
    : Step( pc )
{ }


LateralSmoother::~LateralSmoother()
{ }    


HorSampling LateralSmoother::getInputHRg( const HorSampling& hrg ) const
{
    HorSampling res = hrg;
    res.start.inl = hrg.start.inl - res.step.inl * pars_.stepout_.row;
    res.start.crl = hrg.start.crl - res.step.crl * pars_.stepout_.col;
    res.stop.inl = hrg.stop.inl + res.step.inl * pars_.stepout_.row;
    res.stop.crl = hrg.stop.crl + res.step.crl * pars_.stepout_.col;
    return res;
}


Step*  LateralSmoother::create( Chain& pc )
{ return new LateralSmoother( pc ); }


void LateralSmoother::setPars( const Array2DFilterPars& pars )
{
    pars_ = pars;
}


void LateralSmoother::fillPar( IOPar& pars ) const
{
    Step::fillPar( pars );

    pars.setYN( sKeyIsMedian(), pars_.type_==Stats::Median );
    pars.setYN( sKeyIsWeighted(),
	    pars_.type_!=Stats::Median && !mIsUdf(pars_.rowdist_) );
    pars.set( sKey::StepOutInl, pars_.stepout_.row );
    pars.set( sKey::StepOutCrl, pars_.stepout_.col );
}


bool LateralSmoother::usePar( const IOPar& pars )
{
    if ( !Step::usePar( pars ) )
	return false;

    bool ismedian, isweighted;
    if ( !pars.getYN( sKeyIsMedian(), ismedian ) || 
	 !pars.get( sKey::StepOutInl, pars_.stepout_.row ) ||
	 !pars.get( sKey::StepOutCrl, pars_.stepout_.col ) ||
	 !pars.getYN( sKeyIsWeighted(), isweighted ) )
    {
	return false;
    }

    pars_.type_ = ismedian ? Stats::Median : Stats::Average;
    pars_.rowdist_ = isweighted ? 1 : mUdf(float);

    return true;
}


Task* LateralSmoother::createTask()
{
    if ( !input_ || !output_ )
	return 0;

    if ( input_->inlsampling_.step!=output_->inlsampling_.step ||
	 input_->crlsampling_.step!=output_->crlsampling_.step || 
	 !mIsEqual(input_->zstep_,output_->zstep_,1e-3*SI().zRange(true).step)) 
    {
	return 0;
    }

    if ( pars_.type_!=Stats::Median )
    {
	if ( !mIsUdf(pars_.rowdist_) )
	{
	    pars_.rowdist_ = (SI().inlDistance()*input_->inlsampling_.step)/
			     (SI().crlDistance()*input_->crlsampling_.step);
	}
    }
    else
    {
	pars_.rowdist_ = mUdf(float);
    }

    return new LateralSmootherTask( input_->getCube( 0 ),
	    input_->inlsampling_.start,
	    input_->crlsampling_.start,
	    input_->z0_,
	    output_->getCube( 0 ),
	    output_->inlsampling_.start,
	    output_->crlsampling_.start,
	    output_->z0_,
	    pars_ );

    return 0;
}


}; //namespace
