/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          December 2004
 RCS:           $Id: scalingattrib.cc,v 1.3 2005-08-01 12:17:00 cvsnanne Exp $
________________________________________________________________________

-*/

#include "scalingattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "datainpspec.h"

#define mStatsTypeRMS	0
#define mStatsTypeMean	1
#define mStatsTypeMax 	2
#define mStatsTypeUser 	3

#define mScalingTypeTPower   0
#define mScalingTypeWindow   1

static inline float interpolator( float fact1, float fact2, 
				  float t1, float t2, float curt )
{
    if ( !mIsZero((t2-t1+2),mDefEps) )
    {
    const float increment = (fact2-fact1) / (t2-t1+2);
    return fact1 + increment * (curt-t1+1);
    }
    else
	return fact1;
}

namespace Attrib
{

void Scaling::initClass()
{
    Desc* desc = new Desc( attribName(), updateDesc );
    desc->ref();

    EnumParam* scalingtype = new EnumParam( scalingTypeStr() );
    scalingtype->addEnum(scalingTypeNamesStr(mScalingTypeTPower));
    scalingtype->addEnum(scalingTypeNamesStr(mScalingTypeWindow));
    desc->addParam(scalingtype);

    FloatInpSpec* floatspec = new FloatInpSpec();
    floatspec->setLimits( Interval<float>(0,mUndefValue) );
    ValParam* powerval = new ValParam( powervalStr(), floatspec );
    powerval->setDefaultValue("1");
    powerval->setRequired(false);
    desc->addParam( powerval );
    
    ZGateParam gate( gateStr() );
    gate.setLimits( Interval<float>(-mLargestZGate,mLargestZGate) );

    ParamGroup<ZGateParam>* gateset
		= new ParamGroup<ZGateParam>( 0, gateStr(), gate );
    desc->addParam( gateset );

    FloatInpSpec* floatinpspec = new FloatInpSpec();
    floatinpspec->setLimits( Interval<float>(0,mUndefValue) );
    ValParam factor( factorStr(), floatinpspec );
    factor.setDefaultValue("1");

    ParamGroup<ValParam>* factorset
		= new ParamGroup<ValParam>( 0, factorStr(), factor );
    desc->addParam( factorset );

    EnumParam* statstype = new EnumParam(statsTypeStr());
    statstype->addEnum(statsTypeNamesStr(mStatsTypeRMS));
    statstype->addEnum(statsTypeNamesStr(mStatsTypeMean));
    statstype->addEnum(statsTypeNamesStr(mStatsTypeMax));
    statstype->addEnum(statsTypeNamesStr(mStatsTypeUser));
    desc->addParam( statstype );

    desc->addOutputDataType( Seis::UnknowData );

    InputSpec inputspec( "Data on which the Scaling should be applied", true );
    desc->addInput( inputspec );

    desc->init();

    PF().addDesc( desc, createInstance );
    desc->unRef();
}

    
Provider* Scaling::createInstance( Desc& ds )
{
    Scaling* res = new Scaling( ds );
    res->ref();

    if ( !res->isOK() )
    {
	res->unRef();
	return 0;
    }

    res->unRefNoDelete();
    return res;
}


void Scaling::updateDesc( Desc& desc )
{
    bool isuserdef = ((ValParam*)desc.getParam(statsTypeStr()))->getBoolValue();
    desc.setParamEnabled(factorStr(),isuserdef);
}


const char* Scaling::statsTypeNamesStr(int type)
{
    if ( type==mStatsTypeRMS ) return "RMS";
    if ( type==mStatsTypeMean ) return "Mean";
    if ( type==mStatsTypeMax ) return "Max";
    return "User-defined";
}


const char* Scaling::scalingTypeNamesStr(int type)
{
    if ( type==mScalingTypeTPower ) return "T^n";
    return "Window";
}


Scaling::Scaling( Desc& desc_ )
    : Provider( desc_ )
{
    if ( !isOK() ) return;

    mGetEnum( scalingtype, scalingTypeStr() );
    mGetFloat( powerval, powervalStr() );

    mDescGetParamGroup(ZGateParam,gateset,desc,gateStr())
    for ( int idx=0; idx<gateset->size(); idx++ )
    {
	const ValParam& param = (ValParam&)(*gateset)[idx];
	Interval<float> interval( param.getfValue(0), param.getfValue(1) );
	interval.sort(); interval.scale( 1./zFactor() );
	gates += interval;
    }
    
    mGetEnum( statstype, statsTypeStr() );

    if ( statstype == mStatsTypeUser )
    {
	mDescGetParamGroup(ValParam,factorset,desc,factorStr())
	for ( int idx=0; idx<factorset->size(); idx++ )
	    factors += ((ValParam&)((*factorset)[idx])).getfValue( 0 );
    }
}

bool Scaling::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool Scaling::getInputData( const BinID& relpos, int idx )
{
    inputdata = inputs[0]->getData( relpos, idx );
    return inputdata ? true : false;
}
    

bool Scaling::computeData( const DataHolder& output, const BinID& relpos,
			      int t0, int nrsamples ) const
{
    if ( scalingtype == mScalingTypeTPower )
    {
	scaleTimeN( output, t0, nrsamples );
	return true;
    }

    TypeSet<Interval<int> > samplegates;
    checkTimeGates( gates, samplegates, t0, nrsamples );

    RunningStatistics<float> stats;
    TypeSet<float> scalefactors;
    if ( statstype != mStatsTypeUser )
    {
	for ( int sgidx=0; sgidx<gates.size(); sgidx++ )
	{
	    const Interval<int>& sg = samplegates[sgidx];
	    if ( !sg.start && !sg.stop )
	    {
		scalefactors += 1;
		continue;
	    }
	    
	    for ( int idx=sg.start; idx<=sg.stop; idx++ )
		stats += fabs( inputdata->item(0)->value(idx-inputdata->t0_) );

	    if ( statstype == mStatsTypeRMS )
		scalefactors += stats.rms()? 1/stats.rms() : 1;
	    else if ( statstype == mStatsTypeMean )
		scalefactors += stats.mean() ? 1/stats.mean() : 1;
	    else
		scalefactors += stats.max() ? 1/stats.max() : 1;
	}
    }
    else
	scalefactors = scalefactors;


    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const float trcval = inputdata->item(0)->value(idx - inputdata->t0_);
	float scalefactor = 1;
	bool found = false;
	int cursample = t0 + idx;
	for ( int sgidx=0; sgidx<samplegates.size(); sgidx++ )
	{
	    if ( !found && samplegates[sgidx].includes(cursample) )
	    {
		if ( sgidx+1 < samplegates.size() && 
		     samplegates[sgidx+1].includes(cursample) )
		{
		    scalefactor = interpolator( scalefactors[sgidx], 
			    			scalefactors[sgidx+1],
						samplegates[sgidx+1].start,
						samplegates[sgidx].stop,
						cursample );
		}
		else
		    scalefactor = scalefactors[sgidx];

		found = true;
	    }
	}

	output.item(0)->setValue(idx, trcval * scalefactor);
    }

    return true;
}


void Scaling::scaleTimeN(const DataHolder& output, int t0, int nrsamples) const
{
    float result;
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const float curt = t0*refstep + idx*refstep;
	result = pow(curt,powerval) * 
	    		inputdata->item(0)->value(idx - inputdata->t0_);
	output.item(0)->setValue(idx, result);
    }
}


void Scaling::checkTimeGates( const TypeSet< Interval<float> >& oldtgs,
				  TypeSet<Interval<int> >& newsampgates,
				  int t0, int nrsamples ) const
{
    for( int idx=0; idx<oldtgs.size(); idx++ )
    {
	Interval<int> sg;
	sg.start = (int) (oldtgs[idx].start / refstep);
	sg.stop = (int) (oldtgs[idx].stop / refstep);
	if ( sg.start > nrsamples || sg.stop < t0 )
	{
	    newsampgates += Interval<int>(0,0);
	    continue;
	}

	if ( sg.start < t0 ) sg.start = t0;
	if ( sg.stop > nrsamples ) sg.stop = nrsamples;
	newsampgates += sg;
    }
}

}//namespace
