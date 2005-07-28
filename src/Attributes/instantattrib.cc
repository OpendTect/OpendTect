/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: instantattrib.cc,v 1.3 2005-07-28 10:53:50 cvshelene Exp $
________________________________________________________________________

-*/

#include "instantattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsteering.h"
#include "datainpspec.h"
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
    gate.start = -SI().zRange().step; gate.stop = SI().zRange().step;
}


bool Instantaneous::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool Instantaneous::getInputData( const BinID& relpos, int idx )
{
    realdata = inputs[0]->getData( relpos, idx );
    imagdata = inputs[1]->getData( relpos, idx );
    return realdata && imagdata;
}


#define mGetRVal( sidx ) realdata->item(0)->value( sidx - realdata->t0_ )
#define mGetIVal( sidx ) -imagdata->item(0)->value( sidx - imagdata->t0_ )


bool Instantaneous::computeData( const DataHolder& output, const BinID& relpos, 
				 int t0, int nrsamples ) const
{
    if ( !realdata || !imagdata ) return false;

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const int cursample = t0 + idx;
	if ( outputinterest[0] )
	    output.item(0)->setValue( idx, calcAmplitude(cursample) );
	else if ( outputinterest[1] )
	    output.item(1)->setValue( idx, calcPhase(cursample) );
	else if ( outputinterest[2] )
	    output.item(2)->setValue( idx, calcFrequency(cursample) );
	else if ( outputinterest[3] )
	    output.item(3)->setValue( idx, mGetIVal(cursample) );
	else if ( outputinterest[4] )
	    output.item(4)->setValue( idx, calcAmplitude1Der(cursample) );
	else if ( outputinterest[5] )
	    output.item(5)->setValue( idx, calcAmplitude2Der(cursample) );
	else if ( outputinterest[6] )
	    output.item(6)->setValue( idx, cos(calcPhase(cursample)) );
	else if ( outputinterest[7] )
	    output.item(7)->setValue( idx, calcEnvWPhase(cursample) );
	else if ( outputinterest[8] )
	    output.item(8)->setValue( idx, calcEnvWFreq(cursample) );
	else if ( outputinterest[9] )
	    output.item(9)->setValue( idx, calcPhaseAccel(cursample) );
	else if ( outputinterest[10] )
	    output.item(10)->setValue( idx, calcThinBed(cursample) );
	else if ( outputinterest[11] )
	    output.item(11)->setValue( idx, calcBandWidth(cursample) );
	else if ( outputinterest[12] )
	    output.item(12)->setValue( idx, calcQFactor(cursample) );
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
