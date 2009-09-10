/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Mar 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelltiewavelet.cc,v 1.27 2009-09-10 09:33:16 cvsbruno Exp $";

#include "uiwelltiewavelet.h"

#include "flatposdata.h"
#include "fft.h"
#include "hilberttransform.h"
#include "ioman.h"
#include "pixmap.h"
#include "survinfo.h"
#include "wavelet.h"
#include "welltiedata.h"
#include "welltiegeocalculator.h"

#include "uiaxishandler.h"
#include "uiseiswvltattr.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uiflatviewer.h"
#include "uifunctiondisplay.h"
#include "uilabel.h"
#include "uimsg.h"

#include <complex>

namespace WellTie
{

#define mErrRet(msg,act) \
{ uiMSG().error(msg); act; }
uiWaveletView::uiWaveletView( uiParent* p, WellTie::DataHolder* dh )
	: uiGroup(p)
	, dataholder_(dh)  
	, wvltctio_(*mMkCtxtIOObj(Wavelet))
	, activewvltChged(this)		
{
    createWaveletFields( this );
} 


uiWaveletView::~uiWaveletView()
{
    deepErase( uiwvlts_  );
}


void uiWaveletView::createWaveletFields( uiGroup* grp )
{
    grp->setHSpacing( 40 );
    
    activewvltfld_ = new uiGenInput( this, "Active Wavelet : ",
	 			BoolInpSpec(true,"Initial","Estimated")  );
    activewvltfld_->attach( hCentered );
    activewvltfld_->valuechanged.notify(
	   		 mCB(this, uiWaveletView, activeWvltChanged) );
}


void uiWaveletView::initWavelets( )
{
    deepErase( uiwvlts_ );
    IOObj* ioobj = IOM().get( MultiID( dataholder_->setup().wvltid_ ) );
    uiwvlts_ += new uiWavelet( this, Wavelet::get( ioobj) );
    uiwvlts_ += new uiWavelet( this, dataholder_->getEstimatedWvlt() );
    uiwvlts_[0]->attach( ensureBelow, activewvltfld_ );
    uiwvlts_[1]->attach( rightOf, uiwvlts_[0] );
}


void uiWaveletView::activeWvltChanged( CallBacker* )
{
    dataholder_->dpms()->isinitwvltactive_ = activewvltfld_->getBoolValue();
    activewvltChged.trigger();
}



uiWavelet::uiWavelet( uiParent* p, Wavelet* wvlt )
    : uiGroup(p)
    , wvlt_(wvlt)
    , wvltpropdlg_(0)  
{
    viewer_ = new uiFlatViewer( this );
    uiLabel* wvltlbl = new uiLabel( this, wvlt->name() );
    wvltlbl->attach( alignedAbove, viewer_);
    
    wvltbuts_ += new uiToolButton( this, "Properties", "info.png",
	    mCB(this,uiWavelet,viewWvltPropPushed) );
    wvltbuts_[0]->setToolTip( "Properties" );
    wvltbuts_[0]->attach( alignedBelow, viewer_ );


    wvltbuts_ += new uiToolButton( this, "Rotate", "phase.png",
	    mCB(this,uiWavelet,rotatePhase) );
    wvltbuts_[1]->setToolTip( "Rotate Phase" );
    wvltbuts_[1]->attach( rightOf, wvltbuts_[0] );

    initWaveletViewer();
    drawWavelet();
    wvltpropdlg_ = new WellTie::uiWaveletDispPropDlg( this, wvlt_ );
}


uiWavelet::~uiWavelet()
{
    const TypeSet<DataPack::ID> ids = viewer_->availablePacks();
    for ( int idx=ids.size()-1; idx>=0; idx-- )
	DPM( DataPackMgr::FlatID() ).release( ids[idx] );
    delete wvltpropdlg_; 
}


void uiWavelet::initWaveletViewer()
{
    FlatView::Appearance& app = viewer_->appearance();
    app.annot_.x1_.name_ = "Amplitude";
    app.annot_.x2_.name_ =  "Time";
    app.annot_.setAxesAnnot( false );
    app.setGeoDefaults( true );
    app.ddpars_.show( true, false );
    app.ddpars_.wva_.overlap_ = 0;
    app.ddpars_.wva_.clipperc_.start = app.ddpars_.wva_.clipperc_.stop = 0;
    app.ddpars_.wva_.left_ = Color::NoColor();
    app.ddpars_.wva_.right_ = Color::Black();
    app.ddpars_.wva_.mid_ = Color::Black();
    app.ddpars_.wva_.symmidvalue_ = mUdf(float);
    app.setDarkBG( false );
    viewer_->setInitialSize( uiSize(80,100) );
    viewer_->setStretch( 1, 2 );
}


void uiWavelet::viewWvltPropPushed( CallBacker* )
{
    if ( wvltpropdlg_ )
	wvltpropdlg_->go();
}


void uiWavelet::rotatePhase( CallBacker* cb )
{
    uiSeisWvltRotDlg dlg( this, wvlt_ );
    //dlg.phaserotating.notify( mCB(this,WellTie::uiWavelet,drawWavelet) );
    //dlg.phaserotating.remove( mCB(this,WellTie::uiWaveletView,updateViewer) );
}


void uiWavelet::drawWavelet()
{
    if ( !wvlt_) return;

    const int wvltsz = wvlt_->size();
    Array2DImpl<float>* fva2d = new Array2DImpl<float>( 1, wvltsz );
    memcpy( fva2d->getData(), wvlt_->samples(), wvltsz * sizeof(float) );
    
    FlatDataPack* dp = new FlatDataPack( "Wavelet", fva2d );
    DPM( DataPackMgr::FlatID() ).add( dp ); dp->setName( wvlt_->name() );
    viewer_->setPack( true, dp->id(), false );
    DPM( DataPackMgr::FlatID() ).addAndObtain( dp );
    
    StepInterval<double> posns; posns.setFrom( wvlt_->samplePositions() );
    if ( SI().zIsTime() ) posns.scale( SI().zFactor() );
    dp->posData().setRange( false, posns );
    
    viewer_->setPack( true, dp->id(), false );
    viewer_->handleChange( uiFlatViewer::All );
}




/*
class uiWvltSaveDlg : public uiDialog
{
public:

uiWvltSaveDlg( uiParent* p, const Wavelet* wvlt )
            : uiDialog(p,uiDialog::Setup("Save Estimated Wavelet",
	    "Specify wavelet name","107.4.4"))
	    , wvltctio_(*mMkCtxtIOObj(Wavelet))
	    , wvlt_(wvlt)				       
{
    wvltctio_.ctxt.forread = false;
    wvltfld_ = new uiIOObjSel( this, wvltctio_, "Output wavelet" );
}


bool acceptOK( CallBacker* )
{
    if ( !wvltfld_->commitInput() )
	mErrRet( "Please enter a name for the new Wavelet", return false );

    const int wvltsize = wvlt_->size();
    Wavelet wvlt( wvltfld_->getInput(), -wvltsize/2, SI().zStep() );
    wvlt.reSize( wvltsize );
    for( int idx=0; idx<wvltsize; idx++ )
	wvlt.samples()[idx] = wvlt_->samples()[idx];
    wvlt.put( wvltctio_.ioobj );
    return true;
}

    CtxtIOObj&  	wvltctio_;
    uiIOObjSel* 	wvltfld_;
    const Wavelet* 	wvlt_;
};

void uiWaveletView::saveWvltPushed( CallBacker* )
{
    WellTie::uiWvltSaveDlg dlg( this, wvlts_[1] );
    if ( !dlg.go() ) return;
}
*/

#define mPaddFac 3
uiWaveletDispPropDlg::uiWaveletDispPropDlg( uiParent* p, const Wavelet* wvlt )
	: uiDialog( p,Setup("Wavelet Properties","","107.4.3").modal(false))
	, wvlt_(wvlt)  
	, wvltctio_(*mMkCtxtIOObj(Wavelet))
	, wvltsz_(0)
	, fft_(new FFT())
{
    setCtrlStyle( LeaveOnly );

    if ( !wvlt ) return;
    wvltsz_ = wvlt->size();

    static const char* disppropnms[] = { "Amplitude", "Phase", "Frequency", 0 };
    
    uiFunctionDisplay::Setup fdsu; fdsu.border_.setRight( 0 );

    for ( int idx=0; idx<2; idx++ )
    {
	wvltdisps_ += new uiFunctionDisplay( this, fdsu );
	wvltdisps_[idx]->xAxis()->setName( "samples" );
	wvltdisps_[idx]->yAxis(false)->setName( disppropnms[idx] );
	proparrays_ += new  Array1DImpl<float>( wvltsz_ );
    }

    fdsu.fillbelow(true);	
    wvltdisps_ += new uiFunctionDisplay( this, fdsu );
    wvltdisps_[2]->xAxis()->setName( disppropnms[2] );
    wvltdisps_[2]->yAxis(false)->setName( "" );
    proparrays_ += new  Array1DImpl<float>( mPaddFac*wvltsz_ );

    for ( int idx=1; disppropnms[idx]; idx++ )
	wvltdisps_[idx]->attach( alignedBelow, wvltdisps_[idx-1] );

    setValArrays();
    setDispCurves();
}


uiWaveletDispPropDlg::~uiWaveletDispPropDlg()
{
    deepErase( proparrays_ );
    delete fft_;
}


#define mDoTransform(tf,isstraight,inp,outp,sz) \
{   \
            tf->setInputInfo(Array1DInfoImpl(sz));\
            tf->setDir(isstraight);\
            tf->init();\
            tf->transform(inp,outp);\
}
void uiWaveletDispPropDlg::setValArrays()
{
    memcpy(proparrays_[0]->getData(),wvlt_->samples(),wvltsz_*sizeof(float));

    HilbertTransform* hil = new HilbertTransform();

    Array1DImpl<float_complex> carr( wvltsz_ );

    hil->setCalcRange( 0, wvltsz_, 0 );
    mDoTransform( hil, true, *proparrays_[0], carr, wvltsz_ );
    delete hil;
    
    for ( int idx=0; idx<wvltsz_; idx++ )
    {
	float phase = 0;
	if ( carr.get(idx).real() )
	    phase = atan2( carr.get(idx).imag(), carr.get(idx).real() );
	proparrays_[1]->set( idx, phase );
    }

    const int zpadsz = mPaddFac*wvltsz_;
    Array1DImpl<float_complex> czeropaddedarr( zpadsz );
    Array1DImpl<float_complex> cfreqarr( zpadsz );

    for ( int idx=0; idx<zpadsz; idx++ )
	czeropaddedarr.set( idx, 0 );
    for ( int idx=0; idx<wvltsz_; idx++ )
	czeropaddedarr.set( idx+wvltsz_, carr.get(idx) );

    mDoTransform( fft_, true, czeropaddedarr, cfreqarr, zpadsz );
    for ( int idx=0; idx<zpadsz; idx++ )
    {
	const float imag = pow(cfreqarr.get(idx).imag(),2);
	const float real = pow(cfreqarr.get(idx).real(),2);
	proparrays_[2]->set( zpadsz-idx-1, imag+real ); 
    }
}


void uiWaveletDispPropDlg::setDispCurves()
{
    TypeSet<float> xvals;
    for ( int idx=0; idx<wvltsz_; idx++ )
	xvals += idx;
    for ( int idx=0; idx<proparrays_.size()-1; idx++ )
	wvltdisps_[idx]->setVals(xvals.arr(),proparrays_[idx]->arr(),wvltsz_);

    float maxfreq = fft_->getNyqvist( SI().zStep() );
    if ( SI().zIsTime() )
	maxfreq = mNINT( maxfreq );

    wvltdisps_[2]->setVals( Interval<float>( 0, maxfreq ), 
			    proparrays_[2]->arr(), 
			    mPaddFac*wvltsz_/2 );
}

}; //namespace WellTie
