/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Sept 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseiswvltattr.cc,v 1.9 2009-10-23 15:30:43 cvsbruno Exp $";


#include "uiseiswvltattr.h"

#include "uiaxishandler.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uiwindowfuncseldlg.h"
#include "uislider.h"

#include "arrayndimpl.h"
#include "arrayndutils.h"
#include "fft.h"
#include "fftfilter.h"
#include "survinfo.h"
#include "wavelet.h"
#include "windowfunction.h"

#include <complex>


uiSeisWvltSliderDlg::uiSeisWvltSliderDlg( uiParent* p, Wavelet* wvlt )
    : uiDialog(p,uiDialog::Setup("","",mTODOHelpID))
    , wvlt_(wvlt)
    , orgwvlt_(new Wavelet(*wvlt))
    , sliderfld_(0) 
    , wvltattr_(new WaveletAttrib(wvlt))
    , acting(this)       				
{
}     


void uiSeisWvltSliderDlg::constructSlider( uiSliderExtra::Setup& su,
				      const Interval<float> sliderrg )
{
    sliderfld_ = new uiSliderExtra( this, uiSliderExtra::Setup(su.lbl_)
				    .withedit(true)
				    .sldrsize(su.sldrsize_)
				    .isvertical(su.isvertical_),
				    "wavelet slider" );
    sliderfld_->sldr()->setInterval( sliderrg );
    sliderfld_->sldr()->valueChanged.notify(mCB(this,uiSeisWvltSliderDlg,act));
}


uiSeisWvltSliderDlg::~uiSeisWvltSliderDlg()
{
    delete orgwvlt_;
    delete wvltattr_;
}


uiSeisWvltRotDlg::uiSeisWvltRotDlg( uiParent* p, Wavelet* wvlt )
    : uiSeisWvltSliderDlg(p,wvlt)
{
    setCaption( "Phase rotation Slider" );
    uiSliderExtra::Setup su; 
    su.lbl_ = "Rotate phase";
    su.isvertical_ = true;
    su.sldrsize_ = 250;
    su.withedit_ = true;

    StepInterval<float> sintv( -180.0, 180.0, 1 );
    constructSlider( su, sintv );
    sliderfld_->attach( hCentered );
}     


void uiSeisWvltRotDlg::act( CallBacker* )
{
    const float dphase = sliderfld_->sldr()->getValue();
    float* wvltsamps = wvlt_->samples();
    const float* orgwvltsamps = orgwvlt_->samples();
    const int wvltsz = wvlt_->size();
    Array1DImpl<float> hilsamps( wvltsz );
    wvltattr_->getHilbert( hilsamps );

    for ( int idx=0; idx<wvltsz; idx++ )
	wvltsamps[idx] = orgwvltsamps[idx]*cos( dphase*M_PI/180 ) 
		       - hilsamps.get(idx)*sin( dphase*M_PI/180 );
    acting.trigger();
}




uiSeisWvltTaperDlg::uiSeisWvltTaperDlg( uiParent* p, Wavelet* wvlt )
    : uiSeisWvltSliderDlg(p,wvlt)
    , window_(0) 
    , drawer_(0)	 
{
    setCaption( "Taper Wavelet" );
    uiSliderExtra::Setup su; 
    su.lbl_ = "Taper Wavelet (%)";
    su.sldrsize_ = 220;
    su.withedit_ = true;
    su.isvertical_ = false;

    StepInterval<float> sintv( 0, 100, 1 );
    constructSlider( su, sintv );
    
    mutefld_ = new uiCheckBox( this, "mute zero frequency" ); 
    mutefld_->setChecked();
    mutefld_->attach( rightOf, sliderfld_ );
    mutefld_->activated.notify( mCB( this, uiSeisWvltTaperDlg, act )  );
    
    const int wvltsz = wvlt->size();

    orgwvltvals_ = new Array1DImpl<float>( wvltsz );
    wvltvals_ = new Array1DImpl<float>( wvltsz );
    memcpy( orgwvltvals_->getData(), wvlt_->samples(), sizeof(float)*wvltsz );
    memcpy( wvltvals_->getData(), wvlt_->samples(), sizeof(float)*wvltsz );
    
    window_ = new ArrayNDWindow( Array1DInfoImpl(wvltsz),false,"CosTaper",0 );

    uiWaveletDispProp::Setup setup; setup.withphase_ = false;
    properties_ = new uiWaveletDispProp( this, wvlt_, setup );
    drawer_ = properties_->getAttrDisp( 0 );
    
    const Color& col = Color::DgbColor();
    drawer_->addColor( col );
    Interval<float> funcrg( -1, 1 );
    drawer_->setFunctionRange( funcrg );

    sliderfld_->attach( ensureBelow, properties_ );
    act(0);
}     


uiSeisWvltTaperDlg::~uiSeisWvltTaperDlg()
{
    delete window_;
    delete wvltvals_;
    delete orgwvltvals_;
    delete properties_;
}


void uiSeisWvltTaperDlg::act( CallBacker* )
{
    float var = sliderfld_->sldr()->getValue();
    if ( !var ) var += 0.01;
    const int wvltsz = wvlt_->size();

    var = 1-var/100;
    const char* winname = "CosTaper";
    window_->setType( winname , var );
    window_->apply( orgwvltvals_, wvltvals_ );
    if ( mutefld_->isChecked() )
	wvltattr_->muteZeroFrequency( *wvltvals_ );

    WindowFunction* winfunc = WinFuncs().create( winname );
    for ( int idx=0; idx<wvltsz; idx++ )
	wvlt_->samples()[idx] = wvltvals_->get(idx); 

    winfunc->setVariable( var );
    drawTaper( winfunc );
    properties_->setAttrCurves( wvlt_ );
    acting.trigger();
}


void uiSeisWvltTaperDlg::drawTaper( WindowFunction* winfunc )
{
    drawer_->erasePoints();
    drawer_->createLine( winfunc );
    TypeSet<int> intset; intset += 0;
    drawer_->draw( intset );
}

/*
void uiSeisWvltFilterDlg::filter( float lowfreq, float highfreq )
{
    const int wvltsz = wvlt_->size();
    FFTFilter filter;
    Array1DImpl<float> orgvals( wvltsz ), vals( wvltsz );
    for ( int idx=0; idx<wvltsz; idx++ )
	orgvals.set( idx, orgwvlt_->samples()[idx] );
    float df = FFT::getDf( wvlt_->sampleRate(), wvltsz );

    filter.FFTBandPassFilter( df, lowfreq, float highfreq, orgvals, vals );
    
    for ( int idx=0; idx<wvltsz; idx++ )
	wvlt_->samples()[idx] = vals.get( idx );

    wvltfiltered.trigger();
}
*/



uiWaveletDispPropDlg::uiWaveletDispPropDlg( uiParent* p, const Wavelet* w )
            : uiDialog(p,Setup("Wavelet Properties","","107.4.3").modal(false))
{
    setCtrlStyle( LeaveOnly );
    BufferString winname( w->name() ); winname += " properties";
    setCaption( winname );
    properties_ = new uiWaveletDispProp( this, w, uiWaveletDispProp::Setup());
}


uiWaveletDispPropDlg::~uiWaveletDispPropDlg()
{
}


#define mPaddFac 3
uiWaveletDispProp::uiWaveletDispProp( uiParent* p, const Wavelet* wvlt, 
				      const Setup& su )
	    : uiGroup(p,"Properties")
	    , wvltsz_(0)
{
    if ( !wvlt ) return;
    wvltsz_ = wvlt->size();

    const char* phase = su.withphase_ ? "Phase" : 0;
    attrnms_[0] = "Amplitude"; attrnms_[1] = "Frequency", 
    attrnms_[2] = phase; attrnms_[3] = 0;
    for ( int iattr=0; attrnms_[iattr]; iattr++ )
	addAttrDisp( iattr == 1 );
    
    setAttrCurves( wvlt );
}


uiWaveletDispProp::~uiWaveletDispProp()
{
    deepErase( attrarrays_ );
}


void uiWaveletDispProp::addAttrDisp( bool isfreq )
{
    float zstep = SI().zStep();
    Interval<float> intv( -wvltsz_*zstep*500 , wvltsz_*zstep*500 );
    uiFunctionDrawer::Setup fdsu; fdsu.xaxrg_ = intv; fdsu.drawownaxis_ = false;
    uiBorder border( 60, 10, 10, 10 ); fdsu.border_ = border;
    attrarrays_ += new Array1DImpl<float>( wvltsz_ );
    const int attridx = attrarrays_.size()-1;
    BufferString xname, yname = attrnms_[attridx];
    if ( isfreq )
    {
	xname += attrnms_[0];
	fdsu.fillbelow( true );
	attrarrays_[attridx]->setSize( mPaddFac*wvltsz_ );
	BufferString tmp; mSWAP( xname, yname, tmp );
    }
    else
	xname += "Time (ms)";
    attrdisps_ += new uiFunctionDrawer( this, fdsu );
    if ( attridx )
	attrdisps_[attridx]->attach( alignedBelow, attrdisps_[attridx-1] );

    attrdisps_[attridx]->xAxis()->setName( xname );
    attrdisps_[attridx]->yAxis(false)->setName( yname );
}


void uiWaveletDispProp::setAttrCurves( const Wavelet* wvlt )
{
    WaveletAttrib wvltattr ( wvlt );
    memcpy( attrarrays_[0]->getData(), wvlt->samples(), wvltsz_*sizeof(float) );
    const int wvltsz = wvlt->size();

    wvltattr.getFrequency( *attrarrays_[1], true );
    if ( attrnms_[2] )
	wvltattr.getPhase( *attrarrays_[2] );

    float zstep = SI().zStep();
    float maxfreq = 0.5/zstep;
    if ( SI().zIsTime() )maxfreq = mNINT( maxfreq );

    for ( int idx=0; idx<attrarrays_.size(); idx++ )
    {
	Interval<float> intv( -wvltsz_*zstep*500 , wvltsz_*zstep*500 );
	if ( idx == 1 )
	    intv.set( 0, maxfreq );
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

    Array1DImpl<float_complex> czeropaddedarr( zpadsz ), cfreqarr( zpadsz );

    for ( int idx=0; idx<zpadsz; idx++ )
	czeropaddedarr.set( idx, 0 );

    for ( int idx=0; idx<wvltsz_; idx++ )
	czeropaddedarr.set( ispadding ? idx+wvltsz_:idx, wvltarr_->get(idx) );

    mDoTransform( fft_, true, czeropaddedarr, cfreqarr, zpadsz );

    for ( int idx=0; idx<zpadsz; idx++ )
    {
	float fq = abs( cfreqarr.get(idx) );
	padfreq.set( zpadsz-idx-1, fq );
    }
}


void WaveletAttrib::muteZeroFrequency( Array1DImpl<float>& vals )
{
    const int arraysz = vals.info().getSize(0);
    Array1DImpl<float_complex> cvals( arraysz ), tmparr( arraysz );

    for ( int idx=0; idx<arraysz; idx++ )
	cvals.set( idx, vals.get(idx) );

    mDoTransform( fft_, true, cvals, tmparr, arraysz );
    tmparr.set( 0, 0 );
    mDoTransform( fft_, false, tmparr, cvals, arraysz );

    for ( int idx=0; idx<arraysz; idx++ )
	vals.set( idx, cvals.get(idx).real() );
}
