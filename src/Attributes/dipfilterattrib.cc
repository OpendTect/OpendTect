/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUsedVar = "$Id$";


#include "dipfilterattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "math2.h"
#include "survinfo.h"

#include <math.h>


#define mFilterTypeLowPass           0
#define mFilterTypeHighPass          1
#define mFilterTypeBandPass          2

namespace Attrib
{

mAttrDefCreateInstance(DipFilter)
    
void DipFilter::initClass()
{
    mAttrStartInitClassWithDescAndDefaultsUpdate

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


void DipFilter::updateDefaults( Desc& desc )
{
    ValParam* paramvel = desc.getValParam(maxvelStr());
    mDynamicCastGet( FloatParam*, maxvel, paramvel )
    maxvel->setDefaultValue( SI().zIsTime() ? 1000 : 90 );
}


const char* DipFilter::filterTypeNamesStr( int type )
{
    if ( type==mFilterTypeLowPass ) return "LowPass";
    if ( type==mFilterTypeHighPass ) return "HighPass";
    return "BandPass";
}


DipFilter::DipFilter( Desc& ds )
    : Provider( ds )
    , kernel_(0,0,0)
    , minvel_(0)
{
    if ( !isOK() ) return;

    maxvel_ = SI().zIsTime() ? mUdf(float) : 90;
    inputdata_.allowNull(true);
    
    mGetEnum( type_, typeStr() );
    mGetInt( size_, sizeStr() );
    if ( size_%2 == 0 )
	size_++;

    const int hsz = size_/2;

    if ( type_ == mFilterTypeLowPass || type_ == mFilterTypeBandPass )
	mGetFloat( maxvel_, maxvelStr() );

    if ( type_ == mFilterTypeHighPass || type_ == mFilterTypeBandPass )
	mGetFloat( minvel_, minvelStr() );

    mGetBool( filterazi_, filteraziStr() );

    if ( filterazi_ )
    {
	mGetFloat( minazi_, minaziStr() );
	mGetFloat( maxazi_, maxaziStr() );
    
	aziaperture_ = maxazi_-minazi_;
	while ( aziaperture_ > 180 ) aziaperture_ -= 360;
	while ( aziaperture_ < -180 ) aziaperture_ += 360;

	azi_ = minazi_ + aziaperture_/2;
	while ( azi_ > 90 ) azi_ -= 180;
	while ( azi_ < -90 ) azi_ += 180;
    }

    mGetFloat( taperlen_, taperlenStr() );
    taperlen_ = taperlen_/100;

    kernel_.setSize( desc_.is2D() ? 1 : size_, size_, size_ );
    valrange_ = Interval<float>(minvel_,maxvel_);
    valrange_.sort();
    stepout_ = desc_.is2D() ? BinID( 0, hsz ) : BinID( hsz, hsz );
    zmargin_ = Interval<int>( -hsz, hsz );
}


bool DipFilter::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool DipFilter::initKernel()
{
    if ( mIsZero( refstep_, 1e-7 ) || mIsUdf(refstep_) )
    {
 	pErrMsg("No reference step" );
	return false;
    }

    const bool is2d = desc_.is2D();
    const int hsz = size_/2;
    const int hszinl = is2d ? 0 : hsz;

    for ( int kii=-hszinl; kii<=hszinl; kii++ )
    {
	const float ki = kii * inldist();
	const float ki2 = ki*ki;

	for ( int kci=-hsz; kci<=hsz; kci++ )
	{
	    const float kc = kci * crldist();
	    const float kc2 = kc*kc;

	    const float spatialdist = Math::Sqrt(ki2+kc2);

	    for ( int kti=-hsz; kti<=hsz; kti++ )
	    {
		float kt = kti * refstep_;

		static const float rad2deg = 180 / M_PI;

		float velocity = fabs(spatialdist/kt);
		float dipangle = fabs(rad2deg *atan2( kt, spatialdist));
		float val = zIsTime() ? velocity : dipangle;
		float azimuth = rad2deg * atan2( ki, kc );

		float factor = 1;
		if ( kii || kci || kti )
		{
		    if ( type_==mFilterTypeLowPass )
		    {
			if ( val < valrange_.stop )
			{
			    float ratio = val / valrange_.stop;
			    ratio -= (1-taperlen_);
			    ratio /= taperlen_;

			    factor = taper( 1-ratio );
			}
			else 
			{
			    factor = 0;
			}
		    }
		    else if ( type_==mFilterTypeHighPass )
		    {
			if ( val > valrange_.start )
			{
			    float ratio = val / valrange_.start;
			    ratio -= 1;
			    ratio /= taperlen_;
			    factor = taper( ratio );
			}
			else 
			{
			    factor = 0;
			}
		    }
		    else
		    {
			if ( valrange_.includes( val, false ) )
			{
			    float htaperlen = taperlen_/2;
			    float ratio = (val - valrange_.start)
				   	  / valrange_.width();

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

		    if ( ( kii || kci ) && filterazi_
			    		&& !mIsZero(factor,mDefEps) )
		    {
			float htaperlen = taperlen_/2;
			float diff = azimuth - azi_;
			while ( diff > 90 )
			    diff -= 180;
			while ( diff < -90 )
			    diff += 180;

			diff = fabs(diff);
			if ( diff<aziaperture_/2 )
			{
			    float ratio = diff / (aziaperture_/2);

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

		kernel_.set( hszinl+kii, hsz+kci, hsz+kti, factor );
	    }
	}
    }

    return true;
}


float DipFilter::taper( float pos ) const
{
    if ( pos < 0 ) return 0;
    else if ( pos > 1 ) return 1;

    return (float)( ( 1 - cos(pos * M_PI) ) / 2 );
}


bool DipFilter::getInputData( const BinID& relpos, int index )
{
    while ( inputdata_.size()< (1+stepout_.inl*2) * (1+stepout_.crl*2) )
	inputdata_ += 0;
    
    int idx = 0;

    const BinID bidstep = inputs_[0]->getStepoutStep();
    for ( int inl=-stepout_.inl; inl<=stepout_.inl; inl++ )
    {
	for ( int crl=-stepout_.crl; crl<=stepout_.crl; crl++ )
	{
	    const BinID truepos( relpos.inl + inl*abs(bidstep.inl),
				 relpos.crl + crl*abs(bidstep.crl) );
	    const DataHolder* dh = inputs_[0]->getData( truepos, index );
	    if ( !dh ) return false;

	    inputdata_.replace( idx, dh );
	    idx++;
	}
    }

    dataidx_ = getDataIndex( 0 );
    return true;
}


bool DipFilter::computeData( const DataHolder& output, const BinID& relpos,
			     int z0, int nrsamples, int threadid ) const
{
    if ( outputinterest_.isEmpty() ) return false;

    const int hsz = size_/2;
    const int sizeinl = desc_.is2D() ? 1 : size_;
    for ( int idx=0; idx<nrsamples; idx++)
    {
	int dhoff = 0;
	float sum = 0;
	float wsum = 0;
	for ( int idi=0; idi<sizeinl; idi++ )
	{
	    for ( int idc=0; idc<size_; idc++ )
	    {
		const DataHolder* dh = inputdata_[dhoff++];
		if ( !dh ) continue;

		Interval<int> dhinterval( dh->z0_, dh->z0_+dh->nrsamples_ );

		for ( int idt=0, relt = -hsz; idt<size_; idt++, relt++ )
		{
		    const int sample = z0 + idx + relt;
		    if ( dhinterval.includes(sample, false) )
		    {
			const float weight = kernel_.get( idi, idc, idt );

			sum += getInputValue(*dh,dataidx_,idx+relt ,z0)*weight;
			wsum += weight;
		    }
		}
	    }
	}

	setOutputValue( output, 0, idx, z0, wsum ? sum/wsum
						     : mUdf(float) );
    }

    return true;
}


const BinID* DipFilter::desStepout( int inp, int out ) const
{ return &stepout_; }


const Interval<int>* DipFilter::desZSampMargin( int, int ) const
{ return &zmargin_; }

}; // namespace Attrib
