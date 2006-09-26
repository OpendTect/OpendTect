/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          July 2006
 RCS:           $Id: gapdeconattrib.cc,v 1.9 2006-09-26 15:43:45 cvshelene Exp $
________________________________________________________________________

-*/

#include "gapdeconattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "survinfo.h"
#include "genericnumer.h"


/*!> Solves a symmetric Toeplitz linear system of equations rf=g 
     ( finds f given r ( top row of Toeplitz matrix ) 
     		 and g ( right-hand-side column vector ),
	a is the array[systdim] of solution to Ra=v
	(Claerbout, FGDP, p. 57) )
*/
static inline
void solveSymToeplitzsystem(int systdim, float* r, float* g, float* f, float* a)
{
    if ( r[0] < 0.001 && r[0] > -0.001 ) return; //check mIsZero(r[0],0.001)

    if ( systdim<1 )
	return;

    a[0] = 1;
    float v = r[0];
    f[0] = g[0]/r[0];

    for ( int j=1; j<systdim; j++ )
    {
	a[j] = 0;
	f[j] = 0;
	float tmpvar = 0;		// corresponds to e in Clearbout, FGDP
	for ( int i=0; i<j; i++ )
	    tmpvar += a[i]*r[j-i];
	
	float coef = tmpvar/v;		// corresponds to c in Clearbout, FGDP
	v -= coef*tmpvar;
	if ( v < 0.001 && v > -0.001 )
	    v = 0.001;

	for ( int i=0; i<=j/2; i++ )
	{
	    float bot = a[j-i]-coef*a[i];
	    a[i] -= coef*a[j-i];
	    a[j-i] = bot;
	}

	/* use a and v above to get f[i], i = 0,1,2,...,j */
	
	float w; int i;
	for ( i=0,w=0; i<j; i++ )
	    w += f[i]*r[j-i];
	
	coef = (w-g[j])/v;
	for ( i=0; i<=j; i++ )
	    f[i] -= coef*a[j-i];
    }
}


namespace Attrib
{
    
mAttrDefCreateInstance(GapDecon)
    
void GapDecon::initClass()
{
    mAttrStartInitClassWithUpdate
	
    IntParam* lagsize = new IntParam( lagsizeStr() );
    desc->addParam( lagsize );
		
    IntParam* gapsize = new IntParam( gapsizeStr() );
    desc->addParam( gapsize );
    
    IntParam* noiselevel = new IntParam( noiselevelStr() );
    noiselevel->setDefaultValue( 1 );
    desc->addParam( noiselevel );

    IntParam* nrtrcs = new IntParam( nrtrcsStr() );
    nrtrcs->setDefaultValue( 3 );
    desc->addParam( nrtrcs );

    ZGateParam* gate = new ZGateParam( gateStr() );
    gate->setLimits( SI().zRange(true) );
    desc->addParam( gate );
    
    BoolParam* isinputzerophase = new BoolParam( isinp0phaseStr() );
    isinputzerophase->setDefaultValue( true );
    desc->addParam( isinputzerophase );

    BoolParam* isoutputzerophase = new BoolParam( isout0phaseStr() );
    isoutputzerophase->setDefaultValue( true );
    desc->addParam( isoutputzerophase );

    BoolParam* useonlyacorr = new BoolParam( onlyacorrStr() );
    useonlyacorr->setDefaultValue( false );
    useonlyacorr->setRequired( false );
    desc->addParam( useonlyacorr );

    desc->addInput( InputSpec("Input data",true) );
    desc->setNrOutputs( Seis::UnknowData, 5 );

    mAttrEndInitClass
}


void GapDecon::updateDesc( Desc& desc )
{
    bool onlyacorr = desc.getValParam( onlyacorrStr() )->getBoolValue();
    desc.setParamEnabled( lagsizeStr(), !onlyacorr );
    desc.setParamEnabled( gapsizeStr(), !onlyacorr );
    desc.setParamEnabled( nrtrcsStr(), !onlyacorr );
    desc.setParamEnabled( noiselevelStr(), !onlyacorr );
    desc.setParamEnabled( isinp0phaseStr(), !onlyacorr );
    desc.setParamEnabled( isout0phaseStr(), !onlyacorr );
}


GapDecon::GapDecon( Desc& desc_ )
    : Provider( desc_ )
    , inited_( false )
    , ncorr_( 0 )
    , nlag_( 0 )
    , ngap_( 0 )
{
    if ( !isOK() ) return;

    mGetFloatInterval( gate_, gateStr() );
    gate_.scale( 1/zFactor() );
    if ( !SI().zRange(true).includes(gate_.start) )
	gate_.start = SI().zRange(true).start;
    if ( !SI().zRange(true).includes(gate_.stop) )
	gate_.stop = SI().zRange(true).stop;

    mGetBool( useonlyacorr_, onlyacorrStr() );
    if ( !useonlyacorr_ )
    {
	mGetBool( isinpzerophase_, isinp0phaseStr() );
	mGetBool( isoutzerophase_, isout0phaseStr() );
	mGetInt( lagsize_, lagsizeStr() );
	mGetInt( gapsize_, gapsizeStr() );
	int nrtrcs;
	mGetInt( nrtrcs, nrtrcsStr() );
	stepout_ = BinID( nrtrcs/2, nrtrcs/2 );

	mGetInt( noiselevel_, noiselevelStr() );
    }
}


bool GapDecon::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool GapDecon::getInputData( const BinID& relpos, int zintv )
{
    inputdata_ = inputs[0]->getData( relpos, zintv );
    dataidx_ = getDataIndex( 0 );

    return inputdata_;
}


bool GapDecon::computeData( const DataHolder& output, const BinID& relpos, 
			    int z0, int nrsamples ) const
{
    if ( !inputdata_ ) return false;

    if ( !inited_ )
    {
	const_cast<GapDecon*>(this)->ncorr_ = 
				    mNINT( gate_.width() / refstep );
	if ( !useonlyacorr_ )
	{
	    const_cast<GapDecon*>(this)->nlag_  = 
					mNINT( lagsize_ / refstep / zFactor() );
	    const_cast<GapDecon*>(this)->ngap_ = 
					mNINT( gapsize_ / refstep / zFactor() );
	}
	const_cast<GapDecon*>(this)->lcorr_ =  nlag_? nlag_+ngap_ : ncorr_;
	const_cast<GapDecon*>(this)->inited_ = true;
    }

    int safencorr = mMIN( ncorr_, inputdata_->nrsamples_ );	
    int safelcorr = mMIN( lcorr_, inputdata_->nrsamples_ );	
    mVariableLengthArr( float, wiener, ngap_ );
    mVariableLengthArr( float, spiker, ngap_ );
    ArrPtrMan<float> autocorr = new float[safelcorr];

    memset( wiener, 0, ngap_ * sizeof( float ) );
    memset( spiker, 0, ngap_ * sizeof( float ) );
    memset( autocorr, 0, safelcorr * sizeof( float ) );
    
    float* crosscorr = autocorr + nlag_;//first sample of gap is at 
					//maxlag_+1 = nlag_ because minlag = 0

    int startcorr = mNINT( gate_.start / refstep / zFactor() );
    float* inputarr = inputdata_->series(dataidx_)->arr();

    float* autocorrptr = autocorr;
    genericCrossCorrelation<float,float,float*>( safencorr, startcorr, inputarr,
			     safencorr, startcorr, inputarr,
			     safelcorr, 0, autocorrptr );

    if ( mIsZero( autocorr[0], 0.001 ) )
	return false;

    float scale = 1/autocorr[0];
    for ( int idx=0; idx<safelcorr; idx++)  
	autocorr[idx] *= scale;

    if ( !useonlyacorr_ )
    {
	autocorr[0] *= 1 + (float)noiselevel_/100;
	solveSymToeplitzsystem( ngap_, autocorr, crosscorr, wiener, spiker );
    }

    int stopgapidx = startcorr + nlag_+ ngap_;
    int startgapidx = startcorr + nlag_;
    int inoffs = z0 - inputdata_->z0_;
    int outoffs = z0 - output.z0_;
    ValueSeries<float>* inparr = inputdata_->series(dataidx_);

    if ( useonlyacorr_ )
    {
	for ( int idx = 0; idx < safelcorr; ++idx )
	    output.series(0)->setValue( idx+outoffs, autocorr[idx] );
	return true;
    }
    
    for ( int idx = 0; idx < nrsamples; ++idx )
    {
	int n = mMIN( idx, stopgapidx );
	float sum = inparr->value( idx + inoffs );
	for ( int gapidx=startgapidx; gapidx<n; ++gapidx )
	    sum -= wiener[gapidx-startgapidx] *inparr->value(idx-gapidx+inoffs);
	output.series(0)->setValue( idx + outoffs, sum );
    }
/*
    //few lines to test autocorr filtered with gapdecon
    float* inputoutarr = output.series(0)->arr();
    genericCrossCorrelation<float,float,float*>( ncorr_, startcorr, inputoutarr,
						 ncorr_, startcorr, inputoutarr,
						 lcorr_, 0, autocorrptr );

    float scale2 = 1/autocorr[0];
    for ( int idx=0; idx<lcorr_; idx++)
	autocorr[idx] *= scale2;

    Interval<int> gap( startcorr, startcorr + lcorr_ );
    for ( int idx = 0; idx < nrsamples; ++idx )
    {
	float sum = 0;
	if ( gap.includes(idx) )
	    sum += autocorr[ idx - startcorr ];
	output.series(0)->setValue( idx + outoffs, sum );
    }
    //
*/
    return true;
}


const BinID* GapDecon::reqStepout( int inp, int out ) const
{ return 0; }//to fit with fake 2dline -> 3d
//{ return inp ? 0 : &stepout_; }

}; //namespace
