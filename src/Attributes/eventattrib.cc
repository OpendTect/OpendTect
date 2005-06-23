/*+
  ________________________________________________________________________

  CopyRight:     (C) dGB Beheer B.V.
  Author:        Helene Payraudeau
  Date:          February 2005

  ________________________________________________________________________

-*/

#include "eventattrib.h"
#include "survinfo.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "datainpspec.h"
#include "genericnumer.h"
#include "valseriesinterpol.h"

#define SGWIDTH 1000

namespace Attrib
{

void Event::initClass()
{
    Desc* desc = new Desc( attribName(), updateDesc );
    desc->ref();

    desc->addParam( new ValParam(eventTypeStr(), new IntInpSpec()) );
    						//0 = Extr, 1 = Max, 2 = Min,
                                                 //3 = ZC, 4 = npZC, 5 = pnZC,
                                                 //6 = GateMax, 7 = GateMin
        
    desc->addParam( new BoolParam(issingleeventStr()) );
    desc->addParam( new BoolParam(tonextStr()) );
    
    ZGateParam* gate = new ZGateParam(gateStr());
    gate->setLimits( Interval<float>(-1000,1000) );
    desc->addParam( gate );

    desc->addOutputDataType( Seis::UnknowData );

    InputSpec inputspec( "Data on which the Events should be computed", true );
    desc->addInput( inputspec );

    desc->init();

    PF().addDesc( desc, createInstance );
    desc->unRef();
}


Provider* Event::createInstance( Desc& ds )
{
    Event* res = new Event( ds );
    res->ref();

    if ( !res->isOK() )
    {
	res->unRef();
	return 0;
    }

    res->unRefNoDelete();
    return res;
}


void Event::updateDesc( Desc& desc )
{
    bool issingle = 
	( (ValParam*)desc.getParam(issingleeventStr()) )-> getBoolValue();
    int evtype = ( (ValParam*)desc.getParam(eventTypeStr()) )-> getIntValue();
    if ( !issingle && ( evtype == 6 || evtype == 7 ) )
	desc.setParamEnabled(gateStr(),true);
    else
	desc.setParamEnabled(gateStr(),false);

    if ( issingle )
	desc.setNrOutputs( Seis::UnknowData, 2 );
}


Event::Event( Desc& desc_ )
    : Provider( desc_ )
{
    if ( !isOK() ) return;

    eventtype = ( (ValParam*)desc_.getParam(eventTypeStr()) )->getIntValue(0);
    mGetBool( issingleevent, issingleeventStr() );
    mGetBool( tonext, tonextStr() );
    
    mGetFloatInterval( gate, gateStr() );
    gate.start = gate.start / zFactor(); gate.stop = gate.stop / zFactor();
}


bool Event::getInputOutput( int input, TypeSet<int>& res ) const
{
        return Provider::getInputOutput( input, res );
}


bool Event::getInputData( const BinID& relpos, int idx )
{
    inputdata = inputs[0]->getData( relpos, idx );
    return inputdata;
}


VSEvent::Type Event::findEventType() const
{
    VSEvent::Type evtype;
    switch ( eventtype )
    {
	case 0: evtype = VSEvent::Extr;
	break;
	case 1: evtype = VSEvent::Max;
	break;
	case 2: evtype = VSEvent::Min;
	break;
	case 3: evtype = VSEvent::ZC;
	break;
	case 4: evtype = VSEvent::ZCNegPos;
	break;
	case 5: evtype = VSEvent::ZCPosNeg;
	break;
	case 6: evtype = VSEvent::GateMax;
	break;
	case 7: evtype = VSEvent::GateMin;
	break;
    }
    return evtype;
}


ValueSeriesEvent<float,float> Event::findNextEvent( 
					ValueSeriesEvent<float,float> nextev, 
					int dir, VSEvent::Type evtype,
					int nrsamples )
{
    ValueSeriesEvent<float,float> ev = nextev;
    Interval<float> sg;
    SamplingData<float> sd;
    ValueSeriesEvFinder<float,float> vsevfinder( *(inputdata->item(0)), 
	    					nrsamples, sd );
    sg.start = ev.pos + dir;
    sg.stop = sg.start + dir * SGWIDTH;
    nextev = vsevfinder.find( evtype, sg, 1 );
    int nrloops = 0;
    while ( (int)(nextev.pos)==(int)(ev.pos) && ( !mIsUndefined (nextev.pos) ) )
    {
	sg.start = ev.pos + dir * (2+nrloops);
	sg.stop = sg.start + dir * SGWIDTH;
	nextev = vsevfinder.find( evtype, sg, 1 );
	nrloops++;
    }
    return nextev;
}


void Event::singleEvent( TypeSet<float>& output, int nrsamples, int t0 )
{
    SamplingData<float> sd;
    ValueSeriesEvFinder<float,float> vsevfinder( *(inputdata->item(0)), 
	    					nrsamples, sd );
    VSEvent::Type zc = VSEvent::ZC;
    VSEvent::Type extr = VSEvent::Extr;
    Interval<float> sg(t0, t0-SGWIDTH);
    ValueSeriesEvent<float,float> ev = vsevfinder.find( zc, sg, 1 );
    if( mIsUndefined(ev.pos) )
    {
	sg.stop = t0 + SGWIDTH;
	ev = vsevfinder.find( zc, sg, 1 );
    }

    ValueSeriesEvent<float,float> extrev;
    sg.start = ev.pos + 1;
    sg.stop = sg.start + SGWIDTH;
    ValueSeriesEvent<float,float> nextev = vsevfinder.find( zc, sg, 1 );
    if( outputinterest[0] || outputinterest[2] )
	extrev = vsevfinder.find( extr, sg, 1 );

    for ( int idx = 0 ; idx<nrsamples ; idx++ )
    {
	const float cursample = t0 + idx;
	if ( cursample < ev.pos )
	    output[idx] = 0;
	else if ( cursample > nextev.pos )
	{
	    ev = nextev;
	    nextev = findNextEvent (nextev, 1, zc, nrsamples);
	    if ( outputinterest[0] || outputinterest[2] )
	    {
		sg.start = ev.pos + 1;
		sg.stop = sg.start + SGWIDTH;
		extrev = vsevfinder.find( extr, sg, 1 );
	    }
	}
	if ( cursample > ev.pos && cursample < nextev.pos)
	{
	    if ( outputinterest[0] )
		output[idx] = extrev.val/(nextev.pos - ev.pos);
	    else if ( outputinterest[1] )
	    {
		ValueSeriesInterpolator<float> interp;
		float lastsampval = interp.value( *(inputdata->item(0)),
							ev.pos -1 );
		float nextsampval = interp.value( *(inputdata->item(0)),
							ev.pos +1 );
		output[idx] =
		    fabs( (nextsampval - lastsampval) / 2 );
	    }
	    else if ( outputinterest[2] )
	    {
		float leftdist = extrev.pos - ev.pos;
		float rightdist = nextev.pos - extrev.pos;
		output[idx] = (rightdist-leftdist) / (rightdist+leftdist);
	    }
	}
    }
}


void Event::multipleEvents( TypeSet<float>& output , int nrsamples, int t0 )
{
    SamplingData<float> sd;
    ValueSeriesEvFinder<float,float> vsevfinder( *(inputdata->item(0)), 
	    					nrsamples, sd );
    VSEvent::Type evtype = findEventType();
    if ( evtype == VSEvent::GateMax || evtype == VSEvent::GateMin )
    {
	Interval<int> samplegate( mNINT(gate.start/refstep),
		                          mNINT(gate.stop/refstep) );
	int samplegatewidth = samplegate.width();
	int csample = t0 + samplegate.start;
	Interval<float> sg(0,0);
	for ( int idx=0; idx<nrsamples; idx++ )
	{
	    const int cursample = csample + idx;
	    sg.start = cursample;
	    sg.stop = sg.start + samplegatewidth;
	    ValueSeriesEvent<float,float> ev = vsevfinder.find( evtype, sg, 1 );
	    if ( mIsUndefined(ev.pos) )
		output[idx] = fabs( (t0 + idx) - ev.pos);
	    else
		output[idx] = fabs( (t0 + idx) - ev.pos) * refstep;
	}
	return;
    }

    Interval<float> sg(0,0);
    if ( nrsamples == 1 )
    {
	sg.start = t0;
	sg.stop = tonext ? sg.start + SGWIDTH : sg.start - SGWIDTH;
	ValueSeriesEvent<float,float> ev = vsevfinder.find( evtype, sg, 1 );
	if ( mIsUndefined(ev.pos) )
	    output[0] = fabs( t0 - ev.pos );
	else
	    output[0] = fabs( t0 - ev.pos ) * refstep;
    }
    else
    {
	sg.start = tonext ? t0 : t0 + nrsamples;
	int direction = tonext ? 1 : -1;
	sg.stop = sg.start -direction * SGWIDTH;
	ValueSeriesEvent<float,float> ev = vsevfinder.find( evtype, sg, 1 );
	if ( mIsUndefined(ev.pos) )
	{
	    sg.stop = t0 + direction * SGWIDTH;
	    ev = vsevfinder.find( evtype, sg, 1 );
	}
	ValueSeriesEvent<float,float> nextev = 
	    		findNextEvent( ev, direction, evtype, nrsamples );

	if ( tonext )
	{
	    for ( int idx = 0; idx<nrsamples; idx++ )
	    {
		const int cursample = t0 + idx;
		if ( cursample < ev.pos )
		    output[idx] = 0;
		else if ( cursample > nextev.pos )
		{
		    ev = nextev;
		    nextev = findNextEvent(nextev, 1, evtype, nrsamples);
		}
		if ( cursample > ev.pos && cursample < nextev.pos)
		{
		    if ( mIsUndefined(nextev.pos) )
			output[idx] = (nextev.pos - ev.pos);
		    else 
			output[idx] = (nextev.pos - ev.pos) * refstep;
		}
	    }
	}
	else
	{
	    for ( int idx=nrsamples-1; idx>=0; idx-- )
	    {
		const int cursample = t0 + idx;
		if ( cursample > ev.pos )
		    output[idx] = 0;
		else if ( cursample < nextev.pos )
		{
		    ev = nextev;
		    nextev = findNextEvent(nextev, -1, evtype, nrsamples);
		}
		if ( cursample < ev.pos && cursample > nextev.pos)
		{
		    if ( mIsUndefined(nextev.pos) )
			output[idx] = (ev.pos - nextev.pos);
		    else
			output[idx] = (ev.pos - nextev.pos) * refstep;
		}
	    }
	}
    }
}

    
bool Event::computeData( const DataHolder& output,
			const BinID& relpos,
			int t0, int nrsamples ) const
{
    TypeSet<float> outp(nrsamples,0);
    if ( !inputdata ) return false;

    if ( issingleevent )
        const_cast<Event*>(this)->singleEvent(outp, nrsamples, t0);
    else
        const_cast<Event*>(this)->multipleEvents(outp, nrsamples, t0);

    for ( int idx=0; idx<nrsamples; idx++ )
    {
            if ( outputinterest[0] ) 
		output.item(0)->setValue(idx, outp[idx]);
	    else if ( outputinterest[1] ) 
		output.item(1)->setValue(idx, outp[idx]);
	    else if ( outputinterest[2] )
		output.item(2)->setValue(idx, outp[idx]);
    }
    return true;
}


const Interval<float>* Event::reqZMargin(int input, int output) const
{
    VSEvent::Type evtype = findEventType();
    if ( evtype == VSEvent::GateMax || evtype == VSEvent::GateMin )
	return &gate;
    else
	return 0;
}


}; // namespace Attrib
