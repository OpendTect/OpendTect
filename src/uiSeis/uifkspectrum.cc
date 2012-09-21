/*+
________________________________________________________________________
            
 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:          September 2007
_______________________________________________________________________
                   
-*/   
static const char* rcsID mUnusedVar = "$Id$";

#include "uifkspectrum.h"

#include "uimsg.h"
#include "uiflatviewer.h"
#include "uiflatviewstdcontrol.h"

#include "arrayndimpl.h"
#include "bufstring.h"
#include "datapackbase.h"
#include "flatposdata.h"
#include "fourier.h"


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
	for ( int idy=0; idy<sz1; idy++ )
	    spectrum_->set( idx, idy, abs(output_->get(idx,idy)) );

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

    spectrum_ = new Array2DImpl<float>( xsz, zsz );
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
    MapDataPack* datapack = new MapDataPack( "Attribute", "F-K", &array );
    const int sz0 = array.info().getSize( 0 );
    const int sz1 = array.info().getSize( 1 );
    // TODO: Get dk and df from input data
    const float dk = fft_->getDf( SI().crlDistance(), sz0 );
    const float df = fft_->getDf( SI().zStep(), sz1 );
    StepInterval<double> frg( 0, df*(sz0-1), df );
    StepInterval<double> krg( 0, dk*(sz1-1), dk );
    datapack->posData().setRange( true, krg );
    datapack->posData().setRange( false, frg );
    datapack->setPosCoord( false );
    datapack->setDimNames( "K", "Freq", true );
    DataPackMgr& dpman = DPM(DataPackMgr::FlatID());
    dpman.add( datapack );
    viewer().setPack( false, datapack->id(), false, false );

    return true;
}
