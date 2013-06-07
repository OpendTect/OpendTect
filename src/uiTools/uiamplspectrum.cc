/*+
________________________________________________________________________
            
 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:          September 2007
_______________________________________________________________________
                   
-*/   
static const char* rcsID = "$Id$";

#include "uiamplspectrum.h"

#include "uiaxishandler.h"
#include "uibutton.h"
#include "uifiledlg.h"
#include "uifunctiondisplay.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uispinbox.h"

#include "arrayndimpl.h"
#include "arrayndutils.h"
#include "arrayndwrapper.h"
#include "bufstring.h"
#include "cubesampling.h"
#include "datapackbase.h"
#include "flatposdata.h"
#include "fourier.h"
#include "mouseevent.h"
#include "strmprov.h"


#define mDispVal(v)	20*Math::Log10(v+1)


uiAmplSpectrum::uiAmplSpectrum( uiParent* p, const uiAmplSpectrum::Setup& setup)
    : uiMainWin( p,"Amplitude Spectrum", 0, false, false )
    , timedomain_(0)
    , freqdomain_(0)
    , freqdomainsum_(0)
    , fft_(0)
    , data_(0)
    , specvals_(0)
    , setup_(setup)
{
    if ( !setup_.caption_.isEmpty() )
	setCaption( setup_.caption_ );
    uiFunctionDisplay::Setup su;
    su.fillbelow(true).canvaswidth(600).canvasheight(400).drawborder(true);
    disp_ = new uiFunctionDisplay( this, su );
    disp_->xAxis()->setName( SI().zIsTime() ? !setup_.iscepstrum_
						    ? "Frequency (Hz)"
						    : "Quefrency (ms)"
	    				    : "Wavenumber (/m)" );
    disp_->yAxis(false)->setName( "Power (dB)" );
    disp_->getMouseEventHandler().movement.notify( 
	    mCB(this,uiAmplSpectrum,valChgd) );

    dispparamgrp_ = new uiGroup( this, "Display Params Group" );
    dispparamgrp_->attach( alignedBelow, disp_ );
    BufferString disptitle( "Display between " );
    disptitle += SI().zIsTime() ? "frequencies" : "wavenumber" ;
    rangefld_ = new uiGenInput( dispparamgrp_, disptitle, FloatInpIntervalSpec()
			.setName(BufferString("range start"),0)
			.setName(BufferString("range stop"),1) );
    rangefld_->valuechanged.notify( mCB(this,uiAmplSpectrum,dispRangeChgd ) );
    stepfld_ = new uiLabeledSpinBox( dispparamgrp_, "Gridline step");
    stepfld_->box()->setNrDecimals( SI().zIsTime() ? 0 : 5 );
    stepfld_->attach( rightOf, rangefld_ );
    stepfld_->box()->valueChanging.notify(
	    mCB(this,uiAmplSpectrum,dispRangeChgd) );
    
    BufferString lbl =  SI().zIsTime() ? "Value(frequency, power)" : 
					 "Value(wavenumber, power)";
    valfld_ = new uiGenInput(dispparamgrp_, lbl, FloatInpIntervalSpec());
    valfld_->attach( alignedBelow, rangefld_ );
    valfld_->display( false );

    uiPushButton* exportbut = new uiPushButton( this, "Export", false );
    exportbut->activated.notify( mCB(this,uiAmplSpectrum,exportCB) );
    exportbut->attach( rightAlignedBelow, disp_ );
    
    if ( !setup_.iscepstrum_ )
    {
	uiPushButton* cepbut = new uiPushButton( this,"Display cepstrum",false);
	cepbut->activated.notify( mCB(this,uiAmplSpectrum,ceptrumCB) );
	cepbut->attach( rightAlignedBelow, exportbut );
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


void uiAmplSpectrum::setDataPackID( DataPack::ID dpid, DataPackMgr::ID dmid )
{
    DataPackMgr& dpman = DPM( dmid );
    const DataPack* datapack = dpman.obtain( dpid );
    if ( datapack )
	setCaption( !datapack ? "No data" 
	    : BufferString("Amplitude Spectrum for ",datapack->name()).buf() );

    if ( dmid == DataPackMgr::CubeID() )
    {
	mDynamicCastGet(const ::CubeDataPack*,dp,datapack);
	if ( dp )
	{
	    setup_.nyqvistspspace_ = dp->sampling().zrg.step;	    
	    setData( dp->data() );
	}
    }
    else if ( dmid == DataPackMgr::FlatID() )
    {
	mDynamicCastGet(const FlatDataPack*,dp,datapack);
	if ( dp )
	{
	    setup_.nyqvistspspace_ = dp->posData().range(false).step;	    
	    setData( dp->data() );
	}
    }

    dpman.release( dpid );
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
    memcpy( arr1d.getData(), array, sizeof(float)*size );
    setData( arr1d );
}


void uiAmplSpectrum::setData( const Array3D<float>& array )
{
    Array3DImpl<float>* data = new Array3DImpl<float>( array.info() );
    data->copyFrom( array );
    data_ = data;

    const int sz0 = array.info().getSize( 0 );
    const int sz1 = array.info().getSize( 1 );
    const int sz2 = array.info().getSize( 2 );

    nrtrcs_ = sz0 * sz1;
    initFFT( sz2 );
    if ( !fft_ )
    {
	uiMSG().error( "Cannot initialize FFT" );
	return;
    }

    compute( array );
    putDispData();
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

    const int fftsz = timedomain_->info().getSize(0);

    const int sz0 = array.info().getSize( 0 );
    const int sz1 = array.info().getSize( 1 );
    const int sz2 = array.info().getSize( 2 );

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

    return true;
}


void uiAmplSpectrum::putDispData() 
{
    const int fftsz = freqdomainsum_->info().getSize(0) / 2;
    delete specvals_;
    specvals_ = new Array1DImpl<float>( fftsz );
    Array1DImpl<float> dbspecvals( fftsz );
    for ( int idx=0; idx<fftsz; idx++ )
    {
	const float val = freqdomainsum_->get( idx )/nrtrcs_;
	specvals_->set( idx, val );
	dbspecvals.set( idx, mDispVal( val )  ); 
    }

    float maxfreq = fft_->getNyqvist( setup_.nyqvistspspace_ );
    if ( SI().zIsTime() )
	maxfreq = mNINT32( maxfreq );
    posrange_.set( 0, maxfreq );
    rangefld_->setValue( posrange_ );
    stepfld_->box()->setInterval( posrange_.start, posrange_.stop, 
	    			  (posrange_.stop-posrange_.start)/25 );
    stepfld_->box()->setValue( maxfreq/5 );
    disp_->setVals( posrange_, dbspecvals.arr(), fftsz );
    disp_->draw();
}


void uiAmplSpectrum::getSpectrumData( Array1DImpl<float>& data )
{
    if ( !specvals_ ) return;
    const int datasz = specvals_->info().getSize(0);
    data.setSize( datasz );
    for ( int idx=0; idx<datasz; idx++ )
	data.set( idx, specvals_->get(idx) );
}


void uiAmplSpectrum::dispRangeChgd( CallBacker* )
{
    StepInterval<float> rg = rangefld_->getFInterval();
    rg.step = stepfld_->box()->getFValue();
    if ( posrange_.start > rg.start || posrange_.stop < rg.stop 
	    || rg.stop <=0 || rg.start >= rg.stop )
    { 
	rg.start = posrange_.start; rg.stop = posrange_.stop;
        rg.step = ( rg.stop - rg.start )/5;
	rangefld_->setValue( posrange_ ); 
    }
    disp_->xAxis()->setRange( rg ); 
    disp_->draw();
}


void uiAmplSpectrum::exportCB( CallBacker* )
{
    uiFileDialog dlg( this, false );
    if ( !dlg.go() ) return;

    const char* fname = dlg.fileName();
    StreamData sdo = StreamProvider( fname ).makeOStream();
    if ( !sdo.usable() )
    {
	sdo.close();
	uiMSG().error( "Cannot open output file" );
	return;
    }

    disp_->dump( *sdo.ostrm, false );

    uiMSG().message( "Values written to: ", fname );
    sdo.close();
}


void uiAmplSpectrum::valChgd( CallBacker* )
{
    if ( !specvals_ ) return;

    const Geom::Point2D<int>& pos = disp_->getMouseEventHandler().event().pos();
    Interval<float> rg( disp_->xAxis()->getVal( pos.x ), 
			disp_->yAxis(false)->getVal( pos.y ) );
    const bool disp = disp_->xAxis()->range().includes(rg.start,true) && 
		      disp_->yAxis(false)->range().includes(rg.stop,true);
    valfld_->display( disp );
    if ( !disp )
	return;

    const float ratio = (rg.start-posrange_.start)/posrange_.width();
    const float specsize = specvals_->info().getSize(0);
    const int specidx = mNINT32( ratio * specsize );
    if ( !specvals_->info().validPos(specidx) )
	return;

    rg.stop = mDispVal( specvals_->get(specidx) );
    valfld_->setValue( rg );
}


void uiAmplSpectrum::ceptrumCB( CallBacker* )
{
    if ( !specvals_ ) return;

    uiAmplSpectrum* ampl = new uiAmplSpectrum(
	    this, uiAmplSpectrum::Setup("Amplitude Cepstrum", true, 1  ) );
    ampl->setDeleteOnClose( true );
    ampl->setData( *specvals_ );
    ampl->show();
}
