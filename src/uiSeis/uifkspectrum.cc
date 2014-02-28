/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2012
_______________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uifkspectrum.h"

#include "uimsg.h"
#include "uiflatviewer.h"
#include "uiflatviewstdcontrol.h"

#include "arrayndimpl.h"
#include "bufstring.h"
#include "datapackbase.h"
#include "flatposdata.h"
#include "fourier.h"
#include "keystrs.h"


uiFKSpectrum::uiFKSpectrum( uiParent* p )
    : uiFlatViewMainWin(p,Setup("FK Spectrum"))
    , fft_(0)
    , input_(0)
    , output_(0)
    , spectrum_(0)
{
    uiFlatViewer& vwr = viewer();
    vwr.setInitialSize( uiSize(600,400) );
    vwr.appearance().setDarkBG( false );
    vwr.appearance().setGeoDefaults( false );
    vwr.appearance().annot_.setAxesAnnot(true);
    vwr.appearance().ddpars_.wva_.allowuserchange_ = false;
    vwr.appearance().ddpars_.vd_.show_ = true;
    addControl( new uiFlatViewStdControl(vwr,uiFlatViewStdControl::Setup(0)) );
}


uiFKSpectrum::~uiFKSpectrum()
{
    delete input_;
    delete output_;
    delete fft_;
}


void uiFKSpectrum::setDataPackID( DataPack::ID dpid, DataPackMgr::ID dmid )
{
    DataPackMgr& dpman = DPM( dmid );
    const DataPack* datapack = dpman.obtain( dpid );
    if ( datapack )
	setCaption( !datapack ? "No data"
	    : BufferString("F-K Spectrum for ",datapack->name()).buf() );

    if ( dmid == DataPackMgr::CubeID() )
    {
    }
    else if ( dmid == DataPackMgr::FlatID() )
    {
	mDynamicCastGet(const FlatDataPack*,dp,datapack);
	if ( dp )
	    setData( dp->data() );
    }

    dpman.release( dpid );
}


void uiFKSpectrum::setData( const Array2D<float>& array )
{
    const int sz0 = array.info().getSize( 0 );
    const int sz1 = array.info().getSize( 1 );
    initFFT( sz0, sz1 );
    if ( !fft_ )
    {
	uiMSG().error( "Cannot initialize FFT" );
	return;
    }

    if ( !compute(array) )
	return;

    for ( int idx=0; idx<sz0; idx++ )
    {
	const int kidx = idx<sz0/2 ? idx+sz0/2 : idx-sz0/2;
	for ( int idy=0; idy<sz1/2; idy++ )
	    spectrum_->set( kidx, idy, abs(output_->get(idx,idy)) );
    }

    view( *spectrum_ );
}


void uiFKSpectrum::initFFT( int nrtrcs, int nrsamples )
{
    fft_ = Fourier::CC::createDefault();
    if ( !fft_ ) return;

    const int xsz = nrtrcs; // fft_->getFastSize( nrtrcs );
    const int zsz = nrsamples; // fft_->getFastSize( nrsamples );
    fft_->setInputInfo( Array2DInfoImpl(xsz,zsz) );
    fft_->setDir( true );

    input_ = new Array2DImpl<float_complex>( xsz, zsz );
    input_->setAll( float_complex(0,0) );
    output_ = new Array2DImpl<float_complex>( xsz, zsz );
    output_->setAll( float_complex(0,0) );

    spectrum_ = new Array2DImpl<float>( xsz, zsz/2 );
    spectrum_->setAll( 0 );
}


bool uiFKSpectrum::compute( const Array2D<float>& array )
{
    if ( !output_ ) return false;

    const int sz0 = array.info().getSize( 0 );
    const int sz1 = array.info().getSize( 1 );
    for ( int idx=0; idx<sz0; idx++ )
    {
	for ( int idy=0; idy<sz1; idy++ )
	{
	    const float val = array.get( idx, idy );
	    input_->set( idx, idy, mIsUdf(val) ? 0 : val );
	}
    }

    fft_->setInput( input_->getData() );
    fft_->setOutput( output_->getData() );
    fft_->run( true );

    return true;
}


bool uiFKSpectrum::view( Array2D<float>& array )
{
    MapDataPack* datapack = new MapDataPack( sKey::Attribute(), "F-K", &array );

    const int nrk = array.info().getSize( 0 );
    const int nrtrcs = input_->info().getSize( 0 );
    const float dk = fft_->getDf( SI().crlDistance(), nrtrcs );
    const StepInterval<double> krg( -dk*(nrk-1)/2, dk*(nrk-1)/2, dk );

    const int nrf = array.info().getSize( 1 );
    const int nrz = input_->info().getSize( 1 );
    const float df = fft_->getDf( SI().zStep(), nrz );
    const StepInterval<double> frg( 0, df*(nrf-1), df );

    datapack->posData().setRange( true, krg );
    datapack->posData().setRange( false, frg );
    datapack->setPosCoord( false );
    datapack->setDimNames( "K", "Freq", true );
    DataPackMgr& dpman = DPM(DataPackMgr::FlatID());
    dpman.add( datapack );
    viewer().setPack( false, datapack->id(), false );

    return true;
}
