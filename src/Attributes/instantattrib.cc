/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: instantattrib.cc,v 1.12 2007-10-05 14:03:39 cvshelene Exp $
________________________________________________________________________

-*/

#include "instantattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsteering.h"
#include "survinfo.h"

#include <math.h>

namespace Attrib
{

mAttrDefCreateInstance(Instantaneous)
    
void Instantaneous::initClass()
{
    mAttrStartInitClass

    desc->addInput( InputSpec("Real Data",true) );
    desc->addInput( InputSpec("Imag Data",true) );
    desc->setNrOutputs( Seis::UnknowData, 13 );

    mAttrEndInitClass
}


Instantaneous::Instantaneous( Desc& ds )
    : Provider( ds )
    , sampgate1_( -1,1 )
    , sampgate2_( -2,2 )
{
    if ( !isOK() ) return;
}


bool Instantaneous::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool Instantaneous::getInputData( const BinID& relpos, int zintv )
{
    realdata_ = inputs[0]->getData( relpos, zintv );
    imagdata_ = inputs[1]->getData( relpos, zintv );
    realidx_ = getDataIndex( 0 );
    imagidx_ = getDataIndex( 1 );
    return realdata_ && imagdata_;
}


#define mGetRVal(sidx) realdata_->series(realidx_)->value(sidx-realdata_->z0_)
#define mGetIVal(sidx) -imagdata_->series(imagidx_)->value(sidx-imagdata_->z0_)


bool Instantaneous::computeData( const DataHolder& output, const BinID& relpos, 
				 int z0, int nrsamples, int threadid ) const
{
    if ( !realdata_ || !imagdata_ ) return false;

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const int cursample = z0 + idx;
	const int outidx = z0 - output.z0_ + idx;
	if ( outputinterest[0] )
	    output.series(0)->setValue( outidx, calcAmplitude(cursample) );
	if ( outputinterest[1] )
	    output.series(1)->setValue( outidx, calcPhase(cursample) );
	if ( outputinterest[2] )
	    output.series(2)->setValue( outidx, calcFrequency(cursample) );
	if ( outputinterest[3] )
	    output.series(3)->setValue( outidx, mGetIVal(cursample) );
	if ( outputinterest[4] )
	    output.series(4)->setValue( outidx, calcAmplitude1Der(cursample) );
	if ( outputinterest[5] )
	    output.series(5)->setValue( outidx, calcAmplitude2Der(cursample) );
	if ( outputinterest[6] )
	    output.series(6)->setValue( outidx, cos(calcPhase(cursample)) );
	if ( outputinterest[7] )
	    output.series(7)->setValue( outidx, calcEnvWPhase(cursample) );
	if ( outputinterest[8] )
	    output.series(8)->setValue( outidx, calcEnvWFreq(cursample) );
	if ( outputinterest[9] )
	    output.series(9)->setValue( outidx, calcPhaseAccel(cursample) );
	if ( outputinterest[10] )
	    output.series(10)->setValue( outidx, calcThinBed(cursample) );
	if ( outputinterest[11] )
	    output.series(11)->setValue( outidx, calcBandWidth(cursample) );
	if ( outputinterest[12] )
	    output.series(12)->setValue( outidx, calcQFactor(cursample) );
    }

    return true;
}


float Instantaneous::calcAmplitude( int cursample ) const
{
    const float real = mGetRVal( cursample );
    const float imag = mGetIVal( cursample );
    return sqrt( real*real + imag*imag );
}


float Instantaneous::calcAmplitude1Der( int cursample ) const
{
    const int step = 1;
    const float prev = calcAmplitude( cursample-1 );
    const float next = calcAmplitude( cursample+1 );
    return (next-prev) / (2*refstep);
}


float Instantaneous::calcAmplitude2Der( int cursample ) const
{
    const float prev = calcAmplitude1Der( cursample-1 );
    const float next = calcAmplitude1Der( cursample+1 );
    return (next-prev) / (2*refstep);
}


float Instantaneous::calcPhase( int cursample ) const
{
    const float real = mGetRVal( cursample );
    const float imag = mGetIVal( cursample );
    if ( mIsZero(real,mDefEps) ) return M_PI/2;
    return atan2(imag,real);
}


float Instantaneous::calcFrequency( int cursample ) const
{
    const float real = mGetRVal( cursample );
    const float prevreal = mGetRVal( cursample-1 );
    const float nextreal = mGetRVal( cursample+1 );
    const float dreal_dt = (nextreal - prevreal) / (2*refstep);

    const float imag = mGetIVal( cursample );
    const float previmag = mGetIVal( cursample-1 );
    const float nextimag = mGetIVal( cursample+1 );
    const float dimag_dt = (nextimag-previmag) / (2*refstep);

    float denom = (real*real + imag*imag);
    if ( mIsZero( denom, 1e-6 ) ) denom = 1e-6;
    return (real*dimag_dt - imag*dreal_dt) / denom;
}


float Instantaneous::calcPhaseAccel( int cursample ) const
{
    const float prev = calcFrequency( cursample-1 );
    const float next = calcFrequency( cursample+1 );
    return (next-prev) / (2*refstep);
}


float Instantaneous::calcBandWidth( int cursample ) const
{
    const float denv_dt = calcAmplitude1Der( cursample );
    const float env = calcAmplitude( cursample );
    return denv_dt / (2*M_PI* ( mIsZero(env,1e-6) ? 1e-6 : env ) );
}


float Instantaneous::calcQFactor( int cursample ) const
{
    const float ifq = calcFrequency( cursample );
    const float bandwth = calcBandWidth( cursample );
    return (-0.5 * ifq / ( mIsZero(bandwth,1e-6) ? 1e-6 : bandwth ) );
}


float Instantaneous::calcRMSAmplitude( int cursample ) const
{
    Interval<int> sg( -1, 1 );
    int nrsamples = 0;
    float sumia2 = 0;
    for ( int ids=sg.start; ids<=sg.stop; ids++ )
    {
	const float ia = calcAmplitude( cursample+ids );
	sumia2 += ia*ia;
	nrsamples++;
    }
    
    float dt = (nrsamples-1) * refstep;
    if ( mIsZero( dt, 1e-6 ) ) dt = 1e-6;
    return sqrt( sumia2/dt );
}


float Instantaneous::calcEnvWPhase( int cursample ) const
{
    const float rmsia = calcRMSAmplitude( cursample );
    if ( mIsZero(rmsia,mDefEps) ) return 0;

    float sumia = 0;
    float sumiaiph = 0;
    Interval<int> sg( -1, 1 );
    for ( int ids=sg.start; ids<=sg.stop; ids++ )
    {
	const float ia = calcAmplitude( cursample+ids );
	const float iph = calcPhase( cursample+ids );
	sumia += ia/rmsia;
	sumiaiph += ia*iph/rmsia;
    }

    return sumiaiph / ( mIsZero(sumia,1e-6) ? 1e-6 : sumia );
}


float Instantaneous::calcEnvWFreq( int cursample ) const
{
    const float rmsia = calcRMSAmplitude( cursample );
    if ( mIsZero(rmsia,mDefEps) ) return 0;

    float sumia = 0;
    float sumiaifq = 0;
    Interval<int> sg( -1, 1 );
    for ( int ids=sg.start; ids<=sg.stop; ids++ )
    {
	const float ia = calcAmplitude( cursample+ids );
	const float ifq = calcFrequency( cursample+ids );
	sumia += ia/rmsia;
	sumiaifq += ia*ifq/rmsia;
    }

    return sumiaifq / ( mIsZero(sumia,1e-6) ? 1e-6 : sumia );
}


float Instantaneous::calcThinBed( int cursample ) const
{
    return calcFrequency( cursample ) - calcEnvWFreq( cursample );
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
