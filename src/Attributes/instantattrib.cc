/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2005
________________________________________________________________________

-*/

#include "instantattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsteering.h"
#include "survinfo.h"
#include "math2.h"

#include <math.h>

#define mOutAmplitude		0
#define mOutPhase		1
#define mOutFrequency		2
#define mOutHilbert		3
#define mOutAmplitude1Der	4
#define mOutAmplitude2Der	5
#define mOutCosPhase		6
#define mOutEnvWPhase		7
#define mOutEnvWFreq		8
#define mOutPhaseAccel		9
#define mOutThinBed		10
#define mOutBandwidth		11
#define mOutQFactor		12
#define mOutRotatePhase		13


namespace Attrib
{

mAttrDefCreateInstance(Instantaneous)

void Instantaneous::initClass()
{
    mAttrStartInitClassWithUpdate

    desc->addInput( InputSpec("Imag Data",true) );
    desc->setNrOutputs( Seis::UnknowData, 14 );

    FloatParam* rotangle_ = new FloatParam( rotateAngle() );
    rotangle_->setLimits( Interval<float>(-180,180) );
    rotangle_->setDefaultValue(90);
    rotangle_->setRequired( false );
    desc->addParam( rotangle_ );

    desc->addInput( InputSpec("Real Data",true) );

    desc->setLocality( Desc::SingleTrace );
    mAttrEndInitClass
}


void Instantaneous::updateDesc( Desc& desc )
{
    int outputidx = desc.selectedOutput();
    desc.setParamEnabled( rotateAngle(), outputidx == mOutRotatePhase );
}


Instantaneous::Instantaneous( Desc& ds )
    : Provider( ds )
    , sampgate1_( -1,1 )
    , sampgate2_( -2,2 )
    , rotangle_(0)
{
    if ( !isOK() ) return;

    mGetFloat( rotangle_, rotateAngle() );
}


bool Instantaneous::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool Instantaneous::getInputData( const BinID& relpos, int zintv )
{
    realdata_ = inputs_[0]->getData( relpos, zintv );
    imagdata_ = inputs_[1]->getData( relpos, zintv );
    realidx_ = getDataIndex( 0 );
    imagidx_ = getDataIndex( 1 );
    return realdata_ && imagdata_;
}


#define mGetRVal(sidx) getInputValue( *realdata_, realidx_, sidx, z0 )
#define mGetIVal(sidx) getInputValue( *imagdata_, imagidx_, sidx, z0 )


bool Instantaneous::computeData( const DataHolder& output, const BinID& relpos,
				 int z0, int nrsamples, int threadid ) const
{
    if ( !realdata_ || !imagdata_ ) return false;

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	if ( isOutputEnabled(mOutAmplitude) )
	    setOutputValue( output, mOutAmplitude, idx, z0,
			    calcAmplitude(idx,z0) );
	if ( isOutputEnabled(mOutPhase) )
	    setOutputValue( output, mOutPhase, idx, z0, calcPhase(idx,z0) );
	if ( isOutputEnabled(mOutFrequency) )
	    setOutputValue( output, mOutFrequency, idx, z0,
			    calcFrequency(idx,z0) );
	if ( isOutputEnabled(mOutHilbert) )
	    setOutputValue( output, mOutHilbert, idx, z0, mGetIVal(idx) );
	if ( isOutputEnabled(mOutAmplitude1Der) )
	    setOutputValue( output, mOutAmplitude1Der, idx, z0,
			    calcAmplitude1Der(idx,z0) );
	if ( isOutputEnabled(mOutAmplitude2Der) )
	    setOutputValue( output, mOutAmplitude2Der, idx, z0,
			    calcAmplitude2Der(idx,z0) );
	if ( isOutputEnabled(mOutCosPhase) )
	    setOutputValue( output, mOutCosPhase, idx, z0,
			    cos(calcPhase(idx,z0)) );
	if ( isOutputEnabled(mOutEnvWPhase) )
	    setOutputValue( output, mOutEnvWPhase, idx, z0,
			    calcEnvWPhase(idx,z0) );
	if ( isOutputEnabled(mOutEnvWFreq) )
	    setOutputValue( output, mOutEnvWFreq, idx, z0,
			    calcEnvWFreq(idx,z0) );
	if ( isOutputEnabled(mOutPhaseAccel) )
	    setOutputValue( output, mOutPhaseAccel, idx, z0,
			    calcPhaseAccel(idx,z0) );
	if ( isOutputEnabled(mOutThinBed) )
	    setOutputValue( output, mOutThinBed, idx, z0, calcThinBed(idx,z0) );
	if ( isOutputEnabled(mOutBandwidth) )
	    setOutputValue( output, mOutBandwidth, idx, z0,
			    calcBandWidth(idx,z0) );
	if ( isOutputEnabled(mOutQFactor) )
	    setOutputValue( output, mOutQFactor, idx, z0, calcQFactor(idx,z0) );
	if ( isOutputEnabled(mOutRotatePhase) )
	    setOutputValue(output, mOutRotatePhase, idx, z0,
			    calcRotPhase(idx,z0,rotangle_));
    }

    return true;
}

#define mCheckRetUdf(val1,val2) \
    if ( mIsUdf(val1) || mIsUdf(val2) ) return mUdf(float);

#define mCheckDenom(val) \
    if ( mIsZero(val,mDefEpsF) ) return mUdf(float);

float Instantaneous::calcAmplitude( int cursample, int z0 ) const
{
    const float real = mGetRVal( cursample );
    const float imag = mGetIVal( cursample );
    mCheckRetUdf( real, imag )
    return Math::Sqrt( real*real + imag*imag );
}

#define mDT ( 2.f * refstep_ )

float Instantaneous::calcAmplitude1Der( int cursample, int z0 ) const
{
    const float prev = calcAmplitude( cursample-1, z0 );
    const float next = calcAmplitude( cursample+1, z0 );
    mCheckRetUdf( prev, next )
    return ( next - prev ) / mDT;
}


float Instantaneous::calcAmplitude2Der( int cursample, int z0 ) const
{
    const float prev = calcAmplitude1Der( cursample-1, z0 );
    const float next = calcAmplitude1Der( cursample+1, z0 );
    mCheckRetUdf( prev, next )
    return ( next - prev ) / mDT;
}


float Instantaneous::calcPhase( int cursample, int z0 ) const
{
    const float real = mGetRVal( cursample );
    const float imag = mGetIVal( cursample );
    mCheckRetUdf( real, imag )
    return Math::Atan2( mGetIVal( cursample ), mGetRVal( cursample ) );
}


float Instantaneous::calcFrequency( int cursample, int z0 ) const
{
    const float prev = calcPhase( cursample-1, z0 );
    const float next = calcPhase( cursample+1, z0 );
    mCheckRetUdf( prev, next )

    float dphase = next - prev;
    if ( dphase < 0.f )
	dphase += M_2PIf;

    return dphase / ( M_2PIf * mDT );
}


float Instantaneous::calcPhaseAccel( int cursample, int z0 ) const
{
    const float prev = calcFrequency( cursample-1, z0 );
    const float next = calcFrequency( cursample+1, z0 );
    mCheckRetUdf( prev, next )
    return M_2PIf * ( next - prev ) / mDT;
}


float Instantaneous::calcBandWidth( int cursample, int z0 ) const
{
    const float denv_dt = calcAmplitude1Der( cursample, z0 );
    const float env = calcAmplitude( cursample, z0 );
    mCheckRetUdf( denv_dt, env )
    mCheckDenom( env )

    return fabs( denv_dt / ( M_2PIf * env ) );
}


float Instantaneous::calcQFactor( int cursample, int z0 ) const
{
    const float ifq = calcFrequency( cursample, z0 );
    const float bandwth = calcBandWidth( cursample, z0 );
    mCheckRetUdf( ifq, bandwth )
    mCheckDenom( bandwth )

    return -0.5f * ifq / bandwth;
}


float Instantaneous::calcRotPhase( int cursample, int z0, float angle ) const
{
    const float real = mGetRVal( cursample );
    const float imag = mGetIVal( cursample );
    mCheckRetUdf( real, imag )
    const float radians = Math::toRadians( angle );

    return real * cos( radians ) - imag * sin( radians );
}


float Instantaneous::calcEnvWeighted( int cursample, int z0,
				      bool isphase ) const
{
    float sumiampenv = 0.f;
    float sumienv = 0.f;
    int nrsamples = 0;
    Interval<int> sg( -1, 1 );
    for ( int ids=sg.start; ids<=sg.stop; ids++ )
    {
	const float iamp = isphase ? calcPhase( cursample+ids, z0 )
				   : calcFrequency( cursample+ids, z0 );
	const float ienv = calcAmplitude( cursample+ids, z0 );
	if ( mIsUdf(iamp) || mIsUdf(ienv) ) continue;

	sumienv += ienv;
	sumiampenv += iamp * ienv;
	nrsamples++;
    }

    if ( !nrsamples )
	return mUdf(float);

    mCheckDenom( sumienv )

    return sumiampenv / sumienv;
}


float Instantaneous::calcEnvWPhase( int cursample, int z0 ) const
{
    return calcEnvWeighted( cursample, z0, true );
}


float Instantaneous::calcEnvWFreq( int cursample, int z0 ) const
{
    return calcEnvWeighted( cursample, z0, false );
}


float Instantaneous::calcThinBed( int cursample, int z0 ) const
{
    const float freq = calcFrequency( cursample, z0 );
    const float envwfreq = calcEnvWFreq( cursample, z0 );
    mCheckRetUdf( freq, envwfreq )

    return freq - envwfreq;
}


const Interval<int>* Instantaneous::reqZSampMargin( int inp, int out ) const
{
    if ( out == 5 || out == 8 || out == 9 || out == 10 )
	return &sampgate2_;
    else if ( out == 2 || out == 4 || out == 7 || out == 11 || out == 12 )
	return &sampgate1_;
    else
	return 0;
}


}; // namespace Attrib
