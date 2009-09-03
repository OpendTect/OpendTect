/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Mar 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelltiewavelet.cc,v 1.26 2009-09-03 14:04:30 cvsbruno Exp $";

#include "uiwelltiewavelet.h"

#include "ctxtioobj.h"
#include "flatposdata.h"
#include "fft.h"
#include "hilberttransform.h"
#include "ioman.h"
#include "ioobj.h"
#include "survinfo.h"
#include "statruncalc.h"
#include "wavelet.h"
#include "welltiedata.h"
#include "welltiegeocalculator.h"
#include "welltiesetup.h"

#include "uiaxishandler.h"
#include "uibutton.h"
#include "uiflatviewer.h"
#include "uifunctiondisplay.h"
#include "uigroup.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uimsg.h"

#include <complex>

namespace WellTie
{

#define mErrRet(msg) \
{ uiMSG().error(msg); return false; }
uiWaveletView::uiWaveletView( uiParent* p, const WellTie::DataHolder* dh )
	: uiGroup(p)
	, dataholder_(dh)  
	, twtss_(dh->setup())
	, wvltctio_(*mMkCtxtIOObj(Wavelet))
    	, wvltinitdlg_(0)
    	, wvltestdlg_(0)
{
    for ( int idx=0; idx<2; idx++ )
    {
	viewer_ += new uiFlatViewer( this );
	initWaveletViewer( idx );
    }
    createWaveletFields( this );
} 


uiWaveletView::~uiWaveletView()
{
    for ( int vwridx=0; vwridx<2; vwridx++ )
    {
	const TypeSet<DataPack::ID> ids = viewer_[vwridx]->availablePacks();
	for ( int idx=ids.size()-1; idx>=0; idx-- )
	    DPM( DataPackMgr::FlatID() ).release( ids[idx] );
    }
    if ( wvltinitdlg_ )
	delete wvltinitdlg_;
    if ( wvltestdlg_ )
	delete wvltestdlg_;
}


void uiWaveletView::initWaveletViewer( int vwridx )
{
    FlatView::Appearance& app = viewer_[vwridx]->appearance();
    app.annot_.x1_.name_ = "Amplitude";
    app.annot_.x2_.name_ =  "Time";
    app.annot_.setAxesAnnot( false );
    app.setGeoDefaults( true );
    app.ddpars_.show( true, false );
    app.ddpars_.wva_.overlap_ = 0;
    app.ddpars_.wva_.clipperc_.start = app.ddpars_.wva_.clipperc_.stop = 0;
    app.ddpars_.wva_.left_ = Color( 250, 0, 0 );
    app.ddpars_.wva_.right_ = Color( 0, 0, 250 );
    app.ddpars_.wva_.mid_ = Color( 0, 0, 250 );
    app.ddpars_.wva_.symmidvalue_ = mUdf(float);
    app.setDarkBG( false );
    viewer_[vwridx]->setInitialSize( uiSize(80,100) );
    viewer_[vwridx]->setStretch( 1, 2 );
}


void uiWaveletView::createWaveletFields( uiGroup* grp )
{
    grp->setHSpacing( 40 );
    
    uiLabel* wvltlbl = new uiLabel( this, "Initial wavelet" );
    uiLabel* wvltestlbl = new uiLabel( this, "Estimated wavelet" );
    wvltlbl->attach( alignedAbove, viewer_[0] );
    wvltestlbl->attach( alignedAbove, viewer_[1] );
    wvltbuts_ += new uiPushButton( grp,  "Properties", 
	    mCB(this,uiWaveletView,viewInitWvltPropPushed),false);
    wvltbuts_ += new uiPushButton( grp, "Properties", 
	    mCB(this,uiWaveletView,viewEstWvltPropPushed),false);
    wvltbuts_ += new uiPushButton( grp, "Save Estimated Wavelet", 
	    mCB(this,uiWaveletView,saveWvltPushed),false);

    wvltbuts_[0]->attach( alignedBelow, viewer_[0] );
    wvltbuts_[1]->attach( alignedBelow, viewer_[1] );
    
    viewer_[0]->attach( alignedBelow, wvltlbl );
    viewer_[1]->attach( rightOf, viewer_[0] );
    viewer_[1]->attach( ensureRightOf, viewer_[0] );
    
    wvltbuts_[2]->attach( hCentered );
    wvltbuts_[2]->attach( ensureBelow, wvltbuts_[1] );
    wvltbuts_[2]->attach( ensureBelow, wvltbuts_[0] );
}


void uiWaveletView::initWavelets( )
{
    for ( int idx=wvlts_.size()-1; idx>=0; idx-- )
	delete wvlts_.remove(idx);

    IOObj* ioobj = IOM().get( MultiID(twtss_.wvltid_) );
    wvlts_ += Wavelet::get( ioobj);
    wvlts_ += new Wavelet(*dataholder_->getEstimatedWvlt());

    if ( !wvlts_[0] || !wvlts_[1] ) return;

    for ( int vwridx=0; vwridx<2; vwridx++ )
    {
	const TypeSet<DataPack::ID> ids = viewer_[vwridx]->availablePacks();
	for ( int idx=ids.size()-1; idx>=0; idx-- )
	    DPM( DataPackMgr::FlatID() ).release( ids[idx] );
    }

    for ( int idx=0; idx<2; idx++ )
	drawWavelet( wvlts_[idx], idx );
    
    delete wvltinitdlg_; wvltinitdlg_=0;
    wvltinitdlg_ = new WellTie::uiWaveletDispDlg( this, wvlts_[0], dataholder_);
    delete wvltestdlg_; wvltestdlg_=0;
    wvltestdlg_  = new WellTie::uiWaveletDispDlg( this, wvlts_[1], dataholder_);
}


void uiWaveletView::drawWavelet( const Wavelet* wvlt, int vwridx )
{
    const int wvltsz = wvlt->size();
    Array2DImpl<float>* fva2d = new Array2DImpl<float>( 1, wvltsz );
    memcpy( fva2d->getData(), wvlt->samples(), wvltsz * sizeof(float) );
    
    FlatDataPack* dp = new FlatDataPack( "Wavelet", fva2d );
    DPM( DataPackMgr::FlatID() ).add( dp ); dp->setName( wvlt->name() );
    viewer_[vwridx]->setPack( true, dp->id(), false );
    DPM( DataPackMgr::FlatID() ).addAndObtain( dp );
    
    StepInterval<double> posns; posns.setFrom( wvlt->samplePositions() );
    if ( SI().zIsTime() ) posns.scale( SI().zFactor() );
    dp->posData().setRange( false, posns );
    
    viewer_[vwridx]->setPack( true, dp->id(), false );
    viewer_[vwridx]->handleChange( uiFlatViewer::All );
}


void uiWaveletView::viewInitWvltPropPushed( CallBacker* )
{
    if ( wvltinitdlg_ )
	wvltinitdlg_->go();
}


void uiWaveletView::viewEstWvltPropPushed( CallBacker* )
{
    if ( wvltestdlg_ )
	wvltestdlg_->go();
}


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
	mErrRet( "Please enter a name for the new Wavelet" );

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


#define mPaddFac 3
uiWaveletDispDlg::uiWaveletDispDlg( uiParent* p, const Wavelet* wvlt,
				  const WellTie::DataHolder* dh )
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


uiWaveletDispDlg::~uiWaveletDispDlg()
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
void uiWaveletDispDlg::setValArrays()
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


void uiWaveletDispDlg::setDispCurves()
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
