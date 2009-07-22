/*+
________________________________________________________________________
            
 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:          September 2007
_______________________________________________________________________
                   
-*/   
static const char* rcsID = "$Id: uiamplspectrum.cc,v 1.14 2009-07-22 16:01:42 cvsbert Exp $";

#include "uiamplspectrum.h"

#include "uiaxishandler.h"
#include "uifunctiondisplay.h"

#include "arrayndimpl.h"
#include "arrayndutils.h"
#include "arrayndwrapper.h"
#include "bufstring.h"
#include "datapackbase.h"
#include "fft.h"
#include "survinfo.h"


uiAmplSpectrum::uiAmplSpectrum( uiParent* p )
    : uiMainWin( p,"Amplitude Spectrum", 0, false, false )
    , timedomain_(0)
    , freqdomain_(0)
    , freqdomainsum_(0)
    , fft_(0)
{
    uiFunctionDisplay::Setup su;
    su.fillbelow(true).canvaswidth(600).canvasheight(400).drawborder(true);
    disp_ = new uiFunctionDisplay( this, su );
    disp_->xAxis()->setName( SI().zIsTime() ? "Frequency (Hz)" 
	    				    : "Wavenumber (/m)" );
    disp_->yAxis(false)->setName( "Power (dB)" );
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
    if ( datapack )
	setCaption( !datapack ? "No data" 
	    : BufferString("Amplitude Spectrum for ",datapack->name()).buf() );

    if ( dmid == DataPackMgr::CubeID() )
    {
	mDynamicCastGet(const ::CubeDataPack*,cdp,datapack);
	const Array3D<float>* arr3d = cdp ? &cdp->data() : 0;
	if ( arr3d ) setData( *arr3d );
    }
    else if ( dmid == DataPackMgr::FlatID() )
    {
	mDynamicCastGet(const FlatDataPack*,fdp,datapack);
	const Array2D<float>* arr2d = fdp ? &fdp->data() : 0;
	if ( arr2d ) setData( *arr2d );
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


void uiAmplSpectrum::setData( const Array3D<float>& array )
{
    const int sz0 = array.info().getSize( 0 );
    const int sz1 = array.info().getSize( 1 );
    const int sz2 = array.info().getSize( 2 );

    nrtrcs_ = sz0 * sz1;
    initFFT( sz2 );
    compute( array );
    putDispData();
}


void uiAmplSpectrum::initFFT( int nrsamples ) 
{
    fft_ = new FFT();
    const int fftsz = fft_->nearestBiggerFastSize( nrsamples );
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

	    fft_->transform( *timedomain_, *freqdomain_ );
	    for ( int idz=0; idz<sz2; idz++ )
		freqdomainsum_->arr()[idz] += abs(freqdomain_->get(idz));
	}
    }

    return true;
}


void uiAmplSpectrum::putDispData() 
{
    const int fftsz = freqdomainsum_->info().getSize(0) / 2;
    TypeSet<float> vals( fftsz, 0 );
    for ( int idx=0; idx<fftsz; idx++ )
	vals[idx] = 20 * Math::Log10( abs(freqdomainsum_->get(idx)) / nrtrcs_ );

    float maxfreq = fft_->getNyqvist( SI().zStep() );
    if ( SI().zIsTime() )
	maxfreq = mNINT( maxfreq );
    disp_->setVals( Interval<float>(0,maxfreq), vals.arr(), fftsz );
    disp_->draw();
}
