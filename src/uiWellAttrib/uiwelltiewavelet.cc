/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Mar 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelltiewavelet.cc,v 1.6 2009-05-28 14:38:11 cvsbruno Exp $";

#include "uiwelltiewavelet.h"

#include "arrayndimpl.h"
#include "ctxtioobj.h"
#include "flatposdata.h"
#include "ioman.h"
#include "ioobj.h"
#include "survinfo.h"
#include "statruncalc.h"
#include "wavelet.h"
#include "welltiesetup.h"
#include "hilberttransform.h"
#include "fft.h"
#include "math.h"

#include "uiaxishandler.h"
#include "uifunctiondisplay.h"
#include "uigroup.h"
#include "uiflatviewer.h"
#include "uiioobjsel.h"
#include "uibutton.h"

#include <complex>

uiWellTieWaveletView::uiWellTieWaveletView( uiParent* p, WellTieSetup& twtss )
	: uiGroup(p)
	, twtss_(twtss)
	, wvltctio_(*mMkCtxtIOObj(Wavelet))
	, wvltChanged(this)
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
  //  for (int idx=wvlts_.size(); idx>=0; idx--)
//	delete wvlts_.remove(idx);
  //  delete wvltctio_.ioobj; delete &wvltctio_;
}


void uiWellTieWaveletView::initWaveletViewer( const int vwridx )
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
    grp->setHSpacing(80);
    wvltfld_ = new uiIOObjSel( grp, wvltctio_ );
    wvltfld_->setInput( twtss_.wvltid_ );
    wvltfld_->selectiondone.notify( mCB(this, uiWellTieWaveletView, wvtSel));

    wvltbuts_ += new uiPushButton( grp,  "Initial Wavelet", 
	    mCB(this,uiWellTieWaveletView,viewInitWvltPropPushed),false);
    wvltbuts_ += new uiPushButton( grp, "Estimated Wavelet", 
	    mCB(this,uiWellTieWaveletView,viewEstWvltPropPushed),false);
    wvltbuts_[0]->attach( alignedBelow, wvltfld_ );
    wvltbuts_[1]->attach( ensureBelow, wvltfld_ );
    wvltbuts_[1]->attach( alignedAbove, viewer_[1] );
    
    viewer_[0]->attach( alignedBelow, wvltbuts_[0] );
    viewer_[1]->attach( rightOf, viewer_[0] );
    viewer_[1]->attach( ensureRightOf, viewer_[0] );
}


void uiWellTieWaveletView::initWavelets( Wavelet* wvltest )
{
    IOObj* ioobj = IOM().get( MultiID(twtss_.wvltid_) );
    Wavelet* wvlt = new Wavelet(*Wavelet::get( ioobj));
    if ( !wvlt ) return;
    wvlts_ += wvlt;
    wvlts_ += wvltest;
    for ( int idx=0; idx<2; idx++ )
	drawWavelet( wvlts_[idx], idx );
}


void uiWellTieWaveletView::drawWavelet( Wavelet* wvlt, const int vwridx )
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


void uiWellTieWaveletView::wvtSel( CallBacker* )
{
    if ( twtss_.wvltid_ == wvltfld_->getKey() ) return;
    twtss_.wvltid_ =  wvltfld_->getKey();
    IOObj* ioobj = IOM().get( twtss_.wvltid_ );
    Wavelet* wvlt = Wavelet::get( ioobj );
    viewer_[0]->removePack( viewer_[0]->pack(true)->id() ); 
    drawWavelet( wvlt, 0 );
    wvltChanged.trigger();
}


void uiWellTieWaveletView::viewInitWvltPropPushed( CallBacker* )
{
    uiWellTieWaveletDispDlg wvltdlg( this, wvlts_[0] );
    wvltdlg.go();
}


void uiWellTieWaveletView::viewEstWvltPropPushed( CallBacker* )
{
    uiWellTieWaveletDispDlg wvltdlg( this, wvlts_[1] );
    wvltdlg.go();
}



uiWellTieWaveletDispDlg::uiWellTieWaveletDispDlg( uiParent* p, 
						  const Wavelet* wvlt )
	: uiDialog( p,Setup("Wavelet Properties","",mTODOHelpID))
	, wvltctio_(*mMkCtxtIOObj(Wavelet))
	, wvltsz_(0)
{
    setCtrlStyle( LeaveOnly );

    if ( !wvlt ) return;
    wvltsz_ = wvlt->size();

    static const char* disppropnms[] = { "Amplitude", "Frequency", "Phase", 0 };

    ObjectSet<uiGroup> wvltdispparamgrps;
    uiFunctionDisplay::Setup fdsu; fdsu.border_.setRight( 0 );

    for (int idx=0; disppropnms[idx]; idx++)
    {
	wvltdispparamgrps += new uiGroup( this, disppropnms[idx] );
	if ( idx )
	    wvltdispparamgrps[idx]->attach( ensureBelow,
					    wvltdispparamgrps[idx-1] );
	wvltdisps_ += new uiFunctionDisplay( wvltdispparamgrps[idx], fdsu );
	wvltvalsarr_ += new Array1DImpl<float>( wvltsz_ );
	wvltdisps_[idx]->xAxis()->setName( "samples" );
	wvltdisps_[idx]->yAxis(false)->setName( disppropnms[idx] );
    }

    memcpy(wvltvalsarr_[0]->getData(),wvlt->samples(),wvltsz_*sizeof(float));
    setFrequency();
    setPhase();
    setDispCurves();
}


uiWellTieWaveletDispDlg::~uiWellTieWaveletDispDlg()
{
    for (int idx=wvltvalsarr_.size(); idx>=0; idx--)
	delete wvltvalsarr_.remove(idx);
}


void uiWellTieWaveletDispDlg::setDispCurves()
{
    TypeSet<float> xvals;
    for (int idx=0; idx<wvltsz_; idx++)
	xvals += idx;

    for (int idx=0; idx<3; idx++)
	wvltdisps_[idx]->setVals( xvals.arr(),
				  wvltvalsarr_[idx]->getData(),
    				  wvltsz_ );
}


void uiWellTieWaveletDispDlg::setFrequency()
{
    Array1DImpl<float> vals( wvltsz_ );
    FFT fft;
    fft.setInputInfo( Array1DInfoImpl( wvltsz_) );
    fft.setDir(true);
    fft.init();
    fft.transform( *wvltvalsarr_[0], vals );

    for ( int idx=0; idx<wvltsz_; idx++ )
    wvltvalsarr_[1]->set( idx, vals.get(idx) );
}


void uiWellTieWaveletDispDlg::setPhase()
{
    Array1DImpl<float_complex> cvals( wvltsz_ );

    HilbertTransform hil;
    hil.setCalcRange( 0, wvltsz_, 0 );
    hil.setInputInfo( Array1DInfoImpl(wvltsz_) );
    hil.setDir( true );
    hil.init();
    hil.transform( *wvltvalsarr_[0], cvals );

    for ( int idx=0; idx<wvltsz_; idx++ )
    {
	float phase = 0;
	if ( cvals.get(idx).real() )
	phase =atan2( cvals.get(idx).imag(), cvals.get(idx).real() );
	wvltvalsarr_[2]->set( idx, phase );
    }
}


