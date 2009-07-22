/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: dipfilterattrib.cc,v 1.26 2009-07-22 16:01:30 cvsbert Exp $";


#include "dipfilterattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "math2.h"

#include <math.h>


#define mFilterTypeLowPass           0
#define mFilterTypeHighPass          1
#define mFilterTypeBandPass          2

namespace Attrib
{

mAttrDefCreateInstance(DipFilter)
    
void DipFilter::initClass()
{
    mAttrStartInitClassWithUpdate

    IntParam* size = new IntParam( sizeStr() );
    size->setLimits( StepInterval<int>(3,49,2) );
    size->setDefaultValue( 3 );
    desc->addParam( size );

    EnumParam* type = new EnumParam( typeStr() );
    //Note: Ordering must be the same as numbering!
    type->addEnum( filterTypeNamesStr(mFilterTypeLowPass) );
    type->addEnum( filterTypeNamesStr(mFilterTypeHighPass) );
    type->addEnum( filterTypeNamesStr(mFilterTypeBandPass) );
    type->setDefaultValue( mFilterTypeHighPass );
    desc->addParam( type );

    FloatParam* minvel = new FloatParam( minvelStr() );
    minvel->setLimits( Interval<float>(0,mUdf(float)) );
    minvel->setDefaultValue( 0 );
    desc->addParam( minvel );

    FloatParam* maxvel = new FloatParam( maxvelStr() );
    maxvel->setLimits( Interval<float>(0,mUdf(float)) );
    maxvel->setDefaultValue( 1000 );
    desc->addParam( maxvel );

    BoolParam* filterazi = new BoolParam( filteraziStr() );
    filterazi->setDefaultValue( false );
    desc->addParam( filterazi );

    FloatParam* minazi = new FloatParam( minaziStr() );
    minazi->setLimits( Interval<float>(-180,180) );
    minazi->setDefaultValue(0);
    desc->addParam( minazi );
    
    FloatParam* maxazi = new FloatParam( maxaziStr() );
    maxazi->setLimits( Interval<float>(-180,180) );
    maxazi->setDefaultValue( 30 );
    desc->addParam( maxazi );

    FloatParam* taperlen = new FloatParam( taperlenStr() );
    taperlen->setLimits( Interval<float>(0,100) );
    taperlen->setDefaultValue( 20 );
    desc->addParam( taperlen );

    desc->addOutputDataType( Seis::UnknowData );

    desc->addInput( InputSpec("Input data",true) );
    mAttrEndInitClass
}


void DipFilter::updateDesc( Desc& desc )
{
    const bool filterazi = desc.getValParam(filteraziStr())->getBoolValue();
    desc.setParamEnabled( minaziStr(), filterazi );
    desc.setParamEnabled( maxaziStr(), filterazi );

    const ValParam* type = desc.getValParam( typeStr() );
    if ( !strcmp(type->getStringValue(0),
		filterTypeNamesStr(mFilterTypeLowPass)) )
    {
	desc.setParamEnabled( minvelStr(), false );
	desc.setParamEnabled( maxvelStr(), true );
    }
    else if ( !strcmp(type->getStringValue(0),
		filterTypeNamesStr(mFilterTypeHighPass)) )
    {
	desc.setParamEnabled( minvelStr(), true );
	desc.setParamEnabled( maxvelStr(), false );
    }
    else
    {
	desc.setParamEnabled( minvelStr(), true );
	desc.setParamEnabled( maxvelStr(), true );
    }
}


const char* DipFilter::filterTypeNamesStr( int type )
{
    if ( type==mFilterTypeLowPass ) return "LowPass";
    if ( type==mFilterTypeHighPass ) return "HighPass";
    return "BandPass";
}


DipFilter::DipFilter( Desc& ds )
    : Provider( ds )
    , kernel(0,0,0)
{
    if ( !isOK() ) return;

    inputdata.allowNull(true);
    
    mGetEnum( type, typeStr() );
    mGetInt( size, sizeStr() );
    if ( size%2 == 0 )
	size++;

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

    kernel.setSize( desc.is2D() ? 1 : size, size, size );
    valrange = Interval<float>(minvel,maxvel);
    stepout = desc.is2D() ? BinID( 0, size/2 ) : BinID( size/2, size/2 );
    initKernel();
}


bool DipFilter::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool DipFilter::initKernel()
{
    const bool is2d = desc.is2D();
    const int hsz = size/2;
    const int hszinl = is2d ? 0 : hsz;

    float kernelsum = 0;

    for ( int kii=-hszinl; kii<=hszinl; kii++ )
    {
	float ki = kii * inldist();
	float ki2 = ki*ki;

	for ( int kci=-hsz; kci<=hsz; kci++ )
	{
	    float kc = kci * crldist();
	    float kc2 = kc*kc;

	    float spatialdist = Math::Sqrt(ki2+kc2);

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

		kernel.set( hszinl+kii, hsz+kci, hsz+kti, factor );
		kernelsum += factor;
	    }
	}
    }

    for ( int kii=-hszinl; kii<=hszinl; kii++ )
    {
	for ( int kci=-hsz; kci<=hsz; kci++ )
	{
	    for ( int kti=-hsz; kti<=hsz; kti++ )
	    {
		kernel.set( hszinl+kii, hsz+kci, hsz+kti, 
			 kernel.get( hszinl+kii, hsz+kci, hsz+kti )/ kernelsum);
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


bool DipFilter::getInputData( const BinID& relpos, int index )
{
    while ( inputdata.size()< (1+stepout.inl*2) * (1+stepout.crl*2) )
	inputdata += 0;
    
    int idx = 0;

    const BinID bidstep = inputs[0]->getStepoutStep();
    for ( int inl=-stepout.inl; inl<=stepout.inl; inl++ )
    {
	for ( int crl=-stepout.crl; crl<=stepout.crl; crl++ )
	{
	    const BinID truepos( relpos.inl + inl*abs(bidstep.inl),
				 relpos.crl + crl*abs(bidstep.crl) );
	    const DataHolder* dh = inputs[0]->getData( truepos, index );
	    if ( !dh ) return false;

	    inputdata.replace( idx, dh );
	    idx++;
	}
    }

    dataidx_ = getDataIndex( 0 );
    return true;
}


bool DipFilter::computeData( const DataHolder& output, const BinID& relpos,
			     int z0, int nrsamples, int threadid ) const
{
    if ( outputinterest.isEmpty() ) return false;

    const int hsz = size/2;
    const int sizeinl = desc.is2D() ? 1 : size;
    for ( int idx=0; idx<nrsamples; idx++)
    {
	int dhoff = 0;
	int nrvalues = 0;
	float sum = 0;
	for ( int idi=0; idi<sizeinl; idi++ )
	{
	    for ( int idc=0; idc<size; idc++ )
	    {
		const DataHolder* dh = inputdata[dhoff++];
		if ( !dh ) continue;

		Interval<int> dhinterval( dh->z0_, dh->z0_+dh->nrsamples_ );

		int s = idx - hsz;
		for ( int idt=0; idt<size; idt++ )
		{
		    if ( dhinterval.includes(s+z0) && 
			 s+z0-dh->z0_<dh->nrsamples_ )
		    {
			sum += getInputValue( *dh, dataidx_, s, z0 ) *
			       kernel.get( idi, idc, idt );
			nrvalues++;
		    }

		    s++;
		}
	    }
	}

	setOutputValue( output, 0, idx, z0, nrvalues ? sum/nrvalues
						     : mUdf(float) );
    }

    return true;
}


const BinID* DipFilter::reqStepout( int inp, int out ) const
{ return &stepout; }

}; // namespace Attrib
