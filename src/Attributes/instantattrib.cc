/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: instantattrib.cc,v 1.7 2005-11-29 19:35:42 cvsnanne Exp $
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

void Instantaneous::initClass()
{
    Desc* desc = new Desc( attribName(), updateDesc );
    desc->ref();

    ZGateParam* gate = new ZGateParam( gateStr() );
    gate->setLimits( Interval<float>(-1000,1000) );
    desc->addParam( gate );

    desc->addInput( InputSpec("Real Data",true) );
    desc->addInput( InputSpec("Imag Data",true) );
    desc->setNrOutputs( Seis::UnknowData, 13 );

    desc->init();
    PF().addDesc( desc, createInstance );
    desc->unRef();
}


Provider* Instantaneous::createInstance( Desc& desc )
{
    Instantaneous* res = new Instantaneous( desc );
    res->ref();

    if ( !res->isOK() )
    {
	res->unRef();
	return 0;
    }

    res->unRefNoDelete();
    return res;
}


void Instantaneous::updateDesc( Desc& desc )
{
    desc.setParamEnabled( gateStr(), false );
}


Instantaneous::Instantaneous( Desc& desc_ )
    : Provider( desc_ )
{
    if ( !isOK() ) return;

//  mGetFloatInterval( gate, gateStr() );
//  gate.start = gate.start / zFactor(); gate.stop = gate.stop / zFactor();
    gate.start = -SI().zStep(); gate.stop = SI().zStep();
}


bool Instantaneous::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool Instantaneous::getInputData( const BinID& relpos, int idx )
{
    realdata = inputs[0]->getData( relpos, idx );
    imagdata = inputs[1]->getData( relpos, idx );
    realidx_ = getDataIndex( 0 );
    imagidx_ = getDataIndex( 1 );
    return realdata && imagdata;
}


#define mGetRVal( sidx ) realdata->series(realidx_)->value(sidx-realdata->z0_)
#define mGetIVal( sidx ) -imagdata->series(imagidx_)->value(sidx-imagdata->z0_)


bool Instantaneous::computeData( const DataHolder& output, const BinID& relpos, 
				 int z0, int nrsamples ) const
{
    if ( !realdata || !imagdata ) return false;

    const int offset = output.z0_ - z0;
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const int cursample = z0 + idx;
	const int curidx = offset + idx;
	if ( outputinterest[0] )
	    output.series(0)->setValue( curidx, calcAmplitude(cursample) );
	if ( outputinterest[1] )
	    output.series(1)->setValue( curidx, calcPhase(cursample) );
	if ( outputinterest[2] )
	    output.series(2)->setValue( curidx, calcFrequency(cursample) );
	if ( outputinterest[3] )
	    output.series(3)->setValue( curidx, mGetIVal(cursample) );
	if ( outputinterest[4] )
	    output.series(4)->setValue( curidx, calcAmplitude1Der(cursample) );
	if ( outputinterest[5] )
	    output.series(5)->setValue( curidx, calcAmplitude2Der(cursample) );
	if ( outputinterest[6] )
	    output.series(6)->setValue( curidx, cos(calcPhase(cursample)) );
	if ( outputinterest[7] )
	    output.series(7)->setValue( curidx, calcEnvWPhase(cursample) );
	if ( outputinterest[8] )
	    output.series(8)->setValue( curidx, calcEnvWFreq(cursample) );
	if ( outputinterest[9] )
	    output.series(9)->setValue( curidx, calcPhaseAccel(cursample) );
	if ( outputinterest[10] )
	    output.series(10)->setValue( curidx, calcThinBed(cursample) );
	if ( outputinterest[11] )
	    output.series(11)->setValue( curidx, calcBandWidth(cursample) );
	if ( outputinterest[12] )
	    output.series(12)->setValue( curidx, calcQFactor(cursample) );
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

    return (real*dimag_dt - imag*dreal_dt) / (real*real + imag*imag);
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
    return denv_dt / (2*M_PI*env);
}


float Instantaneous::calcQFactor( int cursample ) const
{
    const float ifq = calcFrequency( cursample );
    const float bandwth = calcBandWidth( cursample );
    return (-0.5 * ifq / bandwth);
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
    
    const float dt = (nrsamples-1) * refstep;
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

    return sumiaiph / sumia;
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

    return sumiaifq / sumia;

}


float Instantaneous::calcThinBed( int cursample ) const
{
    return calcFrequency( cursample ) - calcEnvWFreq( cursample );
}

}; // namespace Attrib
