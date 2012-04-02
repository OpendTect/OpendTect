/*+
________________________________________________________________________
            
 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:          September 2007
_______________________________________________________________________
                   
-*/   
static const char* rcsID = "$Id: uifkspectrum.cc,v 1.1 2012-04-02 22:45:42 cvsnanne Exp $";

#include "uifkspectrum.h"

#include "uimsg.h"

#include "arrayndimpl.h"
#include "bufstring.h"
#include "datapackbase.h"
#include "flatposdata.h"
#include "fourier.h"


uiFKSpectrum::uiFKSpectrum( uiParent* p )
    : uiMainWin( p,"FK Spectrum", 0, false, false )
    , fft_(0)
    , output_(0)
{
}


uiFKSpectrum::~uiFKSpectrum()
{
    delete output_;
    delete fft_;
}


void uiFKSpectrum::setDataPackID( DataPack::ID dpid, DataPackMgr::ID dmid )
{
    DataPackMgr& dpman = DPM( dmid );
    const DataPack* datapack = dpman.obtain( dpid );
    if ( datapack )
	setCaption( !datapack ? "No data" 
	    : BufferString("Amplitude Spectrum for ",datapack->name()).buf() );

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

    view( *output_ );
}


void uiFKSpectrum::initFFT( int nrtrcs, int nrsamples ) 
{
    fft_ = Fourier::CC::createDefault();
    if ( !fft_ ) return;

    const int xsz = nrtrcs; // fft_->getFastSize( nrtrcs );
    const int zsz = nrsamples; // fft_->getFastSize( nrsamples );
    fft_->setInputInfo( Array2DInfoImpl(xsz,zsz) );
    fft_->setDir( true );

    output_ = new Array2DImpl<float>( zsz, zsz );
}


bool uiFKSpectrum::compute( const Array2D<float>& array )
{
    if ( !output_ ) return false;

    fft_->setInput( array.getData() );
    fft_->setOutput( output_->getData() );
    fft_->run( true );

    return true;
}


bool uiFKSpectrum::view( const Array2D<float>& array )
{
    /*
    MapDataPack* datapack = new MapDataPack( "Attribute", "Thickness", data );
    StepInterval<double> inlrg( hs_.start.inl, hs_.stop.inl, hs_.step.inl );
    StepInterval<double> crlrg( hs_.start.crl, hs_.stop.crl, hs_.step.crl );
    datapack->posData().setRange( true, inlrg );
    datapack->posData().setRange( false, crlrg );
    datapack->setPosCoord( false );
    datapack->setDimNames( "In-line", "Cross-line", true );
    DataPackMgr& dpman = DPM(DataPackMgr::FlatID());
    dpman.add( datapack );

    uiFlatViewMainWin* fvmw = new uiFlatViewMainWin( 0,
	    			 uiFlatViewMainWin::Setup( "Thickness Map") );
    uiFlatViewer& vwr = fvmw->viewer();
    vwr.setInitialSize( uiSize(600,400) );
    vwr.appearance().setDarkBG( false );
    vwr.appearance().setGeoDefaults( false );
    vwr.appearance().annot_.setAxesAnnot(true);
    vwr.appearance().ddpars_.wva_.allowuserchange_ = false;
    fvmw->addControl( new uiFlatViewStdControl( vwr, 
					uiFlatViewStdControl::Setup(0) ) );
    vwr.setPack( false, datapack->id(), false, false );
    vwr.appearance().ddpars_.vd_.show_ = true;
    fvmw->start();
    */

    return true;
}
