/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uifkspectrum.h"

#include "uiflatviewer.h"
#include "uiflatviewstdcontrol.h"
#include "uigeninput.h"
#include "uigraphicsview.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uistrings.h"
#include "uitoolbutton.h"

#include "arrayndimpl.h"
#include "arrayndslice.h"
#include "bufstring.h"
#include "datapackbase.h"
#include "envvars.h"
#include "flatposdata.h"
#include "fourier.h"
#include "keystrs.h"
#include "mouseevent.h"
#include "seisdatapack.h"
#include "survinfo.h"


uiFKSpectrum::uiFKSpectrum( uiParent* p, bool setbp )
    : uiFlatViewMainWin(p,Setup(tr("FK Spectrum")))
{
    uiFlatViewer& vwr = viewer();
    vwr.setInitialSize( uiSize(600,400) );
    FlatView::Appearance& app = vwr.appearance();
    app.setDarkBG( false );
    app.setGeoDefaults( false );
    app.annot_.setAxesAnnot(true);
    app.annot_.x1_.name_ = SI().xyInFeet() ? "K (/ft)" : "K (/m)";
    app.annot_.x2_.name_ = SI().zIsTime() ? "F (Hz)" :
		SI().zInMeter() ? "Kz (/m)" : "Kz (/ft)";
    app.ddpars_.wva_.allowuserchange_ = false;
    app.ddpars_.vd_.show_ = true;
    app.ddpars_.vd_.mappersetup_.cliprate_ = Interval<float>(0.005,0.005);
    addControl( new uiFlatViewStdControl(vwr,uiFlatViewStdControl::Setup(0)) );

    mAttachCB( vwr.rgbCanvas().getMouseEventHandler().movement,
	       uiFKSpectrum::mouseMoveCB );
    mAttachCB( vwr.rgbCanvas().getMouseEventHandler().buttonPressed,
	       uiFKSpectrum::mousePressCB );

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
	auto* sep = new uiSeparator( this, "HorSep", OD::Horizontal );
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

	uiString maxlbl = SI().zIsTime() ? tr("Max Vel") : tr("Max Dip");
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
    detachAllNotifiers();
    delete fft_;
    delete input_;
    delete output_;
    delete spectrum_;
}


FlatView::AuxData* uiFKSpectrum::initAuxData()
{
    FlatView::AuxData* ad = viewer().createAuxData(0);
    ad->linestyle_.type_ = OD::LineStyle::Solid;
    ad->linestyle_.width_ = 2;
    ad->linestyle_.color_ = OD::Color::Black();
    viewer().addAuxData( ad );
    return ad;
}


float uiFKSpectrum::getMinValue() const
{ return minfld_ ? minfld_->getFValue() : mUdf(float); }

float uiFKSpectrum::getMaxValue() const
{ return maxfld_ ? maxfld_->getFValue() : mUdf(float); }



static void updateTB( uiToolButton& tb, bool quietmode )
{
    tb.setIcon( quietmode ? "pick" : "stop" );
    tb.setToolTip( quietmode ? od_static_tr("updateTB","Set velocity") :
					     uiStrings::sCancel() );
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
    const uiWorldPoint wp = viewer().getWorld2Ui().transform( ev.pos() );
    lineitm_->poly_ += wp;
    lineitm_->poly_ += FlatView::Point( 0, 0 );
    lineitm_->poly_ += FlatView::Point( -wp.x_, wp.y_ );

    const int nrdec = SI().zIsTime() ? 1 : 3;
    ffld_->setText( toString(wp.y_,nrdec) );
    kfld_->setText( toString(wp.x_,4) );
    const double vel = mIsZero(wp.x_,mDefEpsD)? 0 : Math::Abs(wp.y_/wp.x_);
    velfld_->setText( toString(vel,nrdec) );

    viewer().handleChange( sCast(od_uint32,FlatView::Viewer::Auxdata) );
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

    viewer().handleChange( sCast(od_uint32,FlatView::Viewer::Auxdata) );
}


bool uiFKSpectrum::setDataPackID( const DataPackID& dpid,
				  const DataPackMgr::MgrID& dmid, int version )
{
    ConstRefMan<SeisDataPack> seisdp = DPM(dmid).getDP( dpid );
    return seisdp ? setDataPack( *seisdp.ptr(), version ) : false;
}


bool uiFKSpectrum::setDataPack( const SeisDataPack& seisdp, int version )
{
    setCaption( tr("F-K Spectrum for %1").arg( seisdp.name() ) );

    if ( seisdp.isEmpty() )
	return false;

    mDynamicCastGet(const RegularSeisDataPack*,regsdp,&seisdp);
    const TrcKeyZSampling::Dir dir = regsdp ?
	    regsdp->sampling().defaultDir() : TrcKeyZSampling::Inl;
    const int dim0 = dir==TrcKeyZSampling::Inl ? 1 : 0;

    Array2DSlice<float> slice2d( seisdp.data(version) );
    slice2d.setDimMap( 0, dim0 );
    slice2d.setDimMap( 1, 2 );
    slice2d.setPos( dir, 0 );
    slice2d.init();
    return setData( slice2d );
}


bool uiFKSpectrum::setData( const Array2D<float>& array )
{
    const int sz0 = array.info().getSize( 0 );
    const int sz1 = array.info().getSize( 1 );
    if ( !initFFT(sz0,sz1) )
    {
	uiMSG().error( tr("Cannot initialize FFT") );
	return false;
    }

    if ( !compute(array) )
	return false;

    for ( int idx=0; idx<sz0; idx++ )
    {
	const int kidx = idx<sz0/2 ? idx+sz0/2 : idx-sz0/2;
	for ( int idy=0; idy<sz1/2; idy++ )
	{
	    const float power = abs(output_->get(idx,idy));
	    spectrum_->set( kidx, idy, power );
	    if ( GetEnvVarYN("OD_FK_INDB") )
		spectrum_->set( kidx, idy, 20*Math::Log10(power) );
	}
    }

    return view( *spectrum_ );
}


bool uiFKSpectrum::initFFT( int nrtrcs, int nrsamples )
{
    fft_ = Fourier::CC::createDefault();
    if ( !fft_ )
	return false;

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
    return true;
}


bool uiFKSpectrum::compute( const Array2D<float>& array )
{
    if ( !output_ )
	return false;

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
    RefMan<FlatDataPack> datapack = new FlatDataPack( sKey::Attribute(),
						      &array );
    datapack->setName( "Power" );
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
    vddp_ = datapack;
    viewer().setPack( FlatView::Viewer::VD, datapack.ptr(), false );

    return true;
}
