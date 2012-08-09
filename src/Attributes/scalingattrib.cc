/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          December 2004
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: scalingattrib.cc,v 1.49 2012-08-09 04:38:06 cvssalil Exp $";

#include "scalingattrib.h"

#include "agc.h"
#include "arrayndimpl.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "statruncalc.h"
#include "squeezing.h"
#include "survinfo.h"
#include <math.h>

#define mStatsTypeRMS		0
#define mStatsTypeMean		1
#define mStatsTypeMax 		2
#define mStatsTypeUser 		3
#define mStatsTypeDetrend 	4

#define mScalingTypeZPower   0
#define mScalingTypeWindow   1
#define mScalingTypeAGC	     2			
#define mScalingTypeSqueeze  3			
#define mScalingTypeGain     4			

static inline float interpolator( float fact1, float fact2, 
				  float t1, float t2, float curt )
{
    if ( !mIsZero((t2-t1+2),mDefEps) )
    {
	const float increment = (fact2-fact1) / (t2-t1+2);
	return fact1 + increment * (curt-t1+1);
    }

    return fact1;
}

namespace Attrib
{

mAttrDefCreateInstance(Scaling)    
    
void Scaling::initClass()
{
    mAttrStartInitClassWithUpdate

    EnumParam* scalingtype = new EnumParam( scalingTypeStr() );
    scalingtype->addEnum( scalingTypeNamesStr(mScalingTypeZPower) );
    scalingtype->addEnum( scalingTypeNamesStr(mScalingTypeWindow) );
    scalingtype->addEnum( scalingTypeNamesStr(mScalingTypeAGC) );
    scalingtype->addEnum( scalingTypeNamesStr(mScalingTypeSqueeze) );
    scalingtype->addEnum( scalingTypeNamesStr(mScalingTypeGain) );
    desc->addParam( scalingtype );

    FloatParam* powerval = new FloatParam( powervalStr() );
    desc->setParamEnabled( powervalStr(), false );
    desc->addParam( powerval );
    
    ZGateParam gate( gateStr() );
    gate.setLimits( Interval<float>(-mLargestZGate,mLargestZGate) );

    ParamGroup<ZGateParam>* gateset
		= new ParamGroup<ZGateParam>( 0, gateStr(), gate );
    desc->addParam( gateset );

    FloatParam factor( factorStr() );
    factor.setLimits( Interval<float>(0,mUdf(float)) );

    ParamGroup<ValParam>* factorset
		= new ParamGroup<ValParam>( 0, factorStr(), factor );
    desc->addParam( factorset );

    EnumParam* statstype = new EnumParam( statsTypeStr() );
    statstype->addEnum( statsTypeNamesStr(mStatsTypeRMS) );
    statstype->addEnum( statsTypeNamesStr(mStatsTypeMean) );
    statstype->addEnum( statsTypeNamesStr(mStatsTypeMax) );
    statstype->addEnum( statsTypeNamesStr(mStatsTypeUser) );
    statstype->addEnum( statsTypeNamesStr(mStatsTypeDetrend) );
    statstype->setDefaultValue( mStatsTypeRMS );
    desc->addParam( statstype );

    FloatParam* widthval = new FloatParam( widthStr() );
    widthval->setLimits( Interval<float>(0,mUdf(float)) );
    desc->setParamEnabled( widthStr(), false );
    desc->addParam( widthval );
    widthval->setDefaultValue( 500 );

    FloatGateParam* fgparm = new FloatGateParam(sqrangeStr());
    fgparm->setDefaultValue( Interval<float>(mUdf(float),mUdf(float)) );
    fgparm->setRequired( false );
    desc->addParam( fgparm );
    fgparm = new FloatGateParam(squntouchedStr());
    fgparm->setDefaultValue( Interval<float>(mUdf(float),mUdf(float)) );
    fgparm->setRequired( false );
    desc->addParam( fgparm );

    FloatParam* mutefractionval = new FloatParam( mutefractionStr() );
    mutefractionval->setLimits( Interval<float>(0,mUdf(float)) );
    desc->setParamEnabled( mutefractionStr(), false );
    desc->addParam( mutefractionval );
    mutefractionval->setDefaultValue( 0 );

    desc->addOutputDataType( Seis::UnknowData );
    desc->addInput( InputSpec("Input data",true) );

    desc->setLocality( Desc::SingleTrace );
    mAttrEndInitClass
}

    
void Scaling::updateDesc( Desc& desc )
{
    BufferString type = desc.getValParam( scalingTypeStr() )->getStringValue();
    
    desc.setParamEnabled( powervalStr(), false );
    desc.setParamEnabled( statsTypeStr(), false );
    desc.setParamEnabled( gateStr(), false );
    desc.setParamEnabled( widthStr(), false );
    desc.setParamEnabled( factorStr(), false );
    desc.setParamEnabled( mutefractionStr(), false );
    desc.setParamEnabled( sqrangeStr(), false );
    desc.setParamEnabled( squntouchedStr(), false );
    
    if ( type == scalingTypeNamesStr(mScalingTypeZPower) )
	desc.setParamEnabled( powervalStr(), true );
    else if ( type == scalingTypeNamesStr(mScalingTypeWindow) ||
	      type == scalingTypeNamesStr(mScalingTypeGain) )
    {
	if ( type == scalingTypeNamesStr(mScalingTypeWindow) )
	    desc.setParamEnabled( statsTypeStr(), true );

	desc.setParamEnabled( gateStr(), true );
	const int statstype = desc.getValParam(statsTypeStr())->getIntValue();
	desc.setParamEnabled( factorStr(), statstype==mStatsTypeUser );
    }
    else if ( type == scalingTypeNamesStr(mScalingTypeAGC) )
    {
	desc.setParamEnabled( widthStr(), true );
	desc.setParamEnabled( mutefractionStr(), true );
    }
    else if ( type == scalingTypeNamesStr(mScalingTypeSqueeze) )
    {
	desc.setParamEnabled( sqrangeStr(), true );
	desc.setParamEnabled( squntouchedStr(), true );
    }
}


const char* Scaling::statsTypeNamesStr( int type )
{
    if ( type==mStatsTypeRMS ) return "RMS";
    if ( type==mStatsTypeMean ) return "Mean";
    if ( type==mStatsTypeMax ) return "Max";
    if ( type==mStatsTypeUser ) return "User-defined";
    return "Detrend";
}


const char* Scaling::scalingTypeNamesStr( int type )
{
    //still T in parfile for backward compatibility
    if ( type==mScalingTypeZPower ) return "T^n";
    if ( type==mScalingTypeAGC ) return "AGC";
    if ( type==mScalingTypeSqueeze ) return "Squeeze";
    if ( type==mScalingTypeGain ) return "Gain Correction";
    return "Window";
}

#define mGetSqueezeRgs( var, varstring ) \
{\
    var.start = desc_.getValParam(varstring)->getfValue(0); \
    var.stop = desc_.getValParam(varstring)->getfValue(1); \
    if ( mIsUdf(var.start) || mIsUdf(var.stop) )\
    {\
	Attrib::ValParam* valparam##var = \
	      const_cast<Attrib::ValParam*>(desc_.getValParam(varstring));\
	mDynamicCastGet(Attrib::ZGateParam*,gateparam##var,valparam##var);\
	if ( gateparam##var ) \
	{ \
	    if ( mIsUdf(var.start) ) \
	        var.start = gateparam##var->getDefaultGateValue().start;\
	    if ( mIsUdf(var.stop) ) \
	        var.stop = gateparam##var->getDefaultGateValue().stop;\
	}\
    }\
}

Scaling::Scaling( Desc& desc )
    : Provider( desc )
{
    if ( !isOK() ) return;

    mGetEnum( scalingtype_, scalingTypeStr() );
    mGetFloat( powerval_, powervalStr() );
    mGetFloat( width_, widthStr() );
    mGetFloat( mutefraction_, mutefractionStr() );

    mGetSqueezeRgs( sqrg_, sqrangeStr() );
    mGetSqueezeRgs( squrg_, squntouchedStr() );

    mDescGetParamGroup(ZGateParam,gateset,desc_,gateStr())
    for ( int idx=0; idx<gateset->size(); idx++ )
    {
	const ValParam& param = (ValParam&)(*gateset)[idx];
	Interval<float> interval( param.getfValue(0), param.getfValue(1) );
	interval.sort(); interval.scale( 1.f/zFactor() );
	gates_ += interval;
    }
    
    mGetEnum( statstype_, statsTypeStr() );
    if ( statstype_ == mStatsTypeUser || scalingtype_ == mScalingTypeGain )
    {
	mDescGetParamGroup(ValParam,factorset,desc_,factorStr())
	for ( int idx=0; idx<factorset->size(); idx++ )
	    factors_ += ((ValParam&)((*factorset)[idx])).getfValue( 0 );
    }
    
    desgate_ = Interval<int>( -(1024-1), 1024-1 );
    window_ = Interval<float>( -width_/(2.f*SI().zDomain().userFactor()), 
				width_/(2.f*SI().zDomain().userFactor()) );
}


bool Scaling::allowParallelComputation() const
{
    return scalingtype_ != mScalingTypeAGC && statstype_ != mStatsTypeDetrend;
}


bool Scaling::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool Scaling::getInputData( const BinID& relpos, int zintv )
{
    inputdata_ = inputs_[0]->getData( relpos, zintv );
    dataidx_ = getDataIndex( 0 );
    return inputdata_;
}


void Scaling::getScaleFactorsFromStats( const TypeSet<Interval<int> >& sgates,
					TypeSet<float>& scalefactors,
       					int z0 ) const
{
    Stats::Type statstype = Stats::Max;
    if ( statstype_ == mStatsTypeRMS )
	statstype = Stats::RMS;
    else if ( statstype_ == mStatsTypeMean )
	statstype = Stats::Average;

    Stats::RunCalc<float> stats( Stats::CalcSetup().require(statstype) );

    for ( int sgidx=0; sgidx<gates_.size(); sgidx++ )
    {
	const Interval<int>& sg = sgates[sgidx];
	if ( !sg.start && !sg.stop )
	{
	    scalefactors += 1;
	    continue;
	}

	for ( int idx=sg.start; idx<=sg.stop; idx++ )
	    stats += getInputValue( *inputdata_, dataidx_, idx-z0, z0 );

	float val = (float)stats.getValue( statstype );
	scalefactors += !mIsZero(val,mDefEps) ? 1.f/val : 1;
	stats.clear();
    }
}
    

void Scaling::getTrendsFromStats( const TypeSet<Interval<int> >& sgates,
				  int z0 )
{
    trends_.erase();
    Stats::Type statstype = Stats::Sum;
    Stats::RunCalc<float> stats( Stats::CalcSetup().require(statstype) );
    Stats::RunCalc<float> statsidx( Stats::CalcSetup().require(statstype) );

    for ( int sgidx=0; sgidx<gates_.size(); sgidx++ )
    {
	const Interval<int>& sg = sgates[sgidx];
	if ( !sg.start && !sg.stop )
	{
	    trends_ += Trend(1,0);
	    continue;
	}

	float crosssum = 0;
	int nrindexes = 0;
	for ( int idx=sg.start; idx<=sg.stop; idx++ )
	{
	    float val = getInputValue( *inputdata_, dataidx_, idx-z0, z0 );
	    stats += val;
	    statsidx += idx;
	    crosssum += val*idx;
	    nrindexes++;
	}

	const float sumvalues = (float)stats.getValue( Stats::Sum );
	const float sumidx = (float)statsidx.getValue( Stats::Sum );
	const float sumsqidx = (float)statsidx.getValue( Stats::SqSum );
	const float aval = ( nrindexes * crosssum - sumvalues * sumidx ) / 
	    		   ( nrindexes * sumsqidx - sumidx * sumidx );
	const float bval = ( sumvalues * sumsqidx - sumidx * crosssum ) /
	    		   ( nrindexes * sumsqidx - sumidx * sumidx );
	trends_ += Trend( aval, bval );
	stats.clear();
	statsidx.clear();
    }
}
    

bool Scaling::computeData( const DataHolder& output, const BinID& relpos,
			   int z0, int nrsamples, int threadid ) const
{
    if ( scalingtype_ == mScalingTypeSqueeze )
    {
	scaleSqueeze( output, z0, nrsamples );
	return true;
    }
    else if ( scalingtype_ == mScalingTypeZPower )
    {
	scaleZN( output, z0, nrsamples );
	return true;
    }
    else if ( scalingtype_ == mScalingTypeAGC )
    {
	scaleAGC( output, z0, nrsamples );
	return true;
    }
    else if ( scalingtype_ == mScalingTypeGain )
    {
	scaleGain( output, z0, nrsamples );
	return true;
    }

    TypeSet< Interval<int> > samplegates;
    getSampleGates( gates_, samplegates, z0, nrsamples );

    TypeSet<float> scalefactors;
    if ( statstype_ != mStatsTypeUser && statstype_ != mStatsTypeDetrend )
	getScaleFactorsFromStats( samplegates, scalefactors, z0 );
    else if ( statstype_ == mStatsTypeDetrend )
    {
	const_cast<Scaling*>(this)->getTrendsFromStats( samplegates, z0 );
	for ( int trendidx=0; trendidx<trends_.size(); trendidx++ )
	    scalefactors += 1;
    }
    else
	scalefactors = factors_;

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const int csamp = z0 + idx;
	const float trcval = getInputValue( *inputdata_, dataidx_, idx, z0 );
	float scalefactor = 1;
	bool found = false;
	for ( int sgidx=0; sgidx<samplegates.size(); sgidx++ )
	{
	    if ( !found && samplegates[sgidx].includes(csamp, true ) )
	    {
		if ( statstype_ == mStatsTypeDetrend )
		    scalefactors[sgidx] = trends_[sgidx].valueAtX( csamp );
		if ( sgidx+1 < samplegates.size() && 
		     samplegates[sgidx+1].includes(csamp, true ) )
		{
		    if ( statstype_ == mStatsTypeDetrend )
			scalefactors[sgidx+1] =trends_[sgidx+1].valueAtX(csamp);

		    scalefactor = interpolator( scalefactors[sgidx], 
			    			scalefactors[sgidx+1],
						samplegates[sgidx+1].start,
						samplegates[sgidx].stop,
						csamp );
		}
		else
		    scalefactor = scalefactors[sgidx];

		found = true;
	    }
	}

	const float outval = mIsUdf(trcval)
	    			? trcval
			        : statstype_ == mStatsTypeDetrend
					? trcval - scalefactor
					: trcval * scalefactor;

	setOutputValue( output, 0, idx, z0, outval );
    }

    return true;
}


void Scaling::scaleSqueeze( const DataHolder& output, int z0,
			    int nrsamples) const
{
    DataSqueezer<float> dsq( sqrg_ );
    dsq.setUntouchedRange( squrg_ );
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const float v = getInputValue( *inputdata_, dataidx_, idx, z0 );
       	setOutputValue( output, 0, idx, z0, dsq.value(v) );
    }
}



void Scaling::scaleZN( const DataHolder& output, int z0, int nrsamples) const
{
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const float curt = (idx+z0)*refstep_;
	const float result = pow(curt,powerval_) * 
	    		     getInputValue( *inputdata_, dataidx_, idx, z0 );
       	setOutputValue( output, 0, idx, z0, result );
    }
}


void Scaling::scaleGain( const DataHolder& output, int z0, int nrsamples ) const
{
    int curgateidx = 0;
    TypeSet< Interval<int> > gates;
    for ( int idx=0; idx<gates_.size(); idx++ )
	gates += Interval<int>( (int)gates_[idx].start*1000, 
				(int)gates_[idx].stop*1000 );
    
    while ( true )
    {
	if ( gates[curgateidx].includes(z0, true ) || (curgateidx>=gates.size()-1) )
	    break;
	curgateidx++;
    }

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const float curt = (idx+z0)*refstep_;
	if ( !gates_[curgateidx].includes(curt, true ) && curgateidx<gates_.size()-1 )
	    curgateidx++;

	const float scalefacstart =
	    ( curgateidx==0 || curgateidx >= gates_.size()-1 )
	   		? 1 : factors_[curgateidx-1];
	const float scalefacstop =
	    ( curgateidx==0 || curgateidx >= gates_.size()-1 )
	    		? 1 : factors_[curgateidx];
	Interval<float> curgate = gates_[curgateidx];
	const float val = getInputValue( *inputdata_, dataidx_, idx, z0);
	const float factor = interpolator( scalefacstart, scalefacstop,
					   curgate.start, curgate.stop, curt );
	const float result = val*factor;
       	setOutputValue( output, 0, idx, z0, result );
    }
}


void Scaling::scaleAGC( const DataHolder& output, int z0, int nrsamples ) const
{
    Interval<int> samplewindow;
    samplewindow.start = mNINT32( window_.start/refstep_ );
    samplewindow.stop = mNINT32( window_.stop/refstep_ );

    if ( nrsamples <= samplewindow.width() )
    {
	for ( int idx=0; idx<nrsamples; idx++ )
	{
	    const float result = getInputValue( *inputdata_, dataidx_, idx, z0);
	    setOutputValue( output, 0, idx, z0, result );
	}
	return;
    }
    
    ::AGC<float> agc;
    agc.setMuteFraction( mutefraction_ );
    agc.setSampleGate( samplewindow );
    agc.setInput( *inputdata_->series(dataidx_), inputdata_->nrsamples_ );
    Array1DImpl<float> outarr( inputdata_->nrsamples_ );
    agc.setOutput( outarr );
    agc.execute( false );

    const int shift = output.z0_ - inputdata_->z0_;
    for ( int idx=0; idx<output.nrsamples_; idx++ )
	setOutputValue( output, 0, idx, z0, outarr.get(idx+shift) );
}


void Scaling::getSampleGates( const TypeSet< Interval<float> >& oldtgs,
			      TypeSet<Interval<int> >& newsampgates,
			      int z0, int nrsamples ) const
{
    for( int idx=0; idx<oldtgs.size(); idx++ )
    {
	Interval<int> sg( mNINT32(oldtgs[idx].start/refstep_),
			  mNINT32(oldtgs[idx].stop/refstep_) );
	if ( sg.start>nrsamples+z0 || sg.stop<z0 )
	{
	    newsampgates += Interval<int>(0,0);
	    continue;
	}

	if ( sg.start < inputdata_->z0_ ) sg.start = inputdata_->z0_;
	if ( sg.stop >= inputdata_->z0_ + inputdata_->nrsamples_ ) 
	    sg.stop = inputdata_->z0_ + inputdata_->nrsamples_ -1;
	newsampgates += sg;
    }
}


const Interval<int>* Scaling::desZSampMargin( int inp, int ) const
{
    return scalingtype_ == mScalingTypeZPower ? 0 : &desgate_;
}

} // namespace Attrib
