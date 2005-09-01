/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: dipfilterattrib.cc,v 1.5 2005-09-01 11:51:03 cvsnanne Exp $";


#include "dipfilterattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "datainpspec.h"

#include <math.h>


#define mFilterTypeLowPass           0
#define mFilterTypeHighPass          1
#define mFilterTypeBandPass          2

namespace Attrib
{

DefineEnumNames(DipFilter,KernelSize,0,"KernelSize")
{	   "3",  "5",  "7",  "9",
    "11", "13", "15", "17", "19",
    "21", "23", "25", "27", "29",
    "31", "33", "35", "37", "39",
    "41", "43", "45", "47", "49", 0};


void DipFilter::initClass()
{
    Desc* desc = new Desc( attribName(), updateDesc );
    desc->ref();

    EnumParam* size = new EnumParam(sizeStr());
    size->addEnums(DipFilter::KernelSizeNames);
    size->setDefaultValue("9");
    desc->addParam(size);

    EnumParam* type = new EnumParam(typeStr());
    //Note: Ordering must be the same as numbering!
    type->addEnum(filterTypeNamesStr(mFilterTypeLowPass));
    type->addEnum(filterTypeNamesStr(mFilterTypeHighPass));
    type->addEnum(filterTypeNamesStr(mFilterTypeBandPass));
    type->setDefaultValue("1");
    desc->addParam(type);

    FloatParam* minvel = new FloatParam( minvelStr() );
    minvel->setLimits(Interval<float>(0,mUndefValue));
    minvel->setDefaultValue("0");
    desc->addParam( minvel );

    FloatParam* maxvel = new FloatParam( maxvelStr() );
    maxvel->setLimits(Interval<float>(0,mUndefValue));
    maxvel->setDefaultValue("90");
    desc->addParam( maxvel );

    BoolParam* filterazi = new BoolParam( filteraziStr() );
    filterazi->setDefaultValue(false);
    desc->addParam( filterazi );

    FloatParam* minazi = new FloatParam( minaziStr() );
    minazi->setLimits(Interval<float>(-180,360));
    minazi->setDefaultValue("0");
    desc->addParam( minazi );
    
    FloatParam* maxazi = new FloatParam( maxaziStr() );
    maxazi->setLimits(Interval<float>(-180,360));
    maxazi->setDefaultValue("30");
    desc->addParam( maxazi );

    FloatParam* taperlen = new FloatParam( taperlenStr() );
    taperlen->setLimits(Interval<float>(0,100));
    taperlen->setDefaultValue("20");
    desc->addParam( taperlen );

    desc->addOutputDataType( Seis::UnknowData );

    desc->addInput( InputSpec("Input data",true) );
    desc->init();

    PF().addDesc( desc, createInstance );
    desc->unRef();
}


Provider* DipFilter::createInstance( Desc& desc )
{
    DipFilter* res = new DipFilter( desc );
    res->ref();

    if ( !res->isOK() )
    {
	res->unRef();
	return 0;
    }

    res->unRefNoDelete();
    return res;
}


void DipFilter::updateDesc( Desc& desc )
{
    bool filterazi = desc.getValParam(filteraziStr())->getBoolValue();

    desc.setParamEnabled(minaziStr(),filterazi);
    desc.setParamEnabled(maxaziStr(),filterazi);

    const ValParam* type = desc.getValParam( typeStr() );
    if ( !strcmp(type->getStringValue(0),
		filterTypeNamesStr(mFilterTypeLowPass)) )
    {
	desc.setParamEnabled(minvelStr(),false);
	desc.setParamEnabled(maxvelStr(),true);
    }
    else if ( !strcmp(type->getStringValue(0),
		filterTypeNamesStr(mFilterTypeHighPass)) )
    {
	desc.setParamEnabled(minvelStr(),true);
	desc.setParamEnabled(maxvelStr(),false);
    }
    else
    {
	desc.setParamEnabled(minvelStr(),true);
	desc.setParamEnabled(maxvelStr(),true);
    }
}


const char* DipFilter::filterTypeNamesStr(int type)
{
    if ( type==mFilterTypeLowPass ) return "LowPass";
    if ( type==mFilterTypeHighPass ) return "HighPass";
    return "BandPass";
}


DipFilter::DipFilter( Desc& desc_ )
    : Provider( desc_ )
    , isinited(false)
    , kernel(0,0,0)
{
    if ( !isOK() ) return;

    inputdata.allowNull(true);
    
    mGetEnum( size, sizeStr() );
    mGetEnum( type, typeStr() );

    if ( type == mFilterTypeLowPass || type == mFilterTypeBandPass )
	mGetFloat( maxvel, maxvelStr() );

    if ( type == mFilterTypeHighPass || type == mFilterTypeBandPass )
	mGetFloat( minvel, minvelStr() );

    mGetBool( filterazi, filteraziStr() );

    if ( filterazi )
    {
	mGetFloat( minazi, minaziStr() );
	mGetFloat( maxazi, maxaziStr() );
    
	aziaperture = maxazi-minazi;
	while ( aziaperture > 180 ) aziaperture -= 360;
	while ( aziaperture < -180 ) aziaperture += 360;

	azi = minazi + aziaperture/2;
	while ( azi > 90 ) azi -= 180;
	while ( azi < -90 ) azi += 180;
    }

    mGetFloat( taperlen, taperlenStr() );
    taperlen = taperlen/100;

    kernel.setSize( size*2+3, size*2+3, size*2+3 );
    valrange = Interval<float>(minvel,maxvel);
    stepout = BinID( size+1, size+1 );
    
    size = size*2+3;
}


bool DipFilter::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool DipFilter::init()
{
    const int hsz = size/2;

    float kernelsum = 0;

    for ( int kii=-hsz; kii<=hsz; kii++ )
    {
	float ki = kii * inldist();
	float ki2 = ki*ki;

	for ( int kci=-hsz; kci<=hsz; kci++ )
	{
	    float kc = kci * crldist();
	    float kc2 = kc*kc;

	    float spatialdist = sqrt(ki2+kc2);

	    for ( int kti=-hsz; kti<=hsz; kti++ )
	    {
		float kt = kti * refstep;

		static const float rad2deg = 180 / M_PI;

		float velocity = fabs(spatialdist/kt);
		float dipangle = rad2deg *atan2( kt, spatialdist);
		float val = zIsTime() ? velocity : dipangle;
		float azimuth = rad2deg * atan2( ki, kc );

		float factor = 1;
		if ( kii || kci || kti )
		{
		    if ( type==mFilterTypeLowPass )
		    {
			if ( kti && val < valrange.stop )
			{
			    float ratio = val / valrange.stop;
			    ratio -= (1-taperlen);
			    ratio /= taperlen;

			    factor = taper( 1-ratio );
			}
			else 
			{
			    factor = 0;
			}
		    }
		    else if ( type==mFilterTypeHighPass )
		    {
			if ( kti && val > valrange.start )
			{
			    float ratio = val / valrange.start;
			    ratio -= 1;
			    ratio /= taperlen;
			    factor = taper( ratio );
			}
			else if ( kti )
			    factor = 0;
		    }
		    else
		    {
			if ( kti && valrange.includes( val ) )
			{
			    float htaperlen = taperlen/2;
			    float ratio = (val - valrange.start)
				   	  / valrange.width();

			    if ( ratio > (1-htaperlen))
			    {
				ratio -=(1-htaperlen);
				ratio /= htaperlen;
				factor = taper( ratio );
			    }
			    else if ( ratio < htaperlen )
			    {
				ratio /= htaperlen;
				factor = taper (ratio);
			    }
			}
			else
			{
			    factor = 0;
			}
		    }

		    if ( ( kii || kci ) && filterazi
			    		&& !mIsZero(factor,mDefEps) )
		    {
			float htaperlen = taperlen/2;
			float diff = azimuth - azi;
			while ( diff > 90 )
			    diff -= 180;
			while ( diff < -90 )
			    diff += 180;

			diff = fabs(diff);
			if ( diff<aziaperture/2 )
			{
			    float ratio = diff / (aziaperture/2);

			    if ( ratio > (1-htaperlen))
			    {
				ratio -=(1-htaperlen);
				ratio /= htaperlen;
				factor *= taper( ratio );
			    }
			}
			else
			{
			    factor = 0;
			}
		    }
		}

		kernel.set( hsz+kii, hsz+kci, hsz+kti, factor );
		kernelsum += factor;
	    }
	}
    }

    for ( int kii=-hsz; kii<=hsz; kii++ )
    {
	for ( int kci=-hsz; kci<=hsz; kci++ )
	{
	    for ( int kti=-hsz; kti<=hsz; kti++ )
	    {
		kernel.set( hsz+kii, hsz+kci, hsz+kti, 
			 kernel.get( hsz+kii, hsz+kci, hsz+kti )/ kernelsum);
	    }
	}
    }

    return true;
}


float DipFilter::taper( float pos ) const
{
    if ( pos < 0 ) return 0;
    else if ( pos > 1 ) return 1;

    return (1-cos(pos*M_PI))/2;
}


bool DipFilter::getInputData(const BinID& relpos, int index)
{
    while ( inputdata.size()< (1+stepout.inl*2) * (1+stepout.inl*2) )
	inputdata+=0;
    
    int idx = 0;

    const BinID bidstep = inputs[0]->getStepoutStep();
    for ( int inl=-stepout.inl; inl<=stepout.inl; inl++ )
    {
	for ( int crl=-stepout.crl; crl<=stepout.crl; crl++ )
	{
	    const BinID truepos( relpos.inl + inl*abs(bidstep.inl),
				 relpos.crl + crl*abs(bidstep.crl) );
	    const DataHolder* dh = inputs[0]->getData( truepos, index );
	    if ( !dh )
		return false;

	    inputdata.replace( idx, dh );
	}
    }

    dataidx_ = getDataIndex( 0 );
	
    return true;
}


bool DipFilter::computeData( const DataHolder& output, const BinID& relpos,
			     int t0, int nrsamples ) const
{
    if ( !outputinterest.size() ) return false;

    if ( !isinited )
	const_cast<DipFilter*>(this)->init();

    const int hsz = size/2;

    int nrtraces = size*size;

    for ( int idx=0; idx<nrsamples; idx++)
    {
	const int cursample = t0 + idx;
	int dhoff = 0;
	int nrvalues = 0;

	float sum = 0;
	
	for ( int idi=0; idi<size; idi++ )
	{
	    for ( int idc=0; idc<size; idc++ )
	    {
		const DataHolder* dh = inputdata[dhoff];
		if ( !dh ) continue;

		Interval<float> dhinterval( dh->t0_*refstep,
					    (dh->t0_+dh->nrsamples_) *refstep );

		int s = cursample - hsz;
		for ( int idt=0; idt<size; idt++ )
		{
		    if ( dhinterval.includes( s*refstep )
			    || mIsEqual(dhinterval.start,s*refstep,mDefEps) 
			    || mIsEqual(dhinterval.start,s*refstep,mDefEps) )
		    {
			sum += dh->item(dataidx_)->
			    		value(s)*kernel.get(idi, idc, idt);
			nrvalues++;
		    }
		    
		    s++;
		}
	    }
	}
		    
	if ( outputinterest[0] ) output.item(0)->setValue(idx, sum/nrvalues);
    }

    return true;
}

const BinID* DipFilter::reqStepout( int inp, int out ) const
{ return &stepout; }

};//namespace
