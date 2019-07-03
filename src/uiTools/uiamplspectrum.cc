/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:          September 2007
_______________________________________________________________________

-*/

#include "uiamplspectrum.h"

#include "uiaxishandler.h"
#include "uibutton.h"
#include "uifileselector.h"
#include "uifunctiondisplay.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uispinbox.h"

#include "arrayndimpl.h"
#include "arrayndalgo.h"
#include "arrayndwrapper.h"
#include "bufstring.h"
#include "datapackbase.h"
#include "flatposdata.h"
#include "fourier.h"
#include "mouseevent.h"
#include "od_ostream.h"


uiAmplSpectrum::uiAmplSpectrum( uiParent* p, const uiAmplSpectrum::Setup& setup)
    : uiMainWin( p,tr("Amplitude Spectrum"), 0, false, false )
    , timedomain_(nullptr)
    , freqdomain_(nullptr)
    , freqdomainsum_(nullptr)
    , fft_(nullptr)
    , data_(nullptr)
    , specvals_(nullptr)
    , setup_(setup)
{
    if ( !setup_.caption_.isEmpty() )
	setCaption( setup_.caption_ );
    uiFunctionDisplay::Setup su;
    su.fillbelow(true).canvaswidth(600).canvasheight(400);
    su.noy2axis(true).noy2gridline(true);
    disp_ = new uiFunctionDisplay( this, su );
    disp_->xAxis()->setCaption( SI().zIsTime() ? !setup_.iscepstrum_
						    ? tr("Frequency (Hz)")
						    : tr("Quefrency (ms)")
					    : tr("Wavenumber (/m)") );
    disp_->yAxis(false)->setCaption( tr("Power (dB)") );
    disp_->getMouseEventHandler().movement.notify(
				  mCB(this,uiAmplSpectrum,valChgd) );

    dispparamgrp_ = new uiGroup( this, "Display Params Group" );
    dispparamgrp_->attach( alignedBelow, disp_ );
    uiString disptitle = tr("Display between %1").arg(SI().zIsTime() ?
	      uiStrings::sFrequency() : uiStrings::sWaveNumber());
    rangefld_ = new uiGenInput( dispparamgrp_, disptitle, FloatInpIntervalSpec()
			.setName(BufferString("range start"),0)
			.setName(BufferString("range stop"),1) );
    rangefld_->valuechanged.notify( mCB(this,uiAmplSpectrum,dispRangeChgd ) );
    stepfld_ = new uiLabeledSpinBox( dispparamgrp_, tr("Gridline step"));
    stepfld_->box()->setNrDecimals( SI().zIsTime() ? 0 : 5 );
    stepfld_->attach( rightOf, rangefld_ );
    stepfld_->box()->valueChanging.notify(
	    mCB(this,uiAmplSpectrum,dispRangeChgd) );

    uiString lbl =  tr("Value %1, power").arg(SI().zIsTime() ?
		   uiStrings::sFrequency(true) : uiStrings::sWaveNumber(true));
    valfld_ = new uiGenInput(dispparamgrp_, lbl, FloatInpIntervalSpec());
    valfld_->attach( alignedBelow, rangefld_ );
    valfld_->display( false );
    valfld_->setNrDecimals( 2 );

    normfld_ = new uiCheckBox( dispparamgrp_, uiStrings::sNormalize() );
    normfld_->attach( rightOf, valfld_ );
    normfld_->setChecked( true );

    powerdbfld_ = new uiCheckBox( dispparamgrp_, tr("dB scale") );
    powerdbfld_->attach( rightOf, normfld_ );
    powerdbfld_->setChecked( true );

    normfld_->activated.notify( mCB(this,uiAmplSpectrum,putDispData) );
    powerdbfld_->activated.notify( mCB(this,uiAmplSpectrum,putDispData) );

    exportfld_ = uiButton::getStd( this, OD::Export,
				   mCB(this,uiAmplSpectrum,exportCB), false );
    exportfld_->attach( rightAlignedBelow, disp_ );
    exportfld_->attach( ensureBelow, dispparamgrp_ );

    if ( !setup_.iscepstrum_ )
    {
	uiPushButton* cepbut = new uiPushButton( this, tr("Display cepstrum"),
						 false );
	cepbut->activated.notify( mCB(this,uiAmplSpectrum,ceptrumCB) );
	cepbut->attach( leftOf, exportfld_ );
    }
}


uiAmplSpectrum::~uiAmplSpectrum()
{
    delete fft_;
    delete data_;
    delete timedomain_;
    delete freqdomain_;
    delete freqdomainsum_;
    delete specvals_;
}


void uiAmplSpectrum::setDataPackID(
		DataPack::ID dpid, DataPackMgr::ID dmid, int version )
{
    ConstRefMan<DataPack> datapack = DPM(dmid).getDP( dpid );
    if ( datapack )
	setCaption( !datapack ? tr("No data")
			      : tr("Amplitude Spectrum for %1")
				.arg( datapack->name() ) );

    if ( dmid == DataPackMgr::FlatID() )
    {
	mDynamicCastGet(const FlatDataPack*,dp,datapack.ptr());
	if ( dp )
	{
	    setup_.nyqvistspspace_ = (float) (dp->posData().range(false).step);
	    setData( dp->data() );
	}
    }
}


void uiAmplSpectrum::setData( const Array2D<float>& arr2d )
{
    Array3DWrapper<float> arr3d( const_cast<Array2D<float>&>(arr2d) );
    arr3d.setDimMap( 0, 0 );
    arr3d.setDimMap( 1, 2 );
    arr3d.init();
    setData( arr3d );
}


void uiAmplSpectrum::setData( const Array1D<float>& arr1d )
{
    Array3DWrapper<float> arr3d( const_cast<Array1D<float>&>(arr1d) );
    arr3d.setDimMap( 0, 2 );
    arr3d.init();
    setData( arr3d );
}


void uiAmplSpectrum::setData( const float* array, int size )
{
    Array1DImpl<float> arr1d( size );
    OD::memCopy( arr1d.getData(), array, sizeof(float)*size );
    setData( arr1d );
}


void uiAmplSpectrum::setData( const Array3D<float>& array )
{
    Array3DImpl<float>* data = new Array3DImpl<float>( array.info() );
    data->copyFrom( array );
    data_ = data;

    const int sz0 = array.getSize( 0 );
    const int sz1 = array.getSize( 1 );
    const int sz2 = array.getSize( 2 );

    nrtrcs_ = sz0 * sz1;
    initFFT( sz2 );
    if ( !fft_ )
    {
	uiMSG().error( tr("Cannot initialize FFT") );
	return;
    }

    compute( array );
    putDispData(0);
}


void uiAmplSpectrum::initFFT( int nrsamples )
{
    fft_ = Fourier::CC::createDefault();
    if ( !fft_ ) return;

    const int fftsz = fft_->getFastSize( nrsamples );
    fft_->setInputInfo( Array1DInfoImpl(fftsz) );
    fft_->setDir( true );

    timedomain_ = new Array1DImpl<float_complex>( fftsz );
    freqdomain_ = new Array1DImpl<float_complex>( fftsz );
    freqdomainsum_ = new Array1DImpl<float>( fftsz );

    for ( int idx=0; idx<fftsz; idx++)
    {
	timedomain_->set( idx, 0 );
	freqdomainsum_->set( idx,0 );
    }
}


bool uiAmplSpectrum::compute( const Array3D<float>& array )
{
    if ( !timedomain_ || !freqdomain_ ) return false;

    const int fftsz = timedomain_->getSize(0);

    const int sz0 = array.getSize( 0 );
    const int sz1 = array.getSize( 1 );
    const int sz2 = array.getSize( 2 );

    const int start = (fftsz-sz2) / 2;
    for ( int idx=0; idx<sz0; idx++ )
    {
	for ( int idy=0; idy<sz1; idy++ )
	{
	    for ( int idz=0; idz<sz2; idz++ )
	    {
		const float val = array.get( idx, idy, idz );
		timedomain_->set( start+idz, mIsUdf(val) ? 0 : val );
	    }

	    fft_->setInput( timedomain_->getData() );
	    fft_->setOutput( freqdomain_->getData() );
	    fft_->run( true );

	    for ( int idz=0; idz<fftsz; idz++ )
		freqdomainsum_->arr()[idz] += abs(freqdomain_->get(idz));
	}
    }

    const int freqsz = freqdomainsum_->getSize(0) / 2;
    delete specvals_;
    specvals_ = new Array1DImpl<float>( freqsz );
    maxspecval_ = 0.f;
    for ( int idx=0; idx<freqsz; idx++ )
    {
	const float val = freqdomainsum_->get( idx )/nrtrcs_;
	specvals_->set( idx, val );
	if ( val > maxspecval_ )
	    maxspecval_ = val;
    }

    return true;
}


void uiAmplSpectrum::putDispData( CallBacker* )
{
    const bool isnormalized = normfld_->isChecked();
    const bool dbscale = powerdbfld_->isChecked();
    const int fftsz = freqdomainsum_->getSize(0) / 2;
    Array1DImpl<float> dbspecvals( fftsz );
    for ( int idx=0; idx<fftsz; idx++ )
    {
	float val = specvals_->get( idx );
	if ( isnormalized )
	    val /= maxspecval_;

	if ( dbscale )
	    val = 20 * Math::Log10( val > 0 ? val : mDefEpsF );

	dbspecvals.set( idx, val );
    }

    const float maxfreq = fft_->getNyqvist( setup_.nyqvistspspace_ );
    posrange_.set( 0, maxfreq );
    rangefld_->setValue( posrange_ );
    stepfld_->box()->setInterval( posrange_.start, posrange_.stop,
				  (posrange_.stop-posrange_.start)/25 );
    stepfld_->box()->setValue( maxfreq/5 );
    disp_->yAxis(false)->setCaption( dbscale ? tr("Power (dB)")
					     : uiStrings::sAmplitude() );
    disp_->setVals( posrange_, dbspecvals.arr(), fftsz );
}


void uiAmplSpectrum::getSpectrumData( Array1DImpl<float>& data )
{
    if ( !specvals_ ) return;
    const int datasz = specvals_->getSize(0);
    data.setSize( datasz );
    for ( int idx=0; idx<datasz; idx++ )
	data.set( idx, specvals_->get(idx) );
}


void uiAmplSpectrum::dispRangeChgd( CallBacker* )
{
    StepInterval<float> rg = rangefld_->getFInterval();
    rg.step = stepfld_->box()->getFValue();
    if ( mIsZero(rg.step,mDefEpsF) || rg.step<0 )
	rg.step = ( rg.stop - rg.start )/5;

    const float dstart = rg.start - posrange_.start;
    const float dstop = rg.stop - posrange_.stop;
    const bool startok = mIsZero(dstart,1e-5) || dstart > 0;
    const bool stopok = mIsZero(dstop,1e-5) || dstop < 0;
    if ( !startok || !stopok || rg.isRev() )
    {
	rg = posrange_;
	rg.step = ( rg.stop - rg.start )/5;
	rangefld_->setValue( posrange_ );
    }

    disp_->xAxis()->setRange( rg );
    disp_->draw();
}


void uiAmplSpectrum::exportCB( CallBacker* )
{
    uiFileSelector uifs( this );
    if ( !uifs.go() )
	return;

    const BufferString fnm( uifs.fileName() );
    od_ostream strm( fnm );
    if ( strm.isBad() )
	{ uiMSG().error( uiStrings::phrCannotOpenForWrite(fnm) ); return; }

    disp_->dump( strm, false );

    if ( strm.isBad() )
    {
	uiMSG().error( uiStrings::phrCannotWrite(tr("values to: %1")
				 .arg(fnm)) );
	return;
    }

    uiMSG().message( tr("Values written to: %1").arg(fnm) );
}


void uiAmplSpectrum::valChgd( CallBacker* )
{
    if ( !specvals_ ) return;

    const Geom::Point2D<int>& pos = disp_->getMouseEventHandler().event().pos();
    const float xpos = disp_->xAxis()->getVal( pos.x_ );
    const float ypos = disp_->yAxis(false)->getVal( pos.y_ );
    const bool disp = disp_->xAxis()->range().includes(xpos,true) &&
		      disp_->yAxis(false)->range().includes(ypos,true);
    valfld_->display( disp );
    if ( !disp )
	return;

    const float ratio = (xpos-posrange_.start)/posrange_.width();
    const float specsize = sCast( float, specvals_->getSize(0) );
    const int specidx = mNINT32( ratio * specsize );
    const TypeSet<float>& specvals = disp_->yVals();
    if ( !specvals.validIdx(specidx) )
	return;

    const float yval = specvals[specidx];
    valfld_->setValue( xpos, 0 );
    valfld_->setValue( yval, 1 );
}


void uiAmplSpectrum::ceptrumCB( CallBacker* )
{
    if ( !specvals_ ) return;

    uiAmplSpectrum* ampl = new uiAmplSpectrum(
	    this, uiAmplSpectrum::Setup(tr("Amplitude Cepstrum"), true, 1  ) );
    ampl->setDeleteOnClose( true );
    ampl->setData( *specvals_ );
    ampl->show();
}
