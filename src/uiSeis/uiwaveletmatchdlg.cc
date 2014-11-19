/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uiwaveletmatchdlg.h"

#include "arrayndimpl.h"
#include "genericnumer.h"
#include "wavelet.h"
#include "waveletattrib.h"

#include "uifunctiondisplay.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiseiswvltsel.h"


static const int sDispWidth = 150;
static const int sDispHeight = 150;

uiWaveletMatchDlg::uiWaveletMatchDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Match Wavelets"),mNoDlgTitle,mNoHelpKey))
    , wvltid_(MultiID::udf())
{
    uiFunctionDisplay::Setup fds;
    fds.canvasheight(sDispHeight).canvaswidth(sDispWidth);
    wvlt0disp_ = new uiFunctionDisplay( this, fds );
    wvlt0disp_->setTitle( tr("Reference") );

    wvlt1disp_ = new uiFunctionDisplay( this, fds );
    wvlt1disp_->setTitle( tr("Target") );
    wvlt1disp_->attach( rightTo, wvlt0disp_ );
    wvlt1disp_->attach( heightSameAs, wvlt0disp_ );

    wvltoutdisp_ = new uiFunctionDisplay( this, fds );
    wvltoutdisp_->setTitle( tr("Output") );
    wvltoutdisp_->attach( rightTo, wvlt1disp_ );
    wvltoutdisp_->attach( heightSameAs, wvlt1disp_ );

    wvlt0fld_ = new uiWaveletSel( this, true, "Source Wavelet" );
    wvlt0fld_->attach( alignedBelow, wvlt0disp_ );
    wvlt0fld_->setStretch( 0, 0 );
    wvlt0fld_->selectionDone.notify( mCB(this,uiWaveletMatchDlg,inpSelCB) );
    wvlt1fld_ = new uiWaveletSel( this, true, "Target Wavelet" );
    wvlt1fld_->attach( alignedBelow,  wvlt0fld_ );
    wvlt1fld_->setStretch( 0, 0 );
    wvlt1fld_->selectionDone.notify( mCB(this,uiWaveletMatchDlg,inpSelCB) );

    IntInpSpec filterrg( 21 ); filterrg.setLimits( StepInterval<int>(3,99,2) );
    filterszfld_ = new uiGenInput( this, "Filter length", filterrg );
    filterszfld_->valuechanging.notify( mCB(this,uiWaveletMatchDlg,filterSzCB));
    filterszfld_->attach( rightOf, wvlt1fld_ );

    wvltoutfld_ = new uiWaveletSel( this, false, "Output Wavelet" );
    wvltoutfld_->attach( alignedBelow, wvlt1fld_ );
    wvltoutfld_->setStretch( 0, 0 );
}


void uiWaveletMatchDlg::inpSelCB( CallBacker* cb )
{
    mDynamicCastGet(uiWaveletSel*,inpfld,cb)
    if ( !inpfld ) return;

    PtrMan<Wavelet> wvlt = inpfld->getWavelet();
    if ( !wvlt ) return;

    uiFunctionDisplay* fd = inpfld==wvlt0fld_ ? wvlt0disp_ : wvlt1disp_;
    fd->setVals( wvlt->samplePositions(), wvlt->samples(), wvlt->size() );

    calcFilter();
}


void uiWaveletMatchDlg::filterSzCB( CallBacker* )
{
    calcFilter();
}


/*
static void solveSymToeplitzsystem( int systdim, float* r, float* g,
				    float* f, float* a)
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

	// use a and v above to get f[i], i = 0,1,2,...,j

	float w; int i;
	for ( i=0,w=0; i<j; i++ )
	    w += f[i]*r[j-i];

	coef = (w-g[j])/v;
	for ( i=0; i<=j; i++ )
	    f[i] -= coef*a[j-i];
    }
}
*/


bool uiWaveletMatchDlg::calcFilter()
{
    // Call to genericCrossCorrelation didn't compile: CDash Error

    /*
    PtrMan<Wavelet> refwvlt = wvlt0fld_->getWavelet();
    PtrMan<Wavelet> tarwvlt = wvlt1fld_->getWavelet();
    if ( !refwvlt || !tarwvlt ) return false;

    const int filtersz = filterszfld_->getIntValue();
    mAllocVarLenArr(float,autoref,filtersz);
    mAllocVarLenArr(float,crossref,filtersz);

    const float* wref = refwvlt->samples();
    const int refsz = refwvlt->size();
    genericCrossCorrelation( refsz, 0, wref,
			     refsz, 0, wref,
			     filtersz, 0, autoref );

    const float* wtar = tarwvlt->samples();
    const int tarsz = tarwvlt->size();
    genericCrossCorrelation( refsz, 0, wref,
			     tarsz, 0, wtar,
			     filtersz, 0, crossref );

    mAllocVarLenArr( float, wiener, filtersz );
    mAllocVarLenArr( float, spiker, filtersz );
    solveSymToeplitzsystem( filtersz, autoref, crossref, wiener, spiker );

    Interval<float> intv( -filtersz/2, filtersz/2 );
    wvltoutdisp_->setVals( intv, wiener, filtersz );

    mAllocVarLenArr(float,wqc,filtersz)
    GenericConvolve(refsz,0,wref,filtersz,0,wiener,filtersz,0,wqc);
    wvlt0disp_->setY2Vals( intv, wqc, filtersz );

    */

    return true;
}


bool uiWaveletMatchDlg::acceptOK( CallBacker* )
{
    Wavelet* srcwvlt = wvlt0fld_->getWavelet();
    if ( !srcwvlt ) return false;

    Wavelet* tarwvlt = wvlt1fld_->getWavelet();
    if ( !tarwvlt ) return false;

    const IOObj* outioobj = wvltoutfld_->ioobj();
    if ( !outioobj ) return false;

    const int sz = mMAX(srcwvlt->size(),tarwvlt->size());
    if ( sz != srcwvlt->size() )
	srcwvlt->reSample( mCast(float,sz) );
    else
	tarwvlt->reSample( mCast(float,sz) );

// TODO: Apply window before FFT?
    Array1DImpl<float_complex> fftsrcwvlt( sz );
    WaveletAttrib wvltattr( *srcwvlt );
    wvltattr.transform( fftsrcwvlt, sz );

    Array1DImpl<float_complex> ffttarwvlt( sz );
    wvltattr.setNewWavelet( *tarwvlt );
    wvltattr.transform( ffttarwvlt, sz );

    Array1DImpl<float_complex> fftnewwvlt( sz );
    for ( int idx=0; idx<sz; idx++ )
    {
	float_complex val = ffttarwvlt.get(idx) / fftsrcwvlt.get(idx);
	fftnewwvlt.set( idx, val );
    }

    Array1DImpl<float> newwvlt( sz );
    WaveletAttrib::transformBack( fftnewwvlt, newwvlt );

    Wavelet wvlt;
    wvlt.reSize( sz );
    wvlt.setCenterSample( sz/2 );
    for ( int idx=0; idx<sz; idx++ )
	wvlt.set( idx, newwvlt.get(idx) );

    const bool res = wvlt.put( outioobj );
    if ( !res )
    {
	uiMSG().error( tr("Cannot store new wavelet") );
	return false;
    }

    return true;
}
