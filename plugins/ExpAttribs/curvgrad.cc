/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Haibin Di
 Date:          August 2013
________________________________________________________________________
-*/
static const char* rcsID mUsedVar = "$Id$";

#include "curvgrad.h"

#include "angles.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribparamgroup.h"

#include "seisinfo.h"
#include "statruncalc.h"
#include "survinfo.h"



namespace Attrib 
{

mAttrDefCreateInstance( CurvGrad )

void CurvGrad::initClass()
{
    mAttrStartInitClassWithUpdate;

    BinIDParam* stepout = new BinIDParam( stepoutStr() );
    stepout->setDefaultValue( BinID(1, 1) );
    desc->addParam( stepout );

    EnumParam* attribute = new EnumParam( sKey::Output() );
    attribute->addEnum( "Gradient" );
    attribute->addEnum( "Azimuth" );
    desc->addParam( attribute );

    BoolParam* steer = new BoolParam( sKey::Steering() );
    steer->setDefaultValue ( true );
    desc->addParam( steer );
    
    desc->addOutputDataType ( Seis::UnknowData );
    desc->addInput( InputSpec("Curvature Data", true) );
    
    InputSpec steeringspec( "Steering Data", false );
    steeringspec.issteering_ = true;
    desc->addInput( steeringspec );

    mAttrEndInitClass
}


void CurvGrad::updateDesc( Desc& desc )
{
    desc.inputSpec(1).enabled_ = 
	desc.getValParam(sKey::Steering())->getBoolValue();
}


CurvGrad::CurvGrad( Desc& desc )
    : Provider( desc )
{
    if( !isOK()) return;

    mGetEnum( attribute_, sKey::Output() );
    mGetBinID( stepout_, stepoutStr() );
    mGetBool( dosteer_, sKey::Steering() );

    const float maxso = mMAX( stepout_.inl*inlDist(), stepout_.crl*crlDist() );
    const float maxsecdip = maxSecureDip() * 1000;
    const int boudary = mCast( int, floor(maxso*maxsecdip) + 1 );
    sampgate_ = Interval<int>(-boudary, boudary );

    for( int inl=-stepout_.inl; inl<=stepout_.inl; inl++ )
    {
        for( int crl=-stepout_.crl; crl<=stepout_.crl; crl++ )
        {
            const BinID bid( inl, crl );
            const int steeridx = getSteeringIndex( bid );
            posandsteeridx_.pos_ += bid;
	    posandsteeridx_.steeridx_ += steeridx;
	}
    }
    
    inputdata_.allowNull( true );
}


void CurvGrad::initSteering()
{ stdPrepSteering( stepout_ ); }


bool CurvGrad::getInputOutput( int input, TypeSet<int>& res ) const
{
    if( !input )
	return Provider::getInputOutput( input, res );

    for( int idx=0; idx<posandsteeridx_.steeridx_.size(); idx++ )
	res += posandsteeridx_.steeridx_[idx];

    return true;
}


bool CurvGrad::getInputData( const BinID& relpos, int zintv )
{
    if( inputdata_.isEmpty() )
	inputdata_ += 0;
    
    const DataHolder* inpdata = inputs_[0]->getData( relpos, zintv );
    if( !inpdata ) return false;
    inputdata_.replace( 0, inpdata );

    steeringdata_ = inputs_[1] ? inputs_[1]->getData(relpos, zintv) : 0;

    const int maxlength = mMAX(stepout_.inl, stepout_.crl)*2+1;
    while( inputdata_.size()<maxlength*maxlength )
	inputdata_ += 0;

    const BinID bidstep = inputs_[0]->getStepoutStep();
    for( int idx=0; idx<posandsteeridx_.steeridx_.size(); idx++ )
    {
        if( posandsteeridx_.steeridx_[idx]==0 ) 
	    continue;

        const BinID inpos = relpos + bidstep * posandsteeridx_.pos_[idx];
        const DataHolder* data = inputs_[0]->getData( inpos, zintv );
        inputdata_.replace( posandsteeridx_.steeridx_[idx], data );
    }

    dataidx_ = getDataIndex( 0 );
    return true;
}


const BinID* CurvGrad::desStepout( int inp, int out ) const
{ return inp ? 0 : &stepout_; }


bool CurvGrad::computeData( const DataHolder& output,
	const BinID& relpos, int z0, int nrsamples, int threadid ) const
{
    if ( dataidx_<0 ) 
	return false;

    for (int idx=0; idx<nrsamples; idx++ )
    {
	const int inlsize = 2 * stepout_.inl + 1;
	const int crlsize = 2 * stepout_.crl + 1;

	float* inpvolume_ = new float [inlsize*crlsize*sizeof(float)];
	for( int iter=0; iter<inlsize*crlsize; iter++ )
	    *(inpvolume_+iter) = 0;	
	
	for( int posidx=0; posidx<inputdata_.size(); posidx++ )
	{
	    const int posidx_true_ = posandsteeridx_.steeridx_[posidx];
	    if( !inputdata_[posidx_true_] ) continue;
		
	    float shift = 0;
	    shift = steeringdata_ ?	
		getInputValue( *steeringdata_, posidx_true_, idx, z0) : 0;
	    shift = (mIsUdf(shift) ? 0 : shift );			
	    if( shift<sampgate_.start || shift>sampgate_.stop )
		shift = 0;

	    const int sampidx = idx + mNINT32(shift);	            
	    const float val = getInputValue( 
		    *inputdata_[posidx_true_], dataidx_, sampidx, z0 );

	    if( !mIsUdf(val) ) 
		*(inpvolume_+posidx) = val;			
	}

	const float outval_ = calCurvGrad(inpvolume_);
	setOutputValue( output, 0, idx, z0, outval_ );
	delete [] inpvolume_;
    }

    return true;
}


float CurvGrad::calCurvGrad( float *inpvolume_ ) const
{
    const int inlsteo = mMAX(stepout_.inl, stepout_.crl);
    const int crlstep = mMAX(stepout_.inl, stepout_.crl);
    const int crlsize = 2 * crlstep + 1;
    
    int centralpos = inlsteo*crlsize;
    centralpos += crlstep;

    // crossline) direction
    float data_xb = *(inpvolume_+centralpos-crlstep);
    float data_xf = *(inpvolume_+centralpos+crlstep);

    // inline direction
    float data_yb = *(inpvolume_+centralpos-inlsteo*crlsize);
    float data_yf = *(inpvolume_+centralpos+inlsteo*crlsize);

    float crlcg = (float)(0.5 * (data_xf - data_xb) / crlstep);
    float inlcg = (float)(0.5 * (data_yf - data_yb) / inlsteo);
    float cgazim = Angle::rad2deg( atan2(crlcg, inlcg) );

    //adjust the sign
    const int sign = data_xf*data_xb<=0 || data_yf*data_yb<=0 ? -1 : 1;
    float output = 0.0;
    if( attribute_ == 0 )
	output = sign * Math::Sqrt( inlcg*inlcg+crlcg*crlcg );
    else if( attribute_ == 1 )
	output = cgazim;

    return output;
}


const Interval <int>* CurvGrad::desZSampMargin( int inp, int ) const
{ return inp==1 || !dosteer_  ? 0 : &sampgate_; }


}
