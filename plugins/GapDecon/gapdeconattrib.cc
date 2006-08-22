/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          July 2006
 RCS:           $Id: gapdeconattrib.cc,v 1.5 2006-08-22 14:06:17 cvshelene Exp $
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
    mAttrStartInitClass
	
    IntParam* lagsize = new IntParam( lagsizeStr() );
    desc->addParam( lagsize );
		
    IntParam* gapsize = new IntParam( gapsizeStr() );
    desc->addParam( gapsize );
    
    IntParam* noiselevel = new IntParam( noiselevelStr() );
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

    desc->addInput( InputSpec("Input data",true) );
    desc->setNrOutputs( Seis::UnknowData, 5 );

    mAttrEndInitClass
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

    mGetBool( isinpzerophase_, isinp0phaseStr() );
    mGetBool( isoutzerophase_, isout0phaseStr() );
    mGetInt( lagsize_, lagsizeStr() );
    mGetInt( gapsize_, gapsizeStr() );
    int nrtrcs;
    mGetInt( nrtrcs, nrtrcsStr() );
    stepout_ = BinID( nrtrcs/2, nrtrcs/2 );

    mGetInt( noiselevel_, noiselevelStr() );
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
	const_cast<GapDecon*>(this)->nlag_  = 
				    mNINT( lagsize_ / refstep / zFactor() );
	const_cast<GapDecon*>(this)->ncorr_ = 
				    mNINT( gate_.width() / refstep );
	const_cast<GapDecon*>(this)->ngap_ = 
				    mNINT( gapsize_ / refstep / zFactor() );
	//lcorr = imaxlag[0] + 1;//TODO here only nlag usefull?
	const_cast<GapDecon*>(this)->inited_ = true;
    }

    float* wiener = new float[nlag_];
    float* spiker = new float[nlag_];
    float* autocorr = new float[nlag_];//TODO confirm with lcorr
    memset( wiener, 0, nlag_ * sizeof( float ) );
    memset( spiker, 0, nlag_ * sizeof( float ) );
    memset( autocorr, 0, nlag_ * sizeof( float ) );
    
    float* crosscorr = autocorr;
    //diff code source: in this plugin mincorr = minlag

    //TODO :use the multiple trcs to get a "trc avg" which is then used for the 
    //autocorrelation -> why not using the volume statistic attribute to quickly
    // come to the same result? because here we only have 1 lag and 1 gap,
    // thus no use to interpolate and also no weithing of the trcs.

    int startcorr = mNINT( gate_.start / refstep / zFactor() );
    float* inputarr = inputdata_->series(dataidx_)->arr();

    genericCrossCorrelation( ncorr_, startcorr, inputarr,
			     ncorr_, startcorr, inputarr,
			     nlag_, 0, autocorr );//TODO nlag_--lcorr

    if ( mIsZero( autocorr[0], 0.001 ) )
	return false;

    float scale = 1/autocorr[0];
    for ( int idx=0; idx<nlag_; idx++)  
	autocorr[idx] *= scale;

    autocorr[0] *= 1 + (float)noiselevel_/100;

    //TODO trick, remove as soon as process is clear
    float* autocorrmodif = new float[nlag_+1];//TODO confirm with lcorr
    autocorrmodif[0] = 100;
    for ( int idx=1; idx<nlag_+1; idx++ )
	autocorrmodif[idx] = autocorr[idx-1];
    
    solveSymToeplitzsystem( nlag_, autocorrmodif, crosscorr, wiener, spiker );

    int stoplagidx = startcorr + nlag_;
    int inoffs = z0 - inputdata_->z0_;
    int outoffs = z0 - output.z0_;
    ValueSeries<float>* inparr = inputdata_->series(dataidx_);
    Interval<int> gap( stoplagidx, stoplagidx+ngap_ );
    
    //diff with SU: apply it only within specified gap
    for ( int idx = 0; idx < nrsamples; ++idx )
    {
	int n = mMIN( idx, stoplagidx );
	float sum = inparr->value( idx + inoffs );

	if ( gap.includes(idx) )
	{
	    for ( int lagidx= startcorr; lagidx < n; ++lagidx )//mincorr=minlag
		sum -= wiener[lagidx-startcorr] * 
					    inparr->value( idx-lagidx+inoffs );
	}
	output.series(0)->setValue( idx + outoffs, sum );
    }

    delete [] wiener; delete [] spiker; delete [] autocorr;
    return true;
}


const BinID* GapDecon::reqStepout( int inp, int out ) const
{ return inp ? 0 : &stepout_; }

}; //namespace
