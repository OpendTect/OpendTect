/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Sept 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseiswvltattr.cc,v 1.4 2009-09-15 13:36:21 cvsbruno Exp $";


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
{
    hilbert_ = new HilbertTransform();
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
    delete hilbert_;
}


void uiSeisWvltRotDlg::sliderMove( CallBacker* )
{
    const float slval = sliderfld_->sldr()->getValue();
    rotatePhase( slval );
}


void uiSeisWvltRotDlg::rotatePhase( float dphase )
{
    const int wvltsz = wvlt_->size();
    Array1DImpl<float> hilsamps ( wvltsz ), samps ( wvltsz  );
    float* wvltsamps = wvlt_->samples();
    memcpy( samps.getData(), orgwvlt_->samples(), wvltsz*sizeof(float) );

    hilbert_->setInputInfo(Array1DInfoImpl( wvltsz ));
    hilbert_->setCalcRange( 0, wvltsz, 0 );
    hilbert_->setDir( true );
    hilbert_->init();
    hilbert_->transform( samps, hilsamps );

    for ( int idx=0; idx<wvltsz; idx++ )
	wvltsamps[idx] = samps.get(idx)*cos( dphase*M_PI/180 ) 
			- hilsamps.get(idx)*sin( dphase*M_PI/180 );
    phaserotating.trigger();
}




#define mPaddFac 3
static const char* attrnms[] = { "Amplitude", "Frequency", "Phase", 0 };
uiWaveletDispPropDlg::uiWaveletDispPropDlg( uiParent* p, const Wavelet* wvlt )
            : uiDialog( p,Setup("Wavelet Properties","","107.4.3").modal(false))
	    , wvlt_(wvlt)
	    , wvltsz_(0)
	    , fft_(new FFT())
{
    setCtrlStyle( LeaveOnly );

    if ( !wvlt ) return;
    wvltsz_ = wvlt->size();

    attrfld_ = new uiComboBox( this, attrnms, "Compute" );
    attrfld_->setCurrentItem( 0 );
    attrfld_->selectionChanged.notify( mCB(this,uiWaveletDispPropDlg,attrSel) );
    attrfld_->attach( hCentered );

    for ( int iattr=0; attrnms[iattr]; iattr++ )
	addAttrDisp( iattr == 1 );
    
    attrfld_->attach( centeredAbove, attrdisps_[0] );

    setAttrArrays();
    setDispCurves();
}


uiWaveletDispPropDlg::~uiWaveletDispPropDlg()
{
    deepErase( attrarrays_ );
    delete fft_;
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


#define mDoTransform(tf,isstraight,inp,outp,sz) \
{   \
	tf->setInputInfo(Array1DInfoImpl(sz));\
	tf->setDir(isstraight);\
	tf->init();\
	tf->transform(inp,outp);\
}
void uiWaveletDispPropDlg::setAttrArrays()
{
    memcpy(attrarrays_[0]->getData(),wvlt_->samples(),wvltsz_*sizeof(float));

    HilbertTransform* hil = new HilbertTransform();
    Array1DImpl<float_complex> carr( wvltsz_ );
    hil->setCalcRange( 0, wvltsz_, 0 );
    mDoTransform( hil, true, *attrarrays_[0], carr, wvltsz_ );
    delete hil;

    for ( int idx=0; idx<wvltsz_; idx++ )
    {
	float phase = 0;
	if ( carr.get(idx).real() )
	    phase = atan2( carr.get(idx).imag(), carr.get(idx).real() );
	attrarrays_[2]->set( idx, phase );
    }

    const int zpadsz = mPaddFac*wvltsz_;
    Array1DImpl<float_complex> czeropaddedarr( zpadsz );
    Array1DImpl<float_complex> cfreqarr( zpadsz );

    removeBias( &czeropaddedarr );

    for ( int idx=0; idx<zpadsz; idx++ )
	czeropaddedarr.set( idx, 0 );
    for ( int idx=0; idx<wvltsz_; idx++ )
	czeropaddedarr.set( idx+wvltsz_, carr.get(idx) );

    mDoTransform( fft_, true, czeropaddedarr, cfreqarr, zpadsz );
    for ( int idx=0; idx<zpadsz; idx++ )
    {
	float val = abs( cfreqarr.get(idx) );
	attrarrays_[1]->set( zpadsz-idx-1, val );
    }
}


void uiWaveletDispPropDlg::setDispCurves()
{
    float zstep = SI().zStep();
    float maxfreq = fft_->getNyqvist( zstep );
    if ( SI().zIsTime() )
	maxfreq = mNINT( maxfreq );

    for ( int idx=0; idx<attrarrays_.size(); idx++ )
    {
	const Interval<float> intv( 0, idx==1? maxfreq : wvltsz_*zstep*1000 );
	attrdisps_[idx]->setVals( intv, attrarrays_[idx]->arr(), 
				  idx == 1 ? mPaddFac*wvltsz_/2 : wvltsz_ );
    }
}


void uiWaveletDispPropDlg::attrSel( CallBacker* )
{
    if ( !attrfld_ ) return;

    const int attr = attrfld_->currentItem();
    if ( attr >= 0 )
	attrdisps_[attr]->display(false);
}

