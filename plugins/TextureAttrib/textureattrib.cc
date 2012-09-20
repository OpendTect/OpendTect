
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        P.F.M. de Groot
 Date:          September 2012
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "textureattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "statruncalc.h"
#include "survinfo.h"
#include <math.h>


namespace Attrib
{

mAttrDefCreateInstance(Texture)    
    
void Texture::initClass()
{
    mAttrStartInitClassWithUpdate

    ZGateParam* gate = new ZGateParam( gateStr() );
    gate->setLimits( Interval<float>(-1000,1000) );
    Interval<float> defgate( -8, 8 );
    gate->setDefaultValue( defgate );
    desc->addParam( gate );
	    
    FloatParam* globalmean = new FloatParam( globalmeanStr() );
    desc->addParam( globalmean );

    FloatParam* globalstdev = new FloatParam( globalstdevStr() );
    globalstdev->setLimits( Interval<float>(0,mUdf(float)) );
    desc->addParam( globalstdev );

    EnumParam* action = new EnumParam( actionStr() );
    action->addEnum( "Contrast" );
    action->addEnum( "Dissimilarity" );
    action->addEnum( "Homogeneity" );
    action->addEnum( "Angular Second Moment" );
    action->addEnum( "Energy" );
    action->addEnum( "Entropy" );
    action->addEnum( "GLCM Mean" );
    action->addEnum( "GLCM Variance" );
    action->addEnum( "GLCM Standard Deviation" );
    action->addEnum( "GLCM Correlation" );
    desc->addParam( action );

    BoolParam* steering = new BoolParam( steeringStr() );
    steering->setDefaultValue( true );
    desc->addParam( steering );

    BinIDParam* stepout = new BinIDParam( stepoutStr() );
    stepout->setDefaultValue( BinID(1,1) );
    stepout->setLimits(Interval<int>(1,50),Interval<int>(1,50)),
    desc->addParam( stepout );

    desc->addOutputDataType( Seis::UnknowData );
    desc->addInput( InputSpec("Input data",true) );
    
    InputSpec steeringspec( "Steering data", false );
    steeringspec.issteering_ = true;
    desc->addInput( steeringspec );

    BoolParam* glcmsize = new BoolParam( glcmsizeStr() );
    desc->addParam( glcmsize );

    BoolParam* scalingtype = new BoolParam( scalingtypeStr() );
    desc->addParam( scalingtype );

    mAttrEndInitClass
}

void Texture::updateDesc( Desc& desc )
{
    bool isauto = desc.getValParam( scalingtypeStr() )->getBoolValue();
	
    desc.setParamEnabled( globalmeanStr(), !isauto );
    desc.setParamEnabled( globalstdevStr(), !isauto );

}

void Texture::updateDefaults( Desc& desc )
{
    ValParam* paramgate = desc.getValParam(gateStr());
    mDynamicCastGet( ZGateParam*, zgate, paramgate )
    float roundedzstep = SI().zStep()*SI().showZ2UserFactor();
    if ( roundedzstep > 0 )
	roundedzstep = (int)( roundedzstep );
    zgate->setDefaultValue( Interval<float>(-roundedzstep*7, roundedzstep*7) );
}

Texture::Texture( Desc& desc )
    : Provider( desc )
	
{
    if ( !isOK() ) return;

    mGetFloatInterval( gate_, gateStr() );
    mGetFloat( globalmean_, globalmeanStr() );
    mGetFloat( globalstdev_, globalstdevStr() );
    gate_.scale( 1./SI().showZ2UserFactor() );
    mGetEnum( action_, actionStr() );
    mGetBinID( stepout_, stepoutStr() );
    mGetBool ( scalingtype_, scalingtypeStr() );
    mGetBool ( matrix_, glcmsizeStr() );
    glcmsize_ = matrix_ ? 16 : 32;

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

    inpdata_.allowNull( true );
}

bool Texture::getInputOutput( int input, TypeSet<int>& res ) const
{
    if ( input == 0 )
    	return Provider::getInputOutput( input, res );

    for ( int idx=0; idx<posandsteeridx_.steeridx_.size(); idx++ )
	res += posandsteeridx_.steeridx_[idx];

    return true;
}

bool Texture::getInputData( const BinID& relpos, int zintv )
{
    if ( inpdata_.isEmpty() )
	inpdata_ += 0;
    const DataHolder* inpdata = inputs_[0]->getData( relpos, zintv );
    if ( !inpdata ) return false;
    inpdata_.replace( 0, inpdata);

    
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

    dataidx_ = getDataIndex( 0 );
    return true;
}

const BinID* Texture::desStepout( int inp, int out ) const
{
    if ( inp==0 )
	return &stepout_;

    return 0;
}

int Texture::scaleVal( float val ) const
{	
    val = val * factor_ + shift_ ;
    if ( val <= 0 ) 
	val = 0 ;
    else if ( val > glcmsize_ - 1 )
	val = glcmsize_ -1 ;
    return (int)val;
}

bool Texture::allowParallelComputation () const 
{
    return !scalingtype_;
}

void Texture::prepareForComputeData()
{
    firsttracescaling_ = 0;
    sampgate_.start = gate_.start/refstep_;
    sampgate_.stop = gate_.stop/refstep_;
    dessampgate_ = Interval<int>( sampgate_.start-2, sampgate_.stop+2 );
    	// TODO: Please update later to protect against steep dip crashes!
}	

bool Texture::computeData( const DataHolder& output, const BinID& relpos,
		int z0, int nrsamples, int threadid ) const
	
{
    float min, max, mean, stdev, sqdif;
    if ( firsttracescaling_== 0 && scalingtype_ )
    {
	min=0;
	max=0;
	mean=0;
	stdev=0;
	sqdif=0;
	int counter=0;
	for ( int idx=0; idx<nrsamples; idx++ ) // Compute scaling and shift factors 
	{
		for ( int posidx=0; posidx<inpdata_.size()-1; posidx++ )
		{
			if ( !inpdata_[posidx] ) continue;
			const float val = getInputValue( *inpdata_[posidx], 
				dataidx_, idx, z0 );
		
			mean += val;
			counter++;
		}
	}
	mean = mean/((float)counter);

	for ( int idx=0; idx<nrsamples; idx++ )  
	{
		for ( int posidx=0; posidx<inpdata_.size()-1; posidx++ )
		{
			if ( !inpdata_[posidx] ) continue;
			const float val = getInputValue( *inpdata_[posidx], 
				dataidx_, idx, z0 );
		
			sqdif += (val-mean)*(val-mean);
		}
	}
	stdev = sqrt(sqdif / ((float) counter));
	const_cast < Texture* > (this)-> firsttracescaling_ = 1;

	min = mean - (3 * stdev);
	max = mean + (3 * stdev);

	if (max==min) 
	{
		const_cast < Texture* > (this)-> factor_ = 1;
		const_cast < Texture* > (this)-> shift_ = 0;
	}
	else
	{
		const_cast < Texture* > (this)-> factor_ = ((float) (glcmsize_-1)) / ((float)(max-min));
		const_cast < Texture* > (this)-> shift_ = -min*factor_;
	}

    }

    else if (firsttracescaling_== 0 && !scalingtype_ )
    {
	    mean = globalmean_;
	    stdev = globalstdev_;
	    min = mean - (3 * stdev);
	    max = mean + (3 * stdev);
	    const_cast < Texture* > (this)-> firsttracescaling_ = 1;

	    if (max==min) 
	    {
		    const_cast < Texture* > (this)-> factor_ = 1;
		    const_cast < Texture* > (this)-> shift_ = 0;
	    }
	    else
	    {
		    const_cast < Texture* > (this)-> factor_ = ((float) (glcmsize_-1)) / ((float)(max-min));
		    const_cast < Texture* > (this)-> shift_ = -min*factor_;
	    }
    }

    //float outval=0;   // Test scaling 
    //for ( int idx=0; idx<nrsamples; idx++ ) 
    //{
    //	const float val = getInputValue( *inpdata_[0], 
    //		dataidx_, idx, z0 );
    //	outval = scaleVal(val);
    //	setOutputValue( output, 0, idx, z0, outval );
    //}

    for ( int idx=0; idx<nrsamples; idx++ ) // Compute unnormalized GLCM Matrix
    {
	    float outval = 0;
	    int refpixpos, neighpixpos, count = 0;
	    float normprob=0, textmeasure=0, glcmmean=0, glcmvar=0;
	    Array2DImpl<int> glcm( glcmsize_, glcmsize_ );
	    glcm.setAll(0);

	    for ( int posidx=0; posidx<inpdata_.size()-1; posidx++ )
	    {
		    if ( !inpdata_[posidx] || !inpdata_[posidx+1]) continue;
		    const float shift = steeringdata_ ? 
				    getInputValue( *steeringdata_,posidx, idx, z0 ) : 0;
		    const int sampidx = idx + ( mIsUdf(shift) ? 0 : mNINT32(shift) );
		    if ( sampidx < 0 || sampidx >= nrsamples ) continue;
		    for ( int isamp=sampgate_.start; isamp<=sampgate_.stop; isamp++ )
		    {
			    const float val = getInputValue( *inpdata_[posidx], 
				    dataidx_, sampidx+isamp, z0 );
			    const float val1 = getInputValue( *inpdata_[posidx+1], 
				    dataidx_, sampidx+isamp, z0 );
	    
			    refpixpos = scaleVal(val);
			    neighpixpos = scaleVal(val1);	
			    glcm.set( refpixpos, neighpixpos, glcm.get( refpixpos, neighpixpos ) +1 );
			    glcm.set( neighpixpos, refpixpos, glcm.get( neighpixpos, refpixpos ) +1 );
			    count++;
		    }
	    }
	    /* Texture Attribute definitions from http://www.fp.ucalgary.ca/mhallbey/equations.htm */

	    if (action_== 0 ) /* 0=Contrast */
	    {
		    for (int m=0; m<glcmsize_; m++)
		    {
			    for (int n=0; n<glcmsize_; n++)
			    {
				    normprob = glcm.get( n, m )/(float)(2*count);
				    textmeasure += normprob*(n - m)*(n - m); 
			    }
		    }
	    }

	    if (action_ == 1 ) /* 1=Dissimilarity */
	    {
		    for (int m=0; m<glcmsize_; m++)
		    {
			    for (int n=0; n<glcmsize_; n++)
			    {
				    normprob = glcm.get( n, m )/(float)(2*count);
				    textmeasure += normprob*abs(n - m); 
			    }
		    }
	    }

	    if (action_ == 2 ) /* 2=Homogeneity */
	    {
		    for (int m=0; m<glcmsize_; m++)
		    {
			    for (int n=0; n<glcmsize_; n++)
			    {
				    normprob = glcm.get( n, m )/(float)(2*count);
				    textmeasure += normprob/(1.0+((n - m)*(n - m))); 
			    }
		    }
	    }

	    if (action_ == 3 || action_ == 4) /* 3=ASM (Angular Second Moment); 4=Energy */
	    {
		    for (int m=0; m<glcmsize_; m++)
		    {
			    for (int n=0; n<glcmsize_; n++)
			    {
				    normprob = glcm.get( n, m )/(float)(2*count);
				    textmeasure += normprob*normprob; 
			    }
		    }
		    if (action_ == 4)
		    {
		    textmeasure = sqrt(textmeasure);
		    }
	    }

	    if (action_ == 5 ) /* 5=Entropy */
	    {
		    for (int m=0; m<glcmsize_; m++)
		    {
			    for (int n=0; n<glcmsize_; n++)
			    {
				    normprob = glcm.get( n, m )/(float)(2*count);
				    if (normprob == 0.0) continue;
				    else textmeasure += normprob*(-log(normprob)); 
			    }
		    }
	    }

	    if (action_ == 6 || action_ == 7 || action_ == 8 || action_ == 9) /* 6=GLCM Mean; 7=GLCM Variance; 8=GLCM Standard Deviation; 9=GLCM correlation */
	    {
		    for (int m=0; m<glcmsize_; m++)
		    {
			    for (int n=0; n<glcmsize_; n++)
			    {
				    normprob = glcm.get( n, m )/(float)(2*count);
				    textmeasure += normprob*n; 
			    }
		    }
		    glcmmean = textmeasure;
	    }

	    if (action_ == 7 || action_ == 8 || action_ == 9) 
	    {
		    textmeasure = 0;
		    for (int m=0; m<glcmsize_; m++)
		    {
			    for (int n=0; n<glcmsize_; n++)
			    {
				    normprob = glcm.get( n, m )/(float)(2*count);
				    textmeasure += normprob*(n - glcmmean)*(n - glcmmean); 
			    }
		    }
		    glcmvar = textmeasure;

		    if (action_ == 8)
		    {
		    textmeasure = sqrt(textmeasure);
		    }
	    }

	    if (action_ == 9 ) 
	    {
		    textmeasure = 0;
		    for (int m=0; m<glcmsize_; m++)
		    {
			    for (int n=0; n<glcmsize_; n++)
			    {
				    normprob = glcm.get( n, m )/(float)(2*count);
				    if (glcmvar == 0.0) continue;
				    else textmeasure += normprob*((n-glcmmean)*(m-glcmmean)/glcmvar); 
			    }
		    }
		    if (textmeasure<=0 ) 
		    {
			    textmeasure=0 ;
		    }
		    else if (textmeasure>= 1)
		    {
			    textmeasure=1 ;
		    }
     }

    outval = textmeasure;
    
    setOutputValue( output, 0, idx, z0, outval );
     }

return true;
}


const Interval<int>* Texture::desZSampMargin(int,int) const
{
    return 0;
}


} // namespace Attrib
