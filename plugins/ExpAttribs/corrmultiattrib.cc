/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "corrmultiattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "statruncalc.h"
#include "survinfo.h"
#include "attribprovider.h"
#include "math2.h"

namespace Attrib
{

mAttrDefCreateInstance(CorrMultiAttrib)


void CorrMultiAttrib::initClass()
{
    mAttrStartInitClassWithDefaultsUpdate

    ZGateParam* gate = new ZGateParam( gateStr() );
    gate->setLimits( Interval<float>(-1000,1000) );
    gate->setDefaultValue( Interval<float>(-16,16) );
    desc->addParam( gate );
    desc->addOutputDataType( Seis::UnknowData );
    desc->addInput( InputSpec("Input data 1",true) );
    desc->addInput( InputSpec("Input data 2",true) );

    mAttrEndInitClass
}


CorrMultiAttrib::CorrMultiAttrib( Desc& ds )
	: Provider(ds)
	, gate_(0, 0)
{
    if ( !isOK() )
	return;

    mGetFloatInterval( gate_, gateStr() );
    gate_.scale(1.f/zFactor());
}


void CorrMultiAttrib::updateDefaults( Desc& desc )
{
    ValParam* paramgate = desc.getValParam( gateStr() );
    mDynamicCastGet( ZGateParam*, zgate, paramgate )
    float roundedzstep = SI().zStep()*SI().zDomain().userFactor();
    if ( roundedzstep>0 )
	roundedzstep = Math::Floor( roundedzstep );
    zgate -> setDefaultValue( Interval<float>(-roundedzstep*7,roundedzstep*7) );
}


bool CorrMultiAttrib::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool CorrMultiAttrib::getInputData( const BinID& relpos, int zintv )
{
    inputdata1_ = inputs_[0]->getData( relpos, zintv );
    inputdata2_ = inputs_[1]->getData( relpos, zintv );
    dataidx1_ = getDataIndex(0);
    dataidx2_ = getDataIndex(1);

    return inputdata1_ && inputdata2_;
}


bool CorrMultiAttrib::computeData( const DataHolder& output,
	      const BinID& relpos, int z0, int nrsamples, int threadid ) const
{
    if ( !inputdata1_ || inputdata1_->isEmpty() ||
	 !inputdata2_ || inputdata2_->isEmpty() )
	return false;

    const Interval<int> samplegate( mNINT32(gate_.start/refstep_),
	mNINT32(gate_.stop/refstep_) );

    for ( int sample=0; sample<nrsamples; sample++ )
    {
	float samp_diff = 0.f;
	float sumsample1sq = 0.f;
	float sumsample2sq = 0.f;
	float similarity, sample1, sample2;
	for ( int gatesample = samplegate.start;
		  gatesample < samplegate.stop+1; gatesample++ )
	{
	    const int sampidx = sample + gatesample;
	    sample1 = getInputValue( *inputdata1_, dataidx1_, sampidx, z0 );
	    sample2 = getInputValue( *inputdata2_, dataidx2_, sampidx, z0 );
	    if ( mIsUdf(sample1) || mIsUdf(sample2) )
	     continue;

	    samp_diff += (sample1-sample2) * (sample1-sample2);
	    sumsample1sq += sample1*sample1;
	    sumsample2sq += sample2*sample2;
	}

	if ( sumsample1sq!=0 || sumsample2sq!=0 )
	{
	    similarity = 1-Math::Sqrt(samp_diff)/
		( (Math::Sqrt(sumsample1sq)) + (Math::Sqrt(sumsample2sq)) );
	    setOutputValue( output, 0, sample, z0, similarity );
	}
	else
	    setOutputValue( output, 0, sample, z0, 1 );
    }
    return true;
}


const Interval<float>* CorrMultiAttrib::desZMargin(int inp,int) const
{
    return &gate_;
}

} // namespace Attrib
