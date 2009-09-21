/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Sept 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseiswvltattr.cc,v 1.7 2009-09-21 11:23:27 cvsbruno Exp $";


#include "uiaxishandler.h"
#include "uifunctiondisplay.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uiseiswvltattr.h"
#include "uislider.h"

#include "arrayndimpl.h"
#include "arrayndutils.h"
#include "fft.h"
#include "survinfo.h"
#include "wavelet.h"

#include <complex>


uiSeisWvltRotDlg::uiSeisWvltRotDlg( uiParent* p, Wavelet* wvlt )
    : uiDialog(p,uiDialog::Setup("Phase Rotation","",mTODOHelpID))
    , wvlt_(wvlt)
    , orgwvlt_(new Wavelet(*wvlt))
    , sliderfld_(0) 
    , phaserotating(this)
    , wvltattr_(new WaveletAttrib(wvlt)) 			 
{
    sliderfld_ = new uiSliderExtra( this, uiSliderExtra::Setup(
				    "Rotation (degree)")
				    .withedit(true)
				    .sldrsize(250)
				    .isvertical(true),
				    "Phase Rotation slider" );
    StepInterval<float> sintv( -180.0, 180.0, 1 );
    sliderfld_->sldr()->setInterval( sintv );
    sliderfld_->sldr()->valueChanged.notify( 
	    		mCB(this,uiSeisWvltRotDlg,sliderMove) );
    sliderfld_->attach( hCentered );
}     



uiSeisWvltRotDlg::~uiSeisWvltRotDlg()
{
    delete orgwvlt_;
    delete wvltattr_;
}


void uiSeisWvltRotDlg::sliderMove( CallBacker* )
{
    const float slval = sliderfld_->sldr()->getValue();
    rotatePhase( slval );
}


void uiSeisWvltRotDlg::rotatePhase( float dphase )
{
    float* wvltsamps = wvlt_->samples();
    const float* orgwvltsamps = orgwvlt_->samples();
    const int wvltsz = wvlt_->size();
    Array1DImpl<float> hilsamps( wvltsz );
    wvltattr_->getHilbert( hilsamps );

    for ( int idx=0; idx<wvltsz; idx++ )
	wvltsamps[idx] = orgwvltsamps[idx]*cos( dphase*M_PI/180 ) 
		       - hilsamps.get(idx)*sin( dphase*M_PI/180 );
    phaserotating.trigger();
}




#define mPaddFac 3
static const char* attrnms[] = { "Amplitude", "Frequency", "Phase", 0 };
uiWaveletDispPropDlg::uiWaveletDispPropDlg( uiParent* p, const Wavelet* wvlt )
            : uiDialog(p,Setup("Wavelet Properties","","107.4.3").modal(false))
	    , wvltattr_(new WaveletAttrib(wvlt)) 			 
	    , wvltsz_(0)
{
    setCtrlStyle( LeaveOnly );
    BufferString winname( wvlt->name() ); winname += " properties";
    setCaption( winname );

    if ( !wvlt ) return;
    wvltsz_ = wvlt->size();

    for ( int iattr=0; attrnms[iattr]; iattr++ )
	addAttrDisp( iattr == 1 );
    
    memcpy( attrarrays_[0]->getData(), wvlt->samples(), wvltsz_*sizeof(float) );

    setAttrCurves();
}


uiWaveletDispPropDlg::~uiWaveletDispPropDlg()
{
    deepErase( attrarrays_ );
}


void uiWaveletDispPropDlg::addAttrDisp( bool isfreq )
{
    uiFunctionDisplay::Setup fdsu; fdsu.border_.setRight( 0 );
    attrarrays_ += new Array1DImpl<float>( wvltsz_ );
    const int attridx = attrarrays_.size()-1;
    BufferString xname, yname( attrnms[attridx] );
    if ( isfreq )
    {
	xname += attrnms[0];
	fdsu.fillbelow( true );
	attrarrays_[attridx]->setSize( mPaddFac*wvltsz_ );
	BufferString tmp; mSWAP( xname, yname, tmp );
    }
    else
	xname += "Time (ms)";
    attrdisps_ += new uiFunctionDisplay( this, fdsu );
    attrdisps_[attridx]->xAxis()->setName( xname );
    attrdisps_[attridx]->yAxis(false)->setName( yname );
    if ( attridx )
	attrdisps_[attridx]->attach( alignedBelow, attrdisps_[attridx-1] );
}


void uiWaveletDispPropDlg::setAttrCurves()
{
    wvltattr_->getFrequency( *attrarrays_[1], 3 );
    wvltattr_->getPhase( *attrarrays_[2] );

    float zstep = SI().zStep();
    float maxfreq = 0.5/zstep;
    if ( SI().zIsTime() )maxfreq = mNINT( maxfreq );

    for ( int idx=0; idx<attrarrays_.size(); idx++ )
    {
	const Interval<float> intv( 0, idx==1? maxfreq : wvltsz_*zstep*1000 );
	attrdisps_[idx]->setVals( intv, attrarrays_[idx]->arr(), 
				  idx == 1 ? mPaddFac*wvltsz_/2 : wvltsz_ );
    }
}




WaveletAttrib::WaveletAttrib( const Wavelet* wvlt ) 
    : wvltsz_(wvlt->size())  
    , wvltarr_(new Array1DImpl<float>(wvltsz_))
    , fft_(new FFT(false))		      
    , hilbert_(new HilbertTransform())					      
{
    if ( !wvlt ) return;
    memcpy( wvltarr_->getData(), wvlt->samples(), wvltsz_*sizeof(float) );
}


WaveletAttrib::~WaveletAttrib() 
{
    delete wvltarr_;
    delete hilbert_;
    delete fft_;
}


#define mDoTransform(tf,isstraight,inp,outp,sz) \
{   \
	tf->setInputInfo(Array1DInfoImpl(sz));\
	tf->setDir(isstraight);\
	tf->init();\
	tf->transform(inp,outp);\
}
void WaveletAttrib::getHilbert( Array1DImpl<float>& hilb )
{
    hilbert_->setCalcRange( 0, wvltsz_, 0 );
    mDoTransform( hilbert_, true, *wvltarr_, hilb, wvltsz_ );
}


void WaveletAttrib::getPhase( Array1DImpl<float>& phase )
{
    Array1DImpl<float_complex> hilb ( wvltsz_ );
    hilbert_->setCalcRange( 0, wvltsz_, 0 );
    mDoTransform( hilbert_, true, *wvltarr_, hilb, wvltsz_ );
    for ( int idx=0; idx<wvltsz_; idx++ )
    {
	float ph = 0;
	if ( hilb.get(idx).real() )
	    ph = atan2( hilb.get(idx).imag(), hilb.get(idx).real() );
	phase.set( idx, ph );
    }
}


void WaveletAttrib::getFrequency( Array1DImpl<float>& padfreq, bool ispadding )
{
    const int padfac = ispadding ? 3 : 1;
    const int zpadsz = padfac*wvltsz_;

    Array1DImpl<float_complex> czeropaddedarr( zpadsz );
    Array1DImpl<float_complex> cfreqarr( zpadsz );

    for ( int idx=0; idx<zpadsz; idx++ )
	czeropaddedarr.set( idx, 0 );
    for ( int idx=0; idx<wvltsz_; idx++ )
	czeropaddedarr.set( ispadding ? idx+wvltsz_ : idx, wvltarr_->get(idx) );

    mDoTransform( fft_, true, czeropaddedarr, cfreqarr, zpadsz );

    for ( int idx=0; idx<zpadsz; idx++ )
    {
	float fq = abs( cfreqarr.get(idx) );
	padfreq.set( zpadsz-idx-1, fq );
    }
}
