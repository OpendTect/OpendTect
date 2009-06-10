/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Mar 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelltiewavelet.cc,v 1.10 2009-06-10 10:13:11 cvsbruno Exp $";

#include "uiwelltiewavelet.h"

#include "arrayndimpl.h"
#include "ctxtioobj.h"
#include "fft.h"
#include "flatposdata.h"
#include "hilberttransform.h"
#include "ioman.h"
#include "ioobj.h"
#include "math.h"
#include "survinfo.h"
#include "statruncalc.h"
#include "wavelet.h"
#include "welltiesetup.h"

#include "uiaxishandler.h"
#include "uibutton.h"
#include "uiflatviewer.h"
#include "uifunctiondisplay.h"
#include "uigroup.h"
#include "uiioobjsel.h"

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
    for ( int idx=wvlts_.size()-1; idx>=0; idx-- )
	delete wvlts_.remove(idx);

    IOObj* ioobj = IOM().get( MultiID(twtss_.wvltid_) );
    wvlts_ += Wavelet::get( ioobj);
    wvlts_ += wvltest;

    if ( !wvlts_[0] || !wvlts_[1] ) return;

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
    uiWellTieWaveletDispDlg* wvltinitdlg = 
	new uiWellTieWaveletDispDlg( this, wvlts_[0] );
    wvltinitdlg->go();
    delete wvltinitdlg;
}


void uiWellTieWaveletView::viewEstWvltPropPushed( CallBacker* )
{
    uiWellTieWaveletDispDlg* wvltestdlg = 
	new uiWellTieWaveletDispDlg( this, wvlts_[1] );
    wvltestdlg->go();
    delete wvltestdlg;
}



uiWellTieWaveletDispDlg::uiWellTieWaveletDispDlg( uiParent* p, 
						  const Wavelet* wvlt )
	: uiDialog( p,Setup("Wavelet Properties","",mTODOHelpID))
	, wvltctio_(*mMkCtxtIOObj(Wavelet))
	, wvltsz_(0)
	, fft_(0)	    
{
    setCtrlStyle( LeaveOnly );

    if ( !wvlt ) return;
    wvltsz_ = wvlt->size();

    static const char* disppropnms[] = { "Amplitude", "Frequency", 0 };

    ObjectSet<uiGroup> wvltdispparamgrps;
    uiFunctionDisplay::Setup fdsu; fdsu.border_.setRight( 0 );

    for ( int idx=0; idx<2; idx++ )
    {
	wvltdispparamgrps += new uiGroup( this, disppropnms[idx] );
	if ( idx )
	    wvltdispparamgrps[idx]->attach( alignedBelow,
					    wvltdispparamgrps[idx-1] );
	if ( idx>0 ) fdsu.fillbelow(true);	
	wvltdisps_ += new uiFunctionDisplay( wvltdispparamgrps[idx], fdsu );
	wvltvalsarr_ += new Array1DImpl<float>( wvltsz_ );
	wvltdisps_[idx]->xAxis()->setName( "samples" );
	wvltdisps_[idx]->yAxis(false)->setName( disppropnms[idx] );
    }

    memcpy(wvltvalsarr_[0]->getData(),wvlt->samples(),wvltsz_*sizeof(float));
    
    setDispCurves();
}


uiWellTieWaveletDispDlg::~uiWellTieWaveletDispDlg()
{
    for ( int idx=wvltvalsarr_.size()-1; idx>=0; idx-- )
	delete wvltvalsarr_.remove(idx);
    delete fft_;
}


void uiWellTieWaveletDispDlg::setDispCurves()
{
    TypeSet<float> xvals, freqvals;
    for ( int idx=0; idx<wvltsz_; idx++ )
	xvals += idx;
    for ( int idx=0; idx<wvltsz_/2; idx++ )
	freqvals += wvltvalsarr_[1]->get(idx+wvltsz_/2);
    for ( int idx=wvltsz_/2; idx<wvltsz_; idx++ )
	freqvals += wvltvalsarr_[1]->get(idx-wvltsz_/2);

    for ( int idx=0; idx<1; idx++ )
	wvltdisps_[idx]->setVals( xvals.arr(),
				  wvltvalsarr_[idx]->getData(),
    				  wvltsz_ );

    float maxfreq = fft_->getNyqvist( SI().zStep() );
    if ( SI().zIsTime() )
	maxfreq = mNINT( maxfreq );

    wvltdisps_[1]->setVals( Interval<float>( 0, maxfreq ), 
	    		    freqvals.arr(), 
			    wvltsz_ );
}



#define mDoTransform(tf,isstraight,inp,outp,sz) \
{   \
        tf->setInputInfo(Array1DInfoImpl(sz));\
        tf->setDir(isstraight);\
        tf->init();\
        tf->transform(inp,outp);\
}
void uiWellTieWaveletDispDlg::setFrequency()
{
    fft_ = new FFT();
    HilbertTransform* hil = new HilbertTransform();
    Array1DImpl<float_complex> cvals( wvltsz_ );
    Array1DImpl<float_complex> cfreqvals( wvltsz_ );

    hil->setCalcRange( 0, wvltsz_, 0 );
    
    mDoTransform( hil, true, *wvltvalsarr_[0], cvals, wvltsz_ );
    delete hil;
    mDoTransform( fft_, true, cvals, cfreqvals, wvltsz_ );
    
    for ( int idx=0; idx<wvltsz_; idx++ )
    {
	float phase = 0;
	if ( cvals.get(idx).real() )
	    phase = atan2( cvals.get(idx).imag(), cvals.get(idx).real() );

	wvltvalsarr_[1]->set( idx, abs(cfreqvals.get(idx)) );
    }
}


