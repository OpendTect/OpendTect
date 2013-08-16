
#include "similaritybyaw.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribparamgroup.h"

#include "seisinfo.h"
#include "survinfo.h"

#include "statruncalc.h"

namespace Attrib 
{

mAttrDefCreateInstance( SimilaritybyAW )

void SimilaritybyAW::initClass()
{
    mAttrStartInitClassWithUpdate;

    desc -> addOutputDataType ( Seis::UnknowData );
    desc -> addInput( InputSpec("Input Data", true) );

    FloatGateParam* refTimeGate = new FloatGateParam( refTimeGateStr() );
    refTimeGate -> setDefaultValue( Interval<float>(-28,28) );
    desc -> addParam( refTimeGate );

    FloatGateParam* searchRange = new FloatGateParam( searchRangeStr() );
    searchRange -> setDefaultValue( Interval<float>(-12,12) );
    desc -> addParam( searchRange );

    BinIDParam* stepout = new BinIDParam( stepoutStr() );
    stepout -> setDefaultValue( BinID(1, 1) );
    desc -> addParam( stepout );

    EnumParam* attribute = new EnumParam( sKey::Output() );
    attribute -> addEnum( "Optimal similarity" );
    attribute -> addEnum( "Optimal time gate" );
    desc -> addParam( attribute );

    BoolParam* steer = new BoolParam( sKey::Steering() );
    steer -> setDefaultValue ( true );
    desc -> addParam( steer );

    InputSpec steeringspec( "Steering Data", false );
    steeringspec.issteering_ = true;
    desc -> addInput( steeringspec );
	
    mAttrEndInitClass
}


void SimilaritybyAW::updateDesc( Desc& desc )
{
    desc.inputSpec(1).enabled_ = 
        desc.getValParam(sKey::Steering()) -> getBoolValue();
}


SimilaritybyAW::SimilaritybyAW( Desc& desc )
    :  Provider( desc )
{
    if( !isOK()) return;

    mGetEnum( attribute_, sKey::Output() );
    mGetFloatInterval( reftimegate_, refTimeGateStr() );
    mGetFloatInterval( searchrange_, searchRangeStr() );
    mGetBinID( stepout_, stepoutStr() );
    mGetBool( dosteer_, sKey::Steering() );

    inlstep_ = mMAX(stepout_.inl, stepout_.crl);
    crlstep_ = inlstep_;
    horgate_.inl = inlstep_;
    horgate_.crl = crlstep_;

    const float zinterval = SI().zStep() * 1000;
    verstep0_ = int((reftimegate_.stop - reftimegate_.start)/(2*zinterval));
    verstep1_ = int((searchrange_.stop - searchrange_.start)/(2*zinterval));
    verstep_ = verstep0_ + verstep1_;
    vergate_.start = - verstep_;
    vergate_.stop  =   verstep_;
    float maxso = mMAX(inlstep_*inlDist(), crlstep_*crlDist());
    const float maxsecdip = maxSecureDip() * 1000;
    desstep_ = int(maxso*maxsecdip);
    desgate_ = Interval<int> (-verstep_-desstep_, verstep_+desstep_);

    for( int inl=-horgate_.inl; inl<=horgate_.inl; inl++ )
    {
        for( int crl=-horgate_.crl; crl<=horgate_.crl; crl++ )
        {
            const BinID bid( inl, crl );
            const int steeridx = getSteeringIndex( bid );
	    posandsteeridx_.pos_ += bid;
	    posandsteeridx_.steeridx_ += steeridx;
	}
    }
   
    inputdata_.allowNull( true );
}


void SimilaritybyAW::initSteering()
{ stdPrepSteering( horgate_ ); }


bool SimilaritybyAW::getInputOutput( int input, TypeSet<int>& res ) const
{
    if( !input )
	return Provider::getInputOutput( input, res );

    for( int idx=0; idx<posandsteeridx_.steeridx_.size(); idx++ )
	res += posandsteeridx_.steeridx_[idx];
	
    return true;
}


bool SimilaritybyAW::getInputData( const BinID& relpos, int zintv )
{
    if( inputdata_.isEmpty() )
	inputdata_ += 0;

    const DataHolder* inpdata = inputs_[0] -> getData( relpos, zintv );
    if( !inpdata ) return false;
    inputdata_.replace( 0, inpdata );
   
    const int maxlength = mMAX(horgate_.inl, horgate_.crl)*2+1;
    while( inputdata_.size()<maxlength*maxlength )
        inputdata_ += 0;

    const BinID bidstep = inputs_[0]->getStepoutStep();
    for( int idx=0; idx<posandsteeridx_.steeridx_.size(); idx++ )
    {
        if( posandsteeridx_.steeridx_[idx]==0 ) 
	    continue;

        const BinID inpos = relpos + bidstep * posandsteeridx_.pos_[idx];
        const DataHolder* data = inputs_[0]->getData( inpos );
        inputdata_.replace( posandsteeridx_.steeridx_[idx], data );
    }

    steeringdata_ = inputs_[1] ? inputs_[1]->getData(relpos, zintv) : 0;
    dataidx_ = getDataIndex( 0 );

    return true;
}


const BinID* SimilaritybyAW::desStepout( int inp, int out ) const
{ return inp==0 ? &horgate_ : 0; }


bool SimilaritybyAW::computeData( const DataHolder& output,
        const BinID& relpos, int z0, int nrsamples, int threadid ) const
{
    for (int idx=0; idx<nrsamples; idx++ )
    {
	const int inlnum = 2 * inlstep_ + 1;
	const int crlnum = 2 * crlstep_ + 1;
	const int znum   = 2 * verstep_ + 1;
		
	float* inpvolume = new float [inlnum*crlnum*znum*sizeof(float)];
	for( int iter=0; iter<inlnum*crlnum*znum; iter++ )
	    *(inpvolume+iter) = 0;
		
	for(int posidx=0; posidx<inlnum*crlnum; posidx++ )
	{
	    int posidx_true_ = posandsteeridx_.steeridx_[posidx];
	
	    if( !inputdata_[posidx_true_] ) continue;
			
	    float shift = steeringdata_ ?
		getInputValue( *steeringdata_, posidx_true_, idx, z0 ) : 0;
            int ishift = mIsUdf(shift) ? 0 : mNINT32(shift);
	    if( ishift<-desstep_ || ishift>desstep_ )
	        ishift = 0;
			
	    const int sampidx = idx + ishift - verstep_;			
	    for( int isamp=0; isamp<znum; isamp++ )
	    {
		float val = getInputValue( *inputdata_[posidx_true_],
			dataidx_, sampidx+isamp, z0 );			
		if( mIsUdf(val) ) val = 0.0;
		
		*(inpvolume+posidx*znum+isamp) = val;
	    }
	}
		
	const float outval = calSimilaritybyAW(inpvolume);	
	setOutputValue( output, 0, idx, z0, outval );
	delete [] inpvolume;
    }

    return true;
}


float SimilaritybyAW::calSimilaritybyAW(float *inpvolume) const
{
    const int crlsize = 2 * crlstep_ + 1;
    const int versize = 2 * verstep_ + 1;
    const int centralpos = inlstep_*crlsize*versize + 
			   crlstep_*versize + verstep1_;
    float OptR_ = 0.0;
    int OptW_ = 0;
    float R_Std = 0;
    float R_Max = 0.0;
    int W_Max = 0;
    float R_Min = 1.0;
    int W_Min = 0;
	
    for( int iter=-verstep1_; iter<=verstep1_; iter++ )
    {
	const int pos0_ = centralpos+iter;
	const int pos1_ = 
	    centralpos+iter - inlstep_*crlsize*versize - crlstep_*versize;
	const int pos2_ = centralpos+iter - inlstep_*crlsize*versize;
	const int pos3_ = 
	    centralpos+iter - inlstep_*crlsize*versize + crlstep_*versize;
	const int pos4_ = centralpos+iter + crlstep_*versize;
	const int pos5_ = 
	    centralpos+iter + inlstep_*crlsize*versize + crlstep_*versize;
	const int pos6_ = centralpos+iter + inlstep_*crlsize*versize;
	const int pos7_ = 
	    centralpos+iter + inlstep_*crlsize*versize - crlstep_*versize;
	const int pos8_ = centralpos+iter - crlstep_*versize;
	
	const float R1_ = calSimilarity
		(inpvolume+pos0_, inpvolume+pos1_, 2*(verstep0_-iter)+1);
	const float R2_ = calSimilarity
		(inpvolume+pos0_, inpvolume+pos2_, 2*(verstep0_-iter)+1);
	const float R3_ = calSimilarity
		(inpvolume+pos0_, inpvolume+pos3_, 2*(verstep0_-iter)+1);
	const float R4_ = calSimilarity
		(inpvolume+pos0_, inpvolume+pos4_, 2*(verstep0_-iter)+1);
	const float R5_ = calSimilarity
		(inpvolume+pos0_, inpvolume+pos5_, 2*(verstep0_-iter)+1);
	const float R6_ = calSimilarity
		(inpvolume+pos0_, inpvolume+pos6_, 2*(verstep0_-iter)+1);
	const float R7_ = calSimilarity
		(inpvolume+pos0_, inpvolume+pos7_, 2*(verstep0_-iter)+1);
	const float R8_ = calSimilarity
		(inpvolume+pos0_, inpvolume+pos8_, 2*(verstep0_-iter)+1);
	const float R_ = mCast(float,0.125*(R1_+R2_+R3_+R4_+R5_+R6_+R7_+R8_));
	
	if( iter==0 )
	    R_Std = R_;
	
	if( R_ >= R_Max )
	{
	    R_Max = R_;
	    W_Max = verstep0_ + iter;
	}

	if( R_ <= R_Min )
	{
	    R_Min = R_;
	    W_Min = verstep0_ + iter;
	}
    }

    OptR_ = R_Max;
    OptW_ = W_Max;
    if( R_Std<0.000 )
    {
	OptR_ = R_Min;
	OptW_ = W_Min;
    }
	
    float output = 0.0;
    if( attribute_ == 0 )
	output = 1.0f - OptR_;

    if( attribute_ == 1 )
    {
	const float zinterval = SI().zStep() * 1000;
	output = float(OptW_) * zinterval;
    }

    return output;
}

float SimilaritybyAW::calSimilarity( float *data1, float*data2, 
				     int datalen ) const
{
    float xy = 0.0;
    float xx = 0.0;
    float yy = 0.0;

    for( int iter=0; iter<datalen; iter++ )
    {
	const float a = (*(data1+iter));
	const float b = (*(data2+iter));
	
	xy += (a-b) * (a-b);
	xx += a * a;
	yy += b * b;
    }
    
    float output = Math::Sqrt(xy)/(Math::Sqrt(xx)+Math::Sqrt(yy));
    output = 1.0f - fabs(output);
    return output;
}


const Interval <int>* SimilaritybyAW::desZSampMargin(int inp, int ) const
{ return inp==1 || !dosteer_ ? &vergate_ : &desgate_; }


}
