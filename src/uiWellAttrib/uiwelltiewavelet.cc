/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Mar 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelltiewavelet.cc,v 1.16 2009-06-21 10:14:28 cvsbruno Exp $";

#include "uiwelltiewavelet.h"

#include "arrayndimpl.h"
#include "ctxtioobj.h"
#include "flatposdata.h"
#include "fft.h"
#include "hilberttransform.h"
#include "ioman.h"
#include "ioobj.h"
#include "math.h"
#include "survinfo.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "statruncalc.h"
#include "wavelet.h"
#include "welltiedata.h"
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


#define mErrRet(msg) \
{ uiMSG().error(msg); return false; }
uiWellTieWaveletView::uiWellTieWaveletView( uiParent* p,
					    const WellTieDataHolder* dh )
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


uiWellTieWaveletView::~uiWellTieWaveletView()
{
    if ( wvltinitdlg_ )
	delete wvltinitdlg_;
    if ( wvltestdlg_ )
	delete wvltestdlg_;
    for (int vwridx=viewer_.size()-1; vwridx>=0; vwridx--)
	viewer_.remove(vwridx);
}


void uiWellTieWaveletView::initWaveletViewer( int vwridx )
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


void uiWellTieWaveletView::createWaveletFields( uiGroup* grp )
{
    grp->setHSpacing(40);
    
    uiLabel* wvltlbl = new uiLabel( this, "Initial wavelet" );
    uiLabel* wvltestlbl = new uiLabel( this, "Estimated wavelet" );
    wvltlbl->attach( alignedAbove, viewer_[0] );
    wvltestlbl->attach( alignedAbove, viewer_[1] );
    wvltbuts_ += new uiPushButton( grp,  "Properties", 
	    mCB(this,uiWellTieWaveletView,viewInitWvltPropPushed),false);
    wvltbuts_ += new uiPushButton( grp, "Properties", 
	    mCB(this,uiWellTieWaveletView,viewEstWvltPropPushed),false);
    wvltbuts_ += new uiPushButton( grp, "Save Estimated Wavelet", 
	    mCB(this,uiWellTieWaveletView,saveWvltPushed),false);

    wvltbuts_[0]->attach( alignedBelow, viewer_[0] );
    wvltbuts_[1]->attach( alignedBelow, viewer_[1] );
    
    viewer_[0]->attach( alignedBelow, wvltlbl );
    viewer_[1]->attach( rightOf, viewer_[0] );
    viewer_[1]->attach( ensureRightOf, viewer_[0] );
    
    wvltbuts_[2]->attach( hCentered );
    wvltbuts_[2]->attach( ensureBelow, wvltbuts_[1] );
    wvltbuts_[2]->attach( ensureBelow, wvltbuts_[0] );
}


void uiWellTieWaveletView::initWavelets( )
{
    for ( int idx=wvlts_.size()-1; idx>=0; idx-- )
	delete wvlts_.remove(idx);

    IOObj* ioobj = IOM().get( MultiID(twtss_.wvltid_) );
    wvlts_ += Wavelet::get( ioobj);
    wvlts_ += new Wavelet(*dataholder_->getEstimatedWvlt());

    if ( !wvlts_[0] || !wvlts_[1] ) return;

    for ( int idx=0; idx<2; idx++ )
	drawWavelet( wvlts_[idx], idx );
    
    if ( !wvltinitdlg_ )
	wvltinitdlg_ = new uiWellTieWaveletDispDlg( this, wvlts_[0] );
    if ( !wvltestdlg_ )
	wvltestdlg_  = new uiWellTieWaveletDispDlg( this, wvlts_[1] );
}


void uiWellTieWaveletView::drawWavelet( const Wavelet* wvlt, int vwridx )
{
    BufferString tmp;
    const int wvltsz = wvlt->size();
    const float zfac = SI().zFactor();

    Array2DImpl<float>* fva2d = new Array2DImpl<float>( 1, wvltsz );
    FlatDataPack* dp = new FlatDataPack( "Wavelet", fva2d );
    DPM( DataPackMgr::FlatID() ).add( dp );
    viewer_[vwridx]->setPack( true, dp->id(), false );

    for ( int wvltidx=0; wvltidx< wvltsz; wvltidx++)
	 fva2d->set( 0, wvltidx,  wvlt->samples()[wvltidx] );
    dp->setName( wvlt->name() );
    
    DPM( DataPackMgr::FlatID() ).add( dp );
    StepInterval<double> posns; posns.setFrom( wvlt->samplePositions() );
    if ( SI().zIsTime() ) posns.scale( zfac );
    
    dp->posData().setRange( false, posns );
    Stats::RunCalc<float> rc( Stats::RunCalcSetup().require(Stats::Max) );
    rc.addValues( wvltsz, wvlt->samples() );
    
    viewer_[vwridx]->setPack( true, dp->id(), false );
    viewer_[vwridx]->handleChange( FlatView::Viewer::All );
}


void uiWellTieWaveletView::viewInitWvltPropPushed( CallBacker* )
{
    wvltinitdlg_->go();
}


void uiWellTieWaveletView::viewEstWvltPropPushed( CallBacker* )
{
    wvltestdlg_->go();
}


class uiWellTieWvltSaveDlg : public uiDialog
{
public:

uiWellTieWvltSaveDlg( uiParent* p, const Wavelet* wvlt )
            : uiDialog(p,uiDialog::Setup("Save Estimated Wavelet",
	    "Specify wavelet name",mTODOHelpID))
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


void uiWellTieWaveletView::saveWvltPushed( CallBacker* )
{
    uiWellTieWvltSaveDlg dlg( this, wvlts_[1] );
    if ( !dlg.go() ) return;
}



uiWellTieWaveletDispDlg::uiWellTieWaveletDispDlg( uiParent* p, 
						  const Wavelet* wvlt )
	: uiDialog( p,Setup("Wavelet Properties","",mTODOHelpID).modal(false))
	, wvlt_(wvlt)  
	, wvltctio_(*mMkCtxtIOObj(Wavelet))
	, wvltsz_(0)
{
    setCtrlStyle( LeaveOnly );

    if ( !wvlt ) return;
    wvltsz_ = wvlt->size();

    static const char* disppropnms[] = { "Amplitude", "Phase", "Frequency", 0 };

    uiFunctionDisplay::Setup fdsu; fdsu.border_.setRight( 0 );
    for ( int idx=0; disppropnms[idx]; idx++ )
    {
	if ( idx>1 ) fdsu.fillbelow(true);	
	wvltdisps_ += new uiFunctionDisplay( this, fdsu );
	wvltdisps_[idx]->xAxis()->setName( "samples" );
	wvltdisps_[idx]->yAxis(false)->setName( disppropnms[idx] );
	if  (idx )
	    wvltdisps_[idx]->attach( alignedBelow, wvltdisps_[idx-1] );
	wvltarrays_ += new  Array1DImpl<float>( wvltsz_ );
    }
    setValArrays();
    setDispCurves();
}


uiWellTieWaveletDispDlg::~uiWellTieWaveletDispDlg()
{
    deepErase( wvltarrays_ );
//    delete wvlttrc_;
}


#define mDoTransform(tf,isstraight,inp,outp,sz) \
{   \
            tf->setInputInfo(Array1DInfoImpl(sz));\
            tf->setDir(isstraight);\
            tf->init();\
            tf->transform(inp,outp);\
}
void uiWellTieWaveletDispDlg::setValArrays()
{
    memcpy(wvltarrays_[0]->getData(),wvlt_->samples(),wvltsz_*sizeof(float));

    FFT* fft = new FFT();
    HilbertTransform* hil = new HilbertTransform();
    Array1DImpl<float_complex> carr( wvltsz_ );
    Array1DImpl<float_complex> czeropaddedarr( 3*wvltsz_ );
    Array1DImpl<float_complex> cfreqarr( 3*wvltsz_ );
    
    hil->setCalcRange( 0, wvltsz_, 0 );

    mDoTransform( hil, true, *wvltarrays_[0], carr, wvltsz_ );
    delete hil;
    for ( int idx=0; idx<wvltsz_; idx++ )
    {
	float phase = 0;
	if ( carr.get(idx).real() )
	    phase = atan2( carr.get(idx).imag(), carr.get(idx).real() );
	wvltarrays_[1]->set( idx, phase );
    }

    for ( int idx=0; idx<3*wvltsz_; idx++ )
    {
	if ( idx>=wvltsz_ && idx<2*wvltsz_ )
	    czeropaddedarr.setValue( idx, carr.get( idx-wvltsz_ ) );
	else
	    czeropaddedarr.setValue( idx, 0 );
    }
		
    mDoTransform( fft, true, czeropaddedarr, cfreqarr, 3*wvltsz_ );
    delete fft;
    for ( int idx=0; idx<wvltsz_; idx++ )
    {
	float val = cfreqarr.get(idx+wvltsz_).real();
	wvltarrays_[2]->set( idx, val );
    }

}


void uiWellTieWaveletDispDlg::setDispCurves()
{
    TypeSet<float> xvals;
    for ( int idx=0; idx<wvltsz_; idx++ )
	xvals += idx;
    for ( int idx=0; idx<wvltarrays_.size(); idx++ )
	wvltdisps_[idx]->setVals(xvals.arr(),wvltarrays_[idx]->arr(),wvltsz_);
}
