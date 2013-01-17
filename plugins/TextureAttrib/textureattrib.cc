
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

    BinIDParam* stepout = new BinIDParam( stepoutStr() );
    stepout->setDefaultValue( BinID(3,3) );
    stepout->setLimits(Interval<int>(1,50),Interval<int>(1,50)),
    desc->addParam( stepout );

    IntParam* glcmsize = new IntParam( glcmsizeStr() );
    glcmsize->setDefaultValue( 16 );
    desc->addParam( glcmsize );

    FloatParam* globalmin = new FloatParam( globalminStr() );
    desc->addParam( globalmin );

    FloatParam* globalmax = new FloatParam( globalmaxStr() );
    desc->addParam( globalmax );

    BoolParam* steering = new BoolParam( steeringStr() );
    steering->setDefaultValue( true );
    desc->addParam( steering );

    desc->addInput( InputSpec("Input data",true) );
    
    InputSpec steeringspec( "Steering data", false );
    steeringspec.issteering_ = true;
    desc->addInput( steeringspec );

    desc->setNrOutputs( Seis::UnknowData, 10 );

    mAttrEndInitClass
}


void Texture::updateDefaults( Desc& desc )
{
    ValParam* paramgate = desc.getValParam( gateStr() );
    mDynamicCastGet(ZGateParam*,zgate,paramgate)
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
    if ( mIsEqual(globalmin_,globalmax_,1e-3) )
    {
	errmsg_ = "Minimum and Maximum values cannot be the same.\n";
	errmsg_ = "Values represent the clipping range of the input.";
	return;
    }

    mGetFloatInterval( gate_, gateStr() );
    gate_.scale( 1.f/SI().showZ2UserFactor() );
    mGetBinID( stepout_, stepoutStr() );
    mGetInt( glcmsize_, glcmsizeStr() );

    int posidx = 0;
    for ( int idx=-stepout_.inl; idx<=stepout_.inl; idx++ )
    {
	for ( int cdx=-stepout_.crl; cdx<=stepout_.crl; cdx++ )
	{
	    const BinID bid( idx, cdx );
	    posandsteeridx_.pos_ += bid;
	    posandsteeridx_.posidx_ += posidx++;
	    posandsteeridx_.steeridx_ += getSteeringIndex( bid );
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
    const DataHolder* inpdata = inputs_[0]->getData( relpos, zintv );
    if ( !inpdata ) return false;

    while ( inpdata_.size() < posandsteeridx_.posidx_.size() )
	inpdata_ += 0;

    const BinID bidstep = inputs_[0]->getStepoutStep();
    for ( int idx=0; idx<posandsteeridx_.posidx_.size(); idx++ )
    {
	const BinID inpos = relpos + bidstep * posandsteeridx_.pos_[idx];
	const DataHolder* data = inputs_[0]->getData( inpos );
	inpdata_.replace( posandsteeridx_.posidx_[idx], data );
    }

    dataidx_ = getDataIndex( 0 );
    steeringdata_ = inputs_[1] ? inputs_[1]->getData( relpos, zintv ) : 0;
    return true;
}


const BinID* Texture::desStepout( int inp, int out ) const
{ return inp==0 ? &stepout_ : 0; }


const Interval<int>* Texture::desZSampMargin( int inp, int outp ) const
{ return inp==0 ? &dessampgate_ : 0; }


int Texture::scaleVal( float val ) const
{
    val = val * scalingfactor_ + scalingshift_ ;
    if ( val <= 0 ) 
	val = 0 ;
    else if ( val > glcmsize_ - 1 )
	val = mCast( float, glcmsize_-1 );
    return (int)val;
}


bool Texture::allowParallelComputation () const 
{ return true; }


void Texture::fillGLCM( int sampleidx, int z0, int posidx1, int posidx2,
			int& glcmcount,	Array2D<int>& glcm ) const
{
    if ( !inpdata_[posidx1] || !inpdata_[posidx2])
	return;

    float shift = 0;
    if ( steeringdata_ )
    {
	const int steeridx = posandsteeridx_.steeridx_[posidx1];
	shift = getInputValue( *steeringdata_, steeridx, sampleidx, z0 );
    }

    const float shiftedidx = sampleidx + ( mIsUdf(shift) ? 0 : shift );
    int refpixpos, neighpixpos = 0;
    for ( int isamp=sampgate_.start; isamp<=sampgate_.stop; isamp++ )
    {
	const float val = getInterpolInputValue( *inpdata_[posidx1],
	    dataidx_, shiftedidx+isamp, z0 );
	const float val1 = getInterpolInputValue( *inpdata_[posidx2],
	    dataidx_, shiftedidx+isamp, z0 );
	if ( mIsUdf(val) || mIsUdf(val1) )
	    continue;

	refpixpos = scaleVal( val );
	neighpixpos = scaleVal( val1 );
	glcm.set( refpixpos, neighpixpos,
		  glcm.get(refpixpos,neighpixpos) + 1 );
	glcm.set( neighpixpos, refpixpos,
		  glcm.get(neighpixpos,refpixpos) + 1 );
	glcmcount++;
    }
}


// Compute unnormalized GLCM Matrix
int Texture::computeGLCM( int idx, int z0, Array2D<int>& glcm ) const
{
    int glcmcount = 0;
    const int inlsz = stepout_.inl *2 +1;
    const int crlsz = stepout_.crl *2 +1;
    for ( int idi=0; idi<inlsz; idi++ )
    {
	for ( int idc=0; idc<crlsz-1; idc++ )
	{
	    const int posidxcrl1 = idi*crlsz + idc;
	    const int posidxcrl2 = posidxcrl1 + 1;
	    fillGLCM( idx, z0, posidxcrl1, posidxcrl2, glcmcount, glcm );
	}
    }

    for ( int idc=0; idc<crlsz; idc++ )
    {
	for ( int idi=0; idi<inlsz-1; idi++ )
	{
	    const int posidxinl1 = idi*crlsz + idc;
	    const int posidxinl2 = posidxinl1 + crlsz;
	    fillGLCM( idx, z0, posidxinl1, posidxinl2, glcmcount, glcm );
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
	float normprob=0;
	glcm.setAll( 0 );

	const int glcmcount = computeGLCM( idx, z0, glcm );

	float con=0, dis=0, hom=0, asmom=0, ent=0, glcm_mean = 0;
	for ( int m=0; m<glcmsize_; m++ )
	{
	    for ( int n=0; n<glcmsize_; n++ )
	    {
		normprob = glcm.get( n, m )/(float)(2*glcmcount);
		const int n_m = n - m;
		// 0=Contrast
		if ( isOutputEnabled(0) )
		    con += normprob*n_m*n_m;

		// 1=Dissimilarity
		if ( isOutputEnabled(1) )
		    dis += normprob*abs(n_m);

		// 2=Homogeneity
		if ( isOutputEnabled(2) )
		    hom += normprob/(1.0f+(n_m*n_m));

		// 3=ASM (Angular Second Moment); 4=Energy
		if ( isOutputEnabled(3) || isOutputEnabled(4) )
		    asmom += normprob * normprob;

		// 5=Entropy
		if ( isOutputEnabled(5) )
		{
		    const float logprob = Math::Log( normprob );
		    ent += mIsUdf(logprob) ? 0 : -normprob*logprob;
		}

		/* 6=GLCM Mean; 7=GLCM Variance;
		   8=GLCM Standard Deviation; 9=GLCM correlation */
		if ( isOutputEnabled(6) || isOutputEnabled(7) ||
		     isOutputEnabled(8) || isOutputEnabled(9) )
		    glcm_mean += normprob * n;
	    }
	}

	if ( isOutputEnabled(0) )
	    setOutputValue( output, 0, idx, z0, con );
	if ( isOutputEnabled(1) )
	    setOutputValue( output, 1, idx, z0, dis );
	if ( isOutputEnabled(2) )
	    setOutputValue( output, 2, idx, z0, hom );
	if ( isOutputEnabled(3) )
	    setOutputValue( output, 3, idx, z0, asmom );
	if ( isOutputEnabled(4) )
	    setOutputValue( output, 4, idx, z0, Math::Sqrt(asmom) );
	if ( isOutputEnabled(5) )
	    setOutputValue( output, 5, idx, z0, ent );
	if ( isOutputEnabled(6) )
	    setOutputValue( output, 6, idx, z0, glcm_mean );


	if ( isOutputEnabled(7) || isOutputEnabled(8) || isOutputEnabled(9) )
	{
	    float glcm_var = 0;
	    for ( int m=0; m<glcmsize_; m++ )
	    {
		for ( int n=0; n<glcmsize_; n++ )
		{
		    normprob = glcm.get( n, m )/(float)(2*glcmcount);
		    glcm_var += normprob * (n-glcm_mean) * (n-glcm_mean); 
		}
	    }

	    if ( isOutputEnabled(7) )
		setOutputValue( output, 7, idx, z0, glcm_var );

	    if ( isOutputEnabled(8) )
		setOutputValue( output, 8, idx, z0, Math::Sqrt(glcm_var) );

	    if ( isOutputEnabled(9) )
	    {
		float glcm_corr = 0;
		if ( mIsZero(glcm_var,mDefEps) )
		    glcm_corr = 1;
		else
		{
		    for ( int m=0; m<glcmsize_; m++ )
		    {
			for ( int n=0; n<glcmsize_; n++ )
			{
			    normprob = glcm.get( n, m )/(float)(2*glcmcount);
			    glcm_corr += normprob * (n-glcm_mean) *
				(m-glcm_mean) / glcm_var;
			}
		    }
		}

		if ( glcm_corr<0 )	glcm_corr = 0;
		else if ( glcm_corr>1 ) glcm_corr = 1;
		setOutputValue( output, 9, idx, z0, glcm_corr );
	    }

	}
    }

    return true;
}

} // namespace Attrib
