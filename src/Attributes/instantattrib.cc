/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2005
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "instantattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsteering.h"
#include "survinfo.h"
#include "math2.h"

#include <math.h>

namespace Attrib
{

mAttrDefCreateInstance(Instantaneous)
    
void Instantaneous::initClass()
{
    mAttrStartInitClass

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
#define mGetIVal(sidx) - getInputValue( *imagdata_, imagidx_, sidx, z0 )


bool Instantaneous::computeData( const DataHolder& output, const BinID& relpos, 
				 int z0, int nrsamples, int threadid ) const
{
    if ( !realdata_ || !imagdata_ ) return false;

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	if ( isOutputEnabled(0) )
	    setOutputValue( output, 0, idx, z0, calcAmplitude(idx,z0) );
	if ( isOutputEnabled(1) )
	    setOutputValue( output, 1, idx, z0, calcPhase(idx,z0) );
	if ( isOutputEnabled(2) )
	    setOutputValue( output, 2, idx, z0, calcFrequency(idx,z0) );
	if ( isOutputEnabled(3) )
	    setOutputValue( output, 3, idx, z0, mGetIVal(idx) );
	if ( isOutputEnabled(4) )
	    setOutputValue( output, 4, idx, z0, calcAmplitude1Der(idx,z0) );
	if ( isOutputEnabled(5) )
	    setOutputValue( output, 5, idx, z0, calcAmplitude2Der(idx,z0) );
	if ( isOutputEnabled(6) )
	    setOutputValue( output, 6, idx, z0, cos(calcPhase(idx,z0)) );
	if ( isOutputEnabled(7) )
	    setOutputValue( output, 7, idx, z0, calcEnvWPhase(idx,z0) );
	if ( isOutputEnabled(8) )
	    setOutputValue( output, 8, idx, z0, calcEnvWFreq(idx,z0) );
	if ( isOutputEnabled(9) )
	    setOutputValue( output, 9, idx, z0, calcPhaseAccel(idx,z0) );
	if ( isOutputEnabled(10) )
	    setOutputValue( output, 10, idx, z0, calcThinBed(idx,z0) );
	if ( isOutputEnabled(11) )
	    setOutputValue( output, 11, idx, z0, calcBandWidth(idx,z0) );
	if ( isOutputEnabled(12) )
	    setOutputValue( output, 12, idx, z0, calcQFactor(idx,z0) );
	if ( isOutputEnabled(13) )
	    setOutputValue(output, 13, idx, z0, calcRotPhase(idx,z0,rotangle_));
    }

    return true;
}

#define mCheckRetUdf(val1,val2) \
    if ( mIsUdf(val1) || mIsUdf(val2) ) return mUdf(float);

float Instantaneous::calcAmplitude( int cursample, int z0 ) const
{
    const float real = mGetRVal( cursample );
    const float imag = mGetIVal( cursample );
    mCheckRetUdf( real, imag );
    return Math::Sqrt( real*real + imag*imag );
}


float Instantaneous::calcAmplitude1Der( int cursample, int z0 ) const
{
    const int step mUnusedVar = 1;
    const float prev = calcAmplitude( cursample-1, z0 );
    const float next = calcAmplitude( cursample+1, z0 );
    mCheckRetUdf( prev, next );
    return (next-prev) / (2*refstep_);
}


float Instantaneous::calcAmplitude2Der( int cursample, int z0 ) const
{
    const float prev = calcAmplitude1Der( cursample-1, z0 );
    const float next = calcAmplitude1Der( cursample+1, z0 );
    mCheckRetUdf( prev, next );
    return (next-prev) / (2*refstep_);
}


float Instantaneous::calcPhase( int cursample, int z0 ) const
{
    const float real = mGetRVal( cursample );
    const float imag = mGetIVal( cursample );
    if ( mIsZero(real,mDefEps) ) return M_PI/2;
    mCheckRetUdf( real, imag );
    return atan2(imag,real);
}


float Instantaneous::calcFrequency( int cursample, int z0 ) const
{
    const float real = mGetRVal( cursample );
    const float prevreal = mGetRVal( cursample-1 );
    const float nextreal = mGetRVal( cursample+1 );
    mCheckRetUdf( prevreal, nextreal );
    const float dreal_dt = (nextreal - prevreal) / (2*refstep_);

    const float imag = mGetIVal( cursample );
    const float previmag = mGetIVal( cursample-1 );
    const float nextimag = mGetIVal( cursample+1 );
    mCheckRetUdf( previmag, nextimag );
    const float dimag_dt = (nextimag-previmag) / (2*refstep_);

    float denom = (real*real + imag*imag);
    if ( mIsZero( denom, 1e-6 ) ) denom = 1e-6;
    return (real*dimag_dt - imag*dreal_dt) / denom;
}


float Instantaneous::calcPhaseAccel( int cursample, int z0 ) const
{
    const float prev = calcFrequency( cursample-1, z0 );
    const float next = calcFrequency( cursample+1, z0 );
    mCheckRetUdf( prev, next );
    return (next-prev) / (2*refstep_);
}


float Instantaneous::calcBandWidth( int cursample, int z0 ) const
{
    const float denv_dt = calcAmplitude1Der( cursample, z0 );
    const float env = calcAmplitude( cursample, z0 );
    mCheckRetUdf( denv_dt, env );
    return (float)fabs(denv_dt / (2*M_PI* ( mIsZero(env,1e-6) ? 1e-6 : env )));
}


float Instantaneous::calcQFactor( int cursample, int z0 ) const
{
    const float ifq = calcFrequency( cursample, z0 );
    const float bandwth = calcBandWidth( cursample, z0 );
    mCheckRetUdf( ifq, bandwth );
    return (-0.5f * ifq / ( mIsZero(bandwth,1e-6) ? 1e-6f : bandwth ) );
}


float Instantaneous::calcRotPhase( int cursample, int z0, float angle ) const
{
    const float real = mGetRVal( cursample );
    const float imag = mGetIVal( cursample );
    return (float) (real*cos( angle*M_PI/180 ) - imag*sin( angle*M_PI/180 ));
}


float Instantaneous::calcRMSAmplitude( int cursample, int z0 ) const
{
    Interval<int> sg( -1, 1 );
    int nrsamples = 0;
    float sumia2 = 0;
    for ( int ids=sg.start; ids<=sg.stop; ids++ )
    {
	const float ia = calcAmplitude( cursample+ids, z0 );
	if ( mIsUdf(ia) ) continue;
	sumia2 += ia*ia;
	nrsamples++;
    }
    
    float dt = (nrsamples-1) * refstep_;
    if ( mIsZero( dt, 1e-6 ) ) dt = 1e-6;
    return Math::Sqrt( sumia2/dt );
}


float Instantaneous::calcEnvWPhase( int cursample, int z0 ) const
{
    const float rmsia = calcRMSAmplitude( cursample, z0 );
    if ( mIsZero(rmsia,mDefEps) ) return 0;
    if ( mIsUdf(rmsia) ) return mUdf(float);

    float sumia = 0;
    float sumiaiph = 0;
    Interval<int> sg( -1, 1 );
    for ( int ids=sg.start; ids<=sg.stop; ids++ )
    {
	const float ia = calcAmplitude( cursample+ids, z0 );
	const float iph = calcPhase( cursample+ids, z0 );
	if ( mIsUdf(ia) || mIsUdf(iph) ) continue;

	sumia += ia/rmsia;
	sumiaiph += ia*iph/rmsia;
    }

    return sumiaiph / ( mIsZero(sumia,1e-6) ? 1e-6f : sumia );
}


float Instantaneous::calcEnvWFreq( int cursample, int z0 ) const
{
    const float rmsia = calcRMSAmplitude( cursample, z0 );
    if ( mIsZero(rmsia,mDefEps) ) return 0;

    float sumia = 0;
    float sumiaifq = 0;
    Interval<int> sg( -1, 1 );
    for ( int ids=sg.start; ids<=sg.stop; ids++ )
    {
	const float ia = calcAmplitude( cursample+ids, z0 );
	const float ifq = calcFrequency( cursample+ids, z0 );
	sumia += ia/rmsia;
	sumiaifq += ia*ifq/rmsia;
    }

    return sumiaifq / ( mIsZero(sumia,1e-6) ? 1e-6f : sumia );
}


float Instantaneous::calcThinBed( int cursample, int z0 ) const
{
    return calcFrequency( cursample, z0 ) - calcEnvWFreq( cursample, z0 );
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
