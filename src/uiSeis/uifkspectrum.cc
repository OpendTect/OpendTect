/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2012
_______________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uifkspectrum.h"

#include "uitoolbutton.h"
#include "uiflatviewer.h"
#include "uiflatviewstdcontrol.h"
#include "uigeninput.h"
#include "uigraphicsview.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uiworld2ui.h"

#include "arrayndimpl.h"
#include "bufstring.h"
#include "datapackbase.h"
#include "flatposdata.h"
#include "fourier.h"
#include "keystrs.h"
#include "mouseevent.h"


uiFKSpectrum::uiFKSpectrum( uiParent* p, bool setbp )
    : uiFlatViewMainWin(p,Setup("FK Spectrum"))
    , fft_(0)
    , input_(0)
    , output_(0)
    , spectrum_(0)
    , minfld_(0), minvelitm_(0)
    , maxfld_(0), maxvelitm_(0)
{
    uiFlatViewer& vwr = viewer();
    vwr.setInitialSize( uiSize(600,400) );
    vwr.appearance().setDarkBG( false );
    vwr.appearance().setGeoDefaults( false );
    vwr.appearance().annot_.setAxesAnnot(true);
    vwr.appearance().ddpars_.wva_.allowuserchange_ = false;
    vwr.appearance().ddpars_.vd_.show_ = true;
    addControl( new uiFlatViewStdControl(vwr,
			uiFlatViewStdControl::Setup(0).withthumbnail(false)) );

    vwr.rgbCanvas().getMouseEventHandler().movement.notify(
	mCB(this,uiFKSpectrum,mouseMoveCB) );
    vwr.rgbCanvas().getMouseEventHandler().buttonPressed.notify(
	mCB(this,uiFKSpectrum,mousePressCB) );

    lineitm_ = initAuxData();

    ffld_ = new uiGenInput( this, SI().zIsTime() ? tr("F") : tr("Kz") );
    ffld_->setReadOnly();
    kfld_ = new uiGenInput( this, tr("K") );
    kfld_->setReadOnly();
    velfld_ = new uiGenInput( this, SI().zIsTime() ? tr("Vel (m/s)") 
                                                   : tr("Dip (deg)"));
    velfld_->setReadOnly();
    ffld_->attach( leftAlignedBelow, &vwr );
    kfld_->attach( rightTo, ffld_ );
    velfld_->attach( rightTo, kfld_ );

    if ( setbp )
    {
	uiSeparator* sep = new uiSeparator( this, "HorSep", OD::Horizontal );
	sep->attach( stretchedBelow, ffld_ );
	uiString minlbl = SI().zIsTime() ? tr("Min Vel") : tr("Min Dip");
	minfld_ = new uiGenInput( this, minlbl );
	minfld_->setReadOnly();
	minfld_->attach( leftAlignedBelow, ffld_ );
	minfld_->attach( ensureBelow, sep );
	minsetbut_ = new uiToolButton( this, "pick", tr("Set min velocity"),
				       mCB(this,uiFKSpectrum,setVelCB) );
	minsetbut_->setToggleButton();
	minsetbut_->attach( rightTo, minfld_ );

	uiString maxlbl = SI().zIsTime() ? tr("Min Vel") : tr("Min Dip");
	maxfld_ = new uiGenInput( this, maxlbl );
	maxfld_->setReadOnly();
	maxfld_->attach( rightOf, minsetbut_ );
	maxsetbut_ = new uiToolButton( this, "pick", tr("Set max velocity"),
				       mCB(this,uiFKSpectrum,setVelCB) );
	maxsetbut_->setToggleButton();
	maxsetbut_->attach( rightTo, maxfld_ );

	minvelitm_ = initAuxData();
	maxvelitm_ = initAuxData();
    }
}


uiFKSpectrum::~uiFKSpectrum()
{
    delete input_;
    delete output_;
    delete fft_;
}


FlatView::AuxData* uiFKSpectrum::initAuxData()
{
    FlatView::AuxData* ad = viewer().createAuxData(0);
    ad->linestyle_.type_ = LineStyle::Solid;
    ad->linestyle_.width_ = 2;
    ad->linestyle_.color_ = Color::Black();
    viewer().addAuxData( ad );
    return ad;
}


float uiFKSpectrum::getMinValue() const
{ return minfld_ ? minfld_->getfValue() : mUdf(float); }

float uiFKSpectrum::getMaxValue() const
{ return maxfld_ ? maxfld_->getfValue() : mUdf(float); }



static void updateTB( uiToolButton& tb, bool quietmode )
{
    tb.setPixmap( quietmode ? "pick" : "stop" );
    tb.setToolTip( quietmode ? "Set velocity" : "Cancel" );
}


void uiFKSpectrum::setVelCB( CallBacker* cb )
{
    uiToolButton* but = cb==minsetbut_ ? minsetbut_ : maxsetbut_;
    const bool quietmode = !but->isOn();
    updateTB( *but, quietmode );
}


void uiFKSpectrum::mouseMoveCB( CallBacker* )
{
    lineitm_->poly_.erase();

    const MouseEvent& ev = viewer().rgbCanvas().getMouseEventHandler().event();
    uiWorld2Ui w2u;
    viewer().getWorld2Ui( w2u );
    uiWorldPoint wp = w2u.transform( ev.pos() );
    lineitm_->poly_ += wp;
    lineitm_->poly_ += FlatView::Point( 0, 0 );
    lineitm_->poly_ += FlatView::Point( -wp.x, wp.y );

    const int nrdec = SI().zIsTime() ? 1 : 3;
    ffld_->setText( toString(wp.y,nrdec) );
    kfld_->setText( toString(wp.x,4) );
    const double vel = mIsZero(wp.x,mDefEpsD)? 0 : Math::Abs(wp.y/wp.x);
    velfld_->setText( toString(vel,nrdec) );

    viewer().handleChange( FlatView::Viewer::Auxdata );
}


void uiFKSpectrum::mousePressCB( CallBacker* )
{
    if ( !minvelitm_ ) return;

    if ( minsetbut_->isOn() )
    {
	minvelitm_->poly_ = lineitm_->poly_;
	minfld_->setText( velfld_->text() );
	updateTB( *minsetbut_, true );
	minsetbut_->setOn( false );
    }
    if ( maxsetbut_->isOn() )
    {
	maxvelitm_->poly_ = lineitm_->poly_;
	maxfld_->setText( velfld_->text() );
	updateTB( *maxsetbut_, true );
	maxsetbut_->setOn( false );
    }

    viewer().handleChange( FlatView::Viewer::Auxdata );
}


void uiFKSpectrum::setDataPackID( DataPack::ID dpid, DataPackMgr::ID dmid )
{
    DataPackMgr& dpman = DPM( dmid );
    const DataPack* datapack = dpman.obtain( dpid );
    setCaption( !datapack ? tr("No data")
	    : BufferString("F-K Spectrum for ",datapack->name()).buf() );

    if ( dmid == DataPackMgr::FlatID() )
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
	uiMSG().error( tr("Cannot initialize FFT") );
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
