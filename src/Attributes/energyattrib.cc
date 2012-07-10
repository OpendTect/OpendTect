/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUnusedVar = "$Id: energyattrib.cc,v 1.42 2012-07-10 08:05:29 cvskris Exp $";

#include "energyattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "math2.h"
#include "statruncalc.h"
#include "survinfo.h"


namespace Attrib
{

mAttrDefCreateInstance(Energy)
    
void Energy::initClass()
{
    mAttrStartInitClassWithDefaultsUpdate

    ZGateParam* gate = new ZGateParam( gateStr() );
    gate->setLimits( Interval<float>(-1000,1000) );
    Interval<float> defgate( -28, 28 );
    gate->setDefaultValue( defgate );
    desc->addParam( gate );

    BoolParam* dograd = new BoolParam( dogradStr(), false, false );
    desc->addParam( dograd );

    desc->addInput( InputSpec("Input Data",true) );
    desc->setNrOutputs( Seis::UnknowData, 3 );

    desc->setLocality( Desc::SingleTrace );
    mAttrEndInitClass
}


Energy::Energy( Desc& ds )
    : Provider(ds)
    , dessampgate_(0,0)
{
    if ( !isOK() ) return;

    mGetBool( dograd_, dogradStr() );
    mGetFloatInterval( gate_, gateStr() );
    gate_.scale( 1./zFactor() );
    if ( dograd_ )
	dessampgate_ = Interval<int>(-1,1);
}


void Energy::updateDefaults( Desc& desc )
{
    ValParam* paramgate = desc.getValParam(gateStr());
    mDynamicCastGet( ZGateParam*, zgate, paramgate )
    float roundedzstep = SI().zStep()*SI().zDomain().userFactor();
    if ( roundedzstep > 0 )
	roundedzstep = (int)( roundedzstep );
    zgate->setDefaultValue( Interval<float>(-roundedzstep*7, roundedzstep*7) );
}


bool Energy::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool Energy::getInputData( const BinID& relpos, int zintv )
{
    inputdata_ = inputs_[0]->getData( relpos, zintv );
    dataidx_ = getDataIndex( 0 );
    return inputdata_;
}


bool Energy::allowParallelComputation() const
{
    return !dograd_;
}


bool Energy::computeData( const DataHolder& output, const BinID& relpos,
			  int z0, int nrsamples, int threadid ) const
{
    if ( !inputdata_ || inputdata_->isEmpty() || output.isEmpty() )
	return false;

    Interval<int> samplegate( mNINT32(gate_.start/refstep_),
			      mNINT32(gate_.stop/refstep_) );
    const int sz = samplegate.width() + 1;
    Stats::WindowedCalc<float> wcalc(
	    Stats::CalcSetup().require(Stats::SqSum), sz );

    const int stopidx = sz + nrsamples - 1;
    for ( int idx=0; idx<stopidx; idx++ )
    {
	wcalc += getInputValue( *inputdata_, dataidx_, samplegate.start+idx,z0);
	const int outidx = idx - sz + 1;
	if ( outidx >= 0 )
	{
	    float val = wcalc.sqSum() / sz;
	    if ( isOutputEnabled(0) )
		setOutputValue( output, 0, outidx, z0, val );
	    if ( isOutputEnabled(1) )
		setOutputValue( output, 1, outidx, z0, Math::Sqrt(val) );
	    if ( isOutputEnabled(2) )
		setOutputValue( output, 2, outidx, z0, Math::Log(val) );
	}
    }

    //post processing for gradient
    if ( dograd_ )
    {
	for ( int ido=0; ido<nrOutputs(); ido++ )
	{
	    if ( !isOutputEnabled(ido) ) continue;
	    float prevval = mUdf(float);
	    for ( int isamp=0; isamp<output.nrsamples_; isamp++ )
	    {
		const float curval = output.series(ido)->value(isamp);
		float nextval = isamp < output.nrsamples_-1
				? output.series(ido)->value(isamp+1)
				: mUdf(float);
		float gradval;
		if ( mIsUdf(prevval) && mIsUdf(nextval) )
		{
		    const bool xtratops = inputdata_->z0_<(z0+samplegate.start);
		    const bool xtrabots = inputdata_->z0_+inputdata_->nrsamples_
					    >(z0+nrsamples+samplegate.stop);
		    if ( nrsamples == 1 && ( xtratops || xtrabots) )
		    {
			wcalc.clear();
			const int startidx = xtratops ? samplegate.start-1
						      : samplegate.start;
			const int lastidx = sz + (xtratops ? 1 : 0)
			    		       + (xtrabots ? 1 : 0);
			for ( int idx=0; idx<lastidx; idx++ )
			{
			    wcalc += getInputValue( *inputdata_, dataidx_,
						    startidx+idx, z0 );
			    if ( idx<sz-1 ) continue;
			    float enval = wcalc.sqSum() / sz;
			    if ( ido==1 )
				enval = Math::Sqrt(enval);
			    else if ( ido==2 )
				enval = Math::Log(enval);

			    if ( idx==sz-1 && xtratops )
				prevval = enval; 
			    else if ( (idx==sz+1) || ( idx==sz && !xtratops) )
				nextval = enval;
			}
		    }
		}
		
		if ( mIsUdf(prevval) )
		{
		    if ( mIsUdf(nextval) )
			gradval = mUdf(float);
		    else
			gradval = nextval - curval;
		}
		else if ( mIsUdf(nextval) )
		    gradval = curval - prevval;
		else
		    gradval = (nextval - prevval)/2;

		prevval = curval;
		setOutputValue( output, ido, isamp, z0, gradval );
	    }
	}
    }

    return true;
}

} // namespace Attrib
