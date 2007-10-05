/*+
________________________________________________________________________
            
 CopyRight:     (C) dGB Beheer B.V.
 Author:	Satyaki Maitra
 Date:          September 2007
 RCS:           $Id: uiamplspectrum.cc,v 1.1 2007-10-05 09:02:59 cvssatyaki Exp $
_______________________________________________________________________
                   
-*/   

#include "uiamplspectrum.h"

#include "uicanvas.h"
#include "uihistogramdisplay.h"
#include "uigeom.h"

#include "arrayndimpl.h"
#include "arrayndutils.h"
#include "bufstring.h"
#include "datapackbase.h"
#include "drawaxis2d.h"
#include "fft.h"
#include "sets.h"
#include "survinfo.h"

#define mTransHeight    250
#define mTransWidth     500


uiAmplSpectrum ::uiAmplSpectrum( uiParent* p )
        : uiDialog( p, uiDialog::Setup("Amplitude Spectrum",0,0) )
	, timedomain_(0)
 	, freqdomain_(0)
	, freqdomainsum_(0)
        , fft_(0)
{
    setCtrlStyle( LeaveOnly );
    canvas_ = new uiCanvas( this );
    canvas_->setPrefHeight( mTransHeight );
    canvas_->setPrefWidth( mTransWidth );
    canvas_->setStretch(0,0);
    histogramdisplay_ = new uiHistogramDisplay( canvas_ );
    histogramdisplay_->setColor( Color(200,100,65,5) );
    canvas_->postDraw.notify( mCB(this,uiAmplSpectrum,reDraw) );

    bdrect_ = uiRect( 20, 5, 20, 20 );
}


uiAmplSpectrum::~uiAmplSpectrum()
{
    delete fft_;
    delete timedomain_;
    delete freqdomain_;
    delete freqdomainsum_;
}


void uiAmplSpectrum::setDataPackID( DataPack::ID dpid, DataPackMgr::ID dmid )
{
    DataPackMgr& dpman = DPM( dmid );
    const DataPack* datapack = dpman.obtain( dpid );

    if ( dmid == DataPackMgr::CubeID )
    {
	mDynamicCastGet(const ::CubeDataPack*,cdp,datapack);
	const Array3D<float>* arr3d = cdp ? &cdp->data() : 0;
	if ( arr3d )
	    setData( *arr3d );

    }
    else if ( dmid == DataPackMgr::FlatID )
    {
	mDynamicCastGet(const FlatDataPack*,fdp,datapack);
	const Array2D<float>* arr2d = fdp ? &fdp->data() : 0;
	if ( arr2d )
	    setData( *arr2d );
    }

    dpman.release( dpid );
}


void uiAmplSpectrum::setData( const Array2D<float>& array )
{
    const int sz2d0 = array.info().getSize( 0 );
    const int sz2d1 = array.info().getSize( 1 );
    
    nrtrcs_ = sz2d0;
    const int nrsamples = sz2d1;
    initFFT( nrsamples );
    compute2D( array );
    setHistogram();
}


void uiAmplSpectrum::setData( const Array3D<float>& array )
{
    const int sz3d0 = array.info().getSize( 0 );
    const int sz3d1 = array.info().getSize( 1 );
    const int sz3d2 = array.info().getSize( 2 );

    nrtrcs_ = sz3d0 * sz3d1;
    initFFT( sz3d2 );
    compute3D( array );
    setHistogram();
}


void uiAmplSpectrum::initFFT( int nrsamples ) 
{
    fft_ = new FFT();
    const int fftsz = fft_->_getNearBigFastSz( nrsamples );
    fft_->setInputInfo( Array1DInfoImpl(fftsz) );
    fft_->setDir( true );
    fft_->init();

    timedomain_ = new Array1DImpl<float_complex>( fftsz );
    freqdomain_ = new Array1DImpl<float_complex>( fftsz );
    freqdomainsum_ = new Array1DImpl<float_complex>( fftsz );

    for ( int idx=0; idx<fftsz; idx++)
    {
	timedomain_->set( idx, 0 );
	freqdomainsum_->set( idx,0 );
    }
}


bool uiAmplSpectrum::compute2D( const Array2D<float>& arr2d )
{
    if ( !timedomain_ || !freqdomain_ ) return false;

    const int fftsz = timedomain_->info().getSize( 0 );

    const int sz0 = arr2d.info().getSize( 0 );
    const int sz1 = arr2d.info().getSize( 1 );

    const int start = (fftsz-sz1) / 2;
    for ( int trcidx=0; trcidx<nrtrcs_; trcidx++ )
    {
	for ( int idy=0; idy<sz1; idy++ )
	{
	    timedomain_->set( start+idy, arr2d.get(trcidx,idy) );
	}
    
	fft_->transform( *timedomain_, *freqdomain_ );
	setFrequency();
    }

    return true;
}


bool uiAmplSpectrum::compute3D( const Array3D<float>& arr3d )
{
    if ( !timedomain_ || !freqdomain_ ) return false;

    const int fftsz = timedomain_->info().getSize(0);

    const int sz0 = arr3d.info().getSize( 0 );
    const int sz1 = arr3d.info().getSize( 1 );
    const int sz2 = arr3d.info().getSize( 2 );

    const int start = (fftsz-sz2) / 2;
    for ( int idx=0; idx<sz0; idx++ )
    {
	for ( int idy=0; idy<sz1; idy++ )
	{
	    for ( int idz=0; idz<sz2; idz++ )
	    {
		timedomain_->set( start+idz, arr3d.get( idx, idy, idz ) );
	    }

	    fft_->transform( *timedomain_, *freqdomain_ );
	    setFrequency();
	}
    }

    return true;
}
	

bool uiAmplSpectrum::setFrequency()
{
    if ( !freqdomainsum_ || !freqdomain_ ) return false;
    
    const int fftsz = timedomain_->info().getSize(0);

    for ( int idx=0; idx<fftsz; idx ++ )
    {
	float_complex sum = freqdomainsum_->get(idx) +
			    freqdomain_->get(idx);
        freqdomainsum_->set( idx, sum );
    }
    return true;
}
        

void uiAmplSpectrum::setHistogram() 
{
    const int fftsz = freqdomainsum_->info().getSize(0) / 2;
    TypeSet<float> histogram( fftsz, 0 );

    for ( int idx=0; idx<fftsz; idx++ )
    {
	const float val = abs(freqdomainsum_->get(idx)) / nrtrcs_;
	histogram[idx] = val;
    }

    const float df = FFT::getDf( SI().zStep(), fftsz );
    SamplingData<float> sd( 0, df );
    histogramdisplay_->setHistogram( histogram, sd );
    histogramdisplay_->setBoundaryRect( bdrect_ );

    const float maxfreq = mNINT( fft_->getNyqvist( SI().zStep() ) );
    const float step = maxfreq/5;
    StepInterval<float> xrg( 0, maxfreq, step );
    histogramdisplay_->setXAxis( xrg ); 
}


void uiAmplSpectrum::reDraw( CallBacker* cb )
{
    ioDrawTool& drawtool = canvas_->drawTool();
    drawtool.setBackgroundColor( Color::White );
    drawtool.clear();
    histogramdisplay_->reDraw( cb );
}
