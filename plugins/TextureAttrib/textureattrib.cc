
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
#include "statruncalc.h"


namespace Attrib
{

mAttrDefCreateInstance(Texture)    
    
void Texture::initClass()
{
    mAttrStartInitClass

    ZGateParam* gate = new ZGateParam( gateStr() );
    gate->setLimits( Interval<float>(-1000,1000) );
    Interval<float> defgate( -8, 8 );
    gate->setDefaultValue( defgate );
    desc->addParam( gate );
	    
    FloatParam* globalmin = new FloatParam( globalminStr() );
    desc->addParam( globalmin );

    FloatParam* globalmax = new FloatParam( globalmaxStr() );
    desc->addParam( globalmax );

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
    stepout->setDefaultValue( BinID(3,3) );
    stepout->setLimits(Interval<int>(1,50),Interval<int>(1,50)),
    desc->addParam( stepout );

    desc->addOutputDataType( Seis::UnknowData );
    desc->addInput( InputSpec("Input data",true) );
    
    InputSpec steeringspec( "Steering data", false );
    steeringspec.issteering_ = true;
    desc->addInput( steeringspec );

    BoolParam* glcmsize = new BoolParam( glcmsizeStr() );
    desc->addParam( glcmsize );

    mAttrEndInitClass
}


void Texture::updateDefaults( Desc& desc )
{
    ValParam* paramgate = desc.getValParam(gateStr());
    mDynamicCastGet( ZGateParam*, zgate, paramgate )
    float roundedzstep = SI().zStep()*SI().showZ2UserFactor();
    if ( roundedzstep > 0 )
	roundedzstep = floor ( roundedzstep );
    zgate->setDefaultValue( Interval<float>(-roundedzstep*7, roundedzstep*7) );
}

Texture::Texture( Desc& desc )
    : Provider( desc )
{
    if ( !isOK() ) return;

    mGetFloat( globalmin_, globalminStr() );
    mGetFloat( globalmax_, globalmaxStr() );
    if ( mIsEqual( globalmin_, globalmax_, 1e-3 ) )
    {
	errmsg_ = "Minimum and Maximum values cannot be the same.\n";
	errmsg_ = "Values represent the clipping range of the input.";
	return;
    }

    mGetFloatInterval( gate_, gateStr() );
    gate_.scale( 1.f/SI().showZ2UserFactor() );
    mGetEnum( action_, actionStr() );
    mGetBinID( stepout_, stepoutStr() );
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
    val = val * scalingfactor_ + scalingshift_ ;
    if ( val <= 0 ) 
	val = 0 ;
    else if ( val > glcmsize_ - 1 )
	val = mCast( float, glcmsize_ -1 );
    return (int)val;
}


bool Texture::allowParallelComputation () const 
{
    return true;
}


#define mFillGlmcMatrix( posidx1, posidx2 ) \
    if ( !inpdata_[posidx1] || !inpdata_[posidx2]) continue; \
    const float shift = steeringdata_ ? \
	getInputValue( *steeringdata_,posidx1, idx, z0 ) : 0; \
    const int sampidx = idx + ( mIsUdf(shift) ? 0 : mNINT32(shift) ); \
    if ( sampidx < 0 || sampidx >= nrsamples ) continue; \
    for ( int isamp=sampgate_.start; isamp<=sampgate_.stop; isamp++ ) \
    { \
	const float val = getInputValue( *inpdata_[posidx1], \
	    dataidx_, sampidx+isamp, z0 ); \
	const float val1 = getInputValue( *inpdata_[posidx2], \
	    dataidx_, sampidx+isamp, z0 ); \
	refpixpos = scaleVal(val); \
	neighpixpos = scaleVal(val1); \
	glcm.set( refpixpos, neighpixpos, \
		  glcm.get( refpixpos, neighpixpos ) +1 ); \
	glcm.set( neighpixpos, refpixpos, \
		  glcm.get( neighpixpos, refpixpos ) +1 ); \
	glcmcount++; \
    }

// Compute unnormalized GLCM Matrix
int Texture::computeGlcmMatrix( const BinID& relpos,
		    int idx, int z0, int nrsamples,
		    int threadid, Array2D<int>& glcm ) const
{
    int refpixpos, neighpixpos, glcmcount = 0;

    const int inlsz = stepout_.inl *2 +1;
    const int crlsz = stepout_.crl *2 +1;
	
    for ( int idi=0; idi<inlsz; idi++ )
    {
	for ( int idc=0; idc<crlsz-1; idc++ )
	{
	    int posidxcrl1 = idi*crlsz + idc;
	    int posidxcrl2 = posidxcrl1 + 1;
	    mFillGlmcMatrix( posidxcrl1, posidxcrl2 );
	}
    }

    for ( int idc=0; idc<crlsz; idc++ )
    {
	for ( int idi=0; idi<inlsz-1; idi++ )
	{
	    int posidxinl1 = idi*crlsz + idc;
	    int posidxinl2 = posidxinl1 + crlsz;
	    mFillGlmcMatrix( posidxinl1, posidxinl2 );
	}
    }

    return glcmcount;
 }


void Texture::prepareForComputeData()
{
    scalingfactor_ = ((float) (glcmsize_-1)) /
			((float)( globalmax_- globalmin_ ));
    scalingshift_ = -globalmin_*scalingfactor_;
    sampgate_.start = mNINT32(gate_.start/refstep_);
    sampgate_.stop = mNINT32(gate_.stop/refstep_);
    const float biggestdist = mMAX (SI().inlDistance(), SI().crlDistance() );
    const float safeextrasamp = mCast(float, biggestdist * mMAXDIP / refstep_);
    dessampgate_ = Interval<int>( mNINT32(sampgate_.start-safeextrasamp),
				  mNINT32(sampgate_.stop+safeextrasamp) );
}	


bool Texture::computeData( const DataHolder& output, const BinID& relpos,
	int z0, int nrsamples, int threadid ) const
{
    Array2DImpl<int> glcm( glcmsize_, glcmsize_ );

    for ( int idx=0; idx<nrsamples; idx++ ) 
    {
	float normprob=0, textmeasure=0, glcmmean=0, glcmvar=0;
	glcm.setAll(0);

	const int glcmcount = computeGlcmMatrix( relpos, idx, z0, nrsamples,
						 threadid, glcm ); 	

	/* Texture Attribute definitions
	from http://www.fp.ucalgary.ca/mhallbey/equations.htm */

	if (action_== 0 ) /* 0=Contrast */
	{
	    for (int m=0; m<glcmsize_; m++)
	    {
		for (int n=0; n<glcmsize_; n++)
		{
		    normprob = glcm.get( n, m )/(float)(2*glcmcount);
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
		    normprob = glcm.get( n, m )/(float)(2*glcmcount);
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
		    normprob = glcm.get( n, m )/(float)(2*glcmcount);
		    textmeasure += normprob/(1.0f+((n - m)*(n - m))); 
		}
	    }
	}

	if (action_ == 3 || action_ == 4) /* 3=ASM (Angular Second Moment); 4=Energy */
	{
	    for (int m=0; m<glcmsize_; m++)
	    {
		for (int n=0; n<glcmsize_; n++)
		{
		    normprob = glcm.get( n, m )/(float)(2*glcmcount);
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
		    normprob = glcm.get( n, m )/(float)(2*glcmcount);
		    if (normprob == 0.0) continue;
		    else textmeasure += normprob*(-log(normprob)); 
		}
	    }
	}

	/* 6=GLCM Mean; 7=GLCM Variance; 8=GLCM Standard Deviation; 9=GLCM correlation */
	if (action_ == 6 || action_ == 7 || action_ == 8 || action_ == 9) 
	{
	    for (int m=0; m<glcmsize_; m++)
	    {
		for (int n=0; n<glcmsize_; n++)
		{
		    normprob = glcm.get( n, m )/(float)(2*glcmcount);
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
		    normprob = glcm.get( n, m )/(float)(2*glcmcount);
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
	    if (glcmvar == 0.0) textmeasure = 1;
	    else for (int m=0; m<glcmsize_; m++) 
	    {
		for (int n=0; n<glcmsize_; n++)
		{
		    normprob = glcm.get( n, m )/(float)(2*glcmcount);
		    textmeasure += normprob*((n-glcmmean)*(m-glcmmean)/glcmvar); 
		}
	    }
	    if (textmeasure<=0 ) 
		textmeasure=0 ;
	    else if (textmeasure>= 1)
		textmeasure=1 ;
	}
	setOutputValue( output, 0, idx, z0, textmeasure );
    }

    return true;
}


const Interval<int>* Texture::desZSampMargin(int,int) const
{
    return 0;
}

} // namespace Attrib
