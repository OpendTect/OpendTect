
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        R. K. Singh
 Date:          May 2007
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "tutorialattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "statruncalc.h"


namespace Attrib
{

mAttrDefCreateInstance(Tutorial)    
    
void Tutorial::initClass()
{
    mAttrStartInitClassWithUpdate

    EnumParam* action = new EnumParam( actionStr() );
    action->addEnum( "Scale" );
    action->addEnum( "Square" );
    action->addEnum( "Smooth" );
    desc->addParam( action );
    
    FloatParam* factor = new FloatParam( factorStr() );
    factor->setLimits( Interval<float>(0,mUdf(float)) );
    factor->setDefaultValue( 1 );
    desc->addParam( factor );

    FloatParam* shift = new FloatParam( shiftStr() );
    desc->addParam( shift );
    shift->setDefaultValue( 0 );

    BoolParam* weaksmooth = new BoolParam( weaksmoothStr() );
    desc->addParam( weaksmooth );

    BoolParam* horsmooth = new BoolParam( horsmoothStr() );
    desc->addParam( horsmooth );

    BoolParam* steering = new BoolParam( steeringStr() );
    steering->setDefaultValue( true );
    desc->addParam( steering );

    BinIDParam* stepout = new BinIDParam( stepoutStr() );
    stepout->setDefaultValue( BinID(1,1) );
    desc->addParam( stepout );

    desc->addOutputDataType( Seis::UnknowData );
    desc->addInput( InputSpec("Input data",true) );
    
    InputSpec steeringspec( "Steering data", false );
    steeringspec.issteering_ = true;
    desc->addInput( steeringspec );

    mAttrEndInitClass
}

    
void Tutorial::updateDesc( Desc& desc )
{
    BufferString action = desc.getValParam( actionStr() )->getStringValue();
    bool horsmooth = desc.getValParam( horsmoothStr() )->getBoolValue();

    desc.setParamEnabled( factorStr(), action=="Scale" );
    desc.setParamEnabled( shiftStr(), action=="Scale" );
    desc.setParamEnabled( horsmoothStr(), action=="Smooth" );
    desc.setParamEnabled( stepoutStr(), action=="Smooth" && horsmooth );
    desc.setParamEnabled( weaksmoothStr(), action=="Smooth" && !horsmooth );
    desc.inputSpec(1).enabled_ = action=="Smooth" && horsmooth
			&& desc.getValParam(steeringStr())->getBoolValue();
}


Tutorial::Tutorial( Desc& desc )
    : Provider( desc )
{
    if ( !isOK() ) return;

    mGetEnum( action_, actionStr() );
    mGetFloat( factor_, factorStr() );
    mGetFloat( shift_, shiftStr() );
    mGetBool ( horsmooth_, horsmoothStr() );
    mGetBinID( stepout_, stepoutStr() );
    mGetBool ( weaksmooth_, weaksmoothStr() );

    if ( action_ == 2  && !horsmooth_ )
    {
	const int dev = weaksmooth_ ? 1 : 2;
	sampgate_.start = -dev;
	sampgate_.stop = dev;
    }

    else if ( action_ == 2  && horsmooth_ )
    {
	for ( int idx=-stepout_.inl; idx<=stepout_.inl; idx++ )
	{
	    for ( int cdx=-stepout_.crl; cdx<=stepout_.crl; cdx++ )
	    {
		const BinID bid ( idx, cdx );
		const int steeridx = getSteeringIndex( bid );
		posandsteeridx_.pos_ += bid;
		posandsteeridx_.steeridx_ += steeridx;
	    }
	}
    }

    inpdata_.allowNull( true );
}


bool Tutorial::getInputOutput( int input, TypeSet<int>& res ) const
{
    if ( input == 0 )
    	return Provider::getInputOutput( input, res );

    for ( int idx=0; idx<posandsteeridx_.steeridx_.size(); idx++ )
	    res += posandsteeridx_.steeridx_[idx];

    return true;
}


bool Tutorial::getInputData( const BinID& relpos, int zintv )
{
    if ( inpdata_.isEmpty() )
	inpdata_ += 0;
    const DataHolder* inpdata = inputs_[0]->getData( relpos, zintv );
    if ( !inpdata ) return false;
    inpdata_.replace( 0, inpdata);


    if ( action_ ==2 && horsmooth_ )
    {
	steeringdata_ = inputs_[1] ? inputs_[1]->getData( relpos, zintv ) : 0;
	const int maxlength  = mMAX(stepout_.inl, stepout_.crl)*2 + 1;
	while ( inpdata_.size() < maxlength * maxlength )
	    inpdata_ += 0;
    
	const BinID bidstep = inputs_[0]->getStepoutStep();
	for ( int idx=0; idx<posandsteeridx_.steeridx_.size(); idx++ )
	{
	    if ( posandsteeridx_.steeridx_[idx] == 0 ) continue;
	    const BinID inpos = relpos + bidstep * posandsteeridx_.pos_[idx];
	    const DataHolder* data = inputs_[0]->getData( inpos );
	    if ( !data ) continue;
	    inpdata_.replace( posandsteeridx_.steeridx_[idx], data);
	}
    }

    dataidx_ = getDataIndex( 0 );

    return true;
}


const BinID* Tutorial::desStepout( int inp, int out ) const
{
    if ( inp==0 && action_==2 && horsmooth_ )
	return &stepout_;

    return 0;
}


bool Tutorial::computeData( const DataHolder& output, const BinID& relpos,
			   int z0, int nrsamples, int threadid ) const
{
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	float outval = 0;
	if ( action_==0 || action_==1 )
	{
	    const float trcval = getInputValue( *inpdata_[0], dataidx_,
		    				idx, z0 );
	    outval = action_==0 ? trcval * factor_ + shift_ :
					trcval * trcval;
	}
	else if ( action_==2 && !horsmooth_ )
	{
	    float sum = 0;
	    int count = 0;
	    for ( int isamp=sampgate_.start; isamp<=sampgate_.stop; isamp++ )
	    {
		const float curval = getInputValue( *inpdata_[0], dataidx_,
			                idx + isamp, z0 );
		if ( !mIsUdf(curval) )
		{
		    sum += curval;
		    count ++;
		}
	    }
	    outval = sum / count;
	}
	else if (action_ == 2 && horsmooth_ )
	{
	    float sum = 0;
	    int count = 0;
	    for ( int posidx=0; posidx<inpdata_.size(); posidx++ )
	    {
		if ( !inpdata_[posidx] ) continue;
		const float shift = steeringdata_ ? 
		    	getInputValue( *steeringdata_,posidx, idx, z0 ) : 0;
		const int sampidx = idx + ( mIsUdf(shift) ? 0 : mNINT32(shift) );
		if ( sampidx < 0 || sampidx >= nrsamples ) continue; 
		const float val = getInputValue( *inpdata_[posidx], 
					dataidx_, sampidx, z0 );
		if ( !mIsUdf(val) )
		{
		    sum += val;
		    count ++;
		}
	    }
	    outval = count ? sum/count : mUdf(float);
	}

	setOutputValue( output, 0, idx, z0, outval );
    }

    return true;
}


const Interval<int>* Tutorial::desZSampMargin(int,int) const
{
    if ( action_ == 2 && !horsmooth_ )
	return &sampgate_;

    return 0;
}


} // namespace Attrib

