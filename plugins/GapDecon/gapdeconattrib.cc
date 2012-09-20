/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          July 2006
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "gapdeconattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "survinfo.h"
#include "genericnumer.h"
#include "convmemvalseries.h"

#define	HALFLENGTH	30


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

//TODO separate the hilbert algo from the attribute itself
static inline float* makeHilbFilt( int hlen )
{
    float* h = new float[hlen*2+1];
    h[hlen] = 0;
    for ( int i=1; i<=hlen; i++ )
    {
	const float taper = (float) (0.54 + 0.46 * 
										cos( M_PI*(float)i / (float)(hlen) ));
	h[hlen+i] = (float) (taper * ( -(float)(i%2)*2.0 / (M_PI*(float)(i)) ));
	h[hlen-i] = -h[hlen+i];
    }

    return h;
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

    IntParam* stepout = new IntParam( stepoutStr() );
    stepout->setDefaultValue( 1 );
    desc->addParam( stepout );

    ZGateParam* gate = new ZGateParam( gateStr() );
    gate->setLimits( SI().zRange(true) );
    desc->addParam( gate );
    
    BoolParam* isinputzerophase = new BoolParam( isinp0phaseStr() );
    isinputzerophase->setDefaultValue( false );
    desc->addParam( isinputzerophase );

    BoolParam* isoutputzerophase = new BoolParam( isout0phaseStr() );
    isoutputzerophase->setDefaultValue( false );
    desc->addParam( isoutputzerophase );

    BoolParam* useonlyacorr = new BoolParam( onlyacorrStr() );
    useonlyacorr->setDefaultValue( false );
    useonlyacorr->setRequired( false );
    desc->addParam( useonlyacorr );

    desc->addInput( InputSpec("Input data",true) );
    desc->addInput( InputSpec("Mixed Input data",false) );
    desc->setNrOutputs( Seis::UnknowData, 5 );

    desc->setLocality( Desc::SingleTrace );
    mAttrEndInitClass
}


void GapDecon::updateDesc( Desc& desc )
{
    bool onlyacorr = desc.getValParam( onlyacorrStr() )->getBoolValue();
    desc.setParamEnabled( lagsizeStr(), !onlyacorr );
    desc.setParamEnabled( gapsizeStr(), !onlyacorr );
    desc.setParamEnabled( stepoutStr(), !onlyacorr );
    desc.setParamEnabled( noiselevelStr(), !onlyacorr );
    desc.setParamEnabled( isinp0phaseStr(), !onlyacorr );
    desc.setParamEnabled( isout0phaseStr(), !onlyacorr );

    if ( !onlyacorr )
    {
	int stepout = desc.getValParam( stepoutStr() )->getIntValue();
	desc.inputSpec(1).enabled_ = stepout>0;
    }
}


GapDecon::GapDecon( Desc& desc )
    : Provider( desc )
    , ncorr_( 0 )
    , nlag_( 0 )
    , ngap_( 0 )
{
    if ( !isOK() ) return;

    mGetFloatInterval( gate_, gateStr() );
    gate_.scale( 1.f/zFactor() );
    if ( !SI().zRange(true).includes(gate_.start,false) )
	gate_.start = SI().zRange(true).start;
    if ( !SI().zRange(true).includes(gate_.stop,false) )
	gate_.stop = SI().zRange(true).stop;

    mGetBool( useonlyacorr_, onlyacorrStr() );
    if ( !useonlyacorr_ )
    {
	mGetBool( isinpzerophase_, isinp0phaseStr() );
	mGetBool( isoutzerophase_, isout0phaseStr() );
	mGetInt( lagsize_, lagsizeStr() );
	mGetInt( gapsize_, gapsizeStr() );
	mGetInt( noiselevel_, noiselevelStr() );
	if ( isoutzerophase_ )
	    hilbfilter_ = makeHilbFilt(HALFLENGTH);
    }

    dessampgate_ = Interval<int>( -(1024-1), 1024-1 );
}


bool GapDecon::getInputData( const BinID& relpos, int zintv )
{
    inputdata_ = inputs_[0]->getData( relpos, zintv );
    dataidx_ = getDataIndex( 0 );

    inputdatamixed_ = inputs_[1] ? inputs_[1]->getData( relpos, zintv ) : 0;
    dataidxmixed_ = inputdatamixed_ ? getDataIndex( 1 ) : -1;

    return inputdata_;
}


void GapDecon::prepareForComputeData()
{
    ncorr_ = mNINT32( gate_.width() / refstep_ );
    if ( !useonlyacorr_ )
    {
	nlag_ = mNINT32( lagsize_ / refstep_ / zFactor() );
	ngap_ = mNINT32( gapsize_ / refstep_ / zFactor() );
    }

    lcorr_ =  nlag_? nlag_+ngap_ : ncorr_;

    Provider::prepareForComputeData();
}


class Masker
{
public:
Masker( const DataHolder* dh, int shift, float avg, int dataidx )
    : data_(dh )
    , avg_(avg)
    , shift_(shift)
    , dataidx_(dataidx) {}

float operator[]( int idx ) const
{
    const int pos = shift_ + idx;
    if ( pos < 0 )
	return data_->series(dataidx_)->value(0) - avg_;
    if ( pos >= data_->nrsamples_ )
	return data_->series(dataidx_)->value(data_->nrsamples_-1) - avg_;
    return data_->series(dataidx_)->value( pos ) - avg_;
}

    const DataHolder*   data_;
    const int           shift_;
    float               avg_;
    int                 dataidx_;
};


bool GapDecon::computeData( const DataHolder& output, const BinID& relpos, 
			    int z0, int nrsamples, int threadid ) const
{
    if ( !inputdata_ ) return false;

    int safencorr = mMIN( ncorr_, inputdata_->nrsamples_ );	
    int safelcorr = mMIN( lcorr_, inputdata_->nrsamples_ );	
    mAllocVarLenArr( float, wiener, ngap_ );
    mAllocVarLenArr( float, spiker, ngap_ );
    ArrPtrMan<float> autocorr = new float[safelcorr];

    memset( wiener, 0, ngap_ * sizeof( float ) );
    memset( spiker, 0, ngap_ * sizeof( float ) );
    memset( autocorr, 0, safelcorr * sizeof( float ) );
    
    float* crosscorr = autocorr + nlag_;//first sample of gap is at 
					//maxlag_+1 = nlag_ because minlag = 0

    int absstartsampidx = mNINT32( gate_.start / refstep_ );
    int startcorr = absstartsampidx - inputdata_->z0_;
    bool usedmixed = inputdatamixed_ && inputdatamixed_->series(dataidxmixed_);
    int safestartcorr = usedmixed ? absstartsampidx-inputdatamixed_->z0_
				  : startcorr;
    ValueSeries<float>* valseries = usedmixed ?
	inputdatamixed_->series(dataidxmixed_) : inputdata_->series(dataidx_);

    if ( !valseries ) return false;

    float* autocorrptr = autocorr;
    genericCrossCorrelation<ValueSeries<float>,ValueSeries<float>,float*>(
			    safencorr, safestartcorr, *valseries,
			    safencorr, safestartcorr, *valseries,
			    safelcorr, 0, autocorrptr );
    if ( mIsZero( autocorr[0], 0.001 ) )
	return false;

    float scale = 1.f/autocorr[0];
    for ( int idx=0; idx<safelcorr; idx++)  
	autocorr[idx] *= scale;

    if ( useonlyacorr_ )
    {
	int maxoutidx = safelcorr < nrsamples ? safelcorr : nrsamples;
	for ( int idx = 0; idx < maxoutidx; ++idx )
	    setOutputValue( output, 0, idx, z0, autocorr[idx] );
	return true;
    }
    else
    {
	autocorr[0] *= 1 + (float)noiselevel_/100;
	solveSymToeplitzsystem( ngap_, autocorr, crosscorr, wiener, spiker );
    }

    int startgapidx = nlag_;
    int stopgapidx =  startgapidx + ngap_;
    int inoffs = z0 - inputdata_->z0_;

    ValueSeries<float>* inparr = inputdata_->series(dataidx_);
    for ( int idx = 0; idx < nrsamples; ++idx )
    {
	int n = mMIN( idx, stopgapidx );
	float sum = inparr->value( idx + inoffs );
	for ( int gapidx=startgapidx; gapidx<n; ++gapidx )
	    sum -= wiener[gapidx-startgapidx] *inparr->value(idx-gapidx+inoffs);
	setOutputValue( output, 0, idx, z0, isoutzerophase_ ? -sum : sum );
    }

    if ( isoutzerophase_ )
    {
	const int shift = z0 - output.z0_;
	DataHolder* tmpdh = output.clone();
	Masker masker( tmpdh, shift, 0, 0 );
	float avg = 0;
	int nrsampleused = nrsamples;
	for ( int idx=0; idx<nrsamples; idx++ )
	{
	    float val = masker[idx];
	    if ( mIsUdf(val) )
	    {
		avg += 0;
		nrsampleused--;
	    }
	    else
		avg += val;
	}

	masker.avg_ = avg / nrsampleused;
	float* outparr = output.series(0)->arr();
	GenericConvolve( HALFLENGTH*2+1, -HALFLENGTH, hilbfilter_, nrsamples,
			 0, masker, nrsamples, 0, outparr );

	delete tmpdh;
    }

/*    
    //few lines to test autocorr filtered with gapdecon
    float* inputoutarr = output.series(0)->arr();
    genericCrossCorrelation<float,float,float*>( ncorr_, startcorr, inputoutarr,
						 ncorr_, startcorr, inputoutarr,
						 lcorr_, 0, autocorrptr );

    float scale2 = 1./autocorr[0];
    for ( int idx=0; idx<lcorr_; idx++)
	autocorr[idx] *= scale2;

    for ( int idx = 0; idx < nrsamples; ++idx )
    {
	float sum = 0;
	if ( idx<lcorr_ )
	    sum += autocorr[ idx ];
	setOutputValue( output, 0, idx, z0, sum );
    }
    //
*/
    return true;
}


}; //namespace
