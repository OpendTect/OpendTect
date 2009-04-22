/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Mar 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelltiewavelet.cc,v 1.2 2009-04-22 09:22:06 cvsbruno Exp $";

#include "uiwelltiewavelet.h"
#include "welltiesynthetics.h"

#include "arraynd.h"
#include "arrayndimpl.h"
#include "attribdesc.h"      
#include "attribdescset.h" 
#include "ctxtioobj.h"
#include "flatposdata.h"
#include "ioman.h"
#include "ioobj.h"
#include "survinfo.h"
#include "statruncalc.h"
#include "varlenarray.h"
#include "wavelet.h"
#include "welltiesetup.h"

#include "uiflatviewer.h"
#include "uiioobjsel.h"
#include "uitextedit.h"


uiWellTieWavelet::uiWellTieWavelet( uiParent* p, WellTieSetup& twtss)
	: uiGroup(p)
	, twtss_(twtss)
	, wvltctio_(*mMkCtxtIOObj(Wavelet))
	, wvltChanged(this)					 
{
    for ( int idx=0; idx<2; idx++ )
    {
	viewer_ += new uiFlatViewer( this, true );
	if ( idx>1 ) 
	    viewer_[idx]->attach( rightOf, viewer_[idx-1] );
	initWaveletViewer( idx );
    }
    createWaveletFields( this );
} 


uiWellTieWavelet::~uiWellTieWavelet()
{
  //  delete wvltctio_.ioobj; delete &wvltctio_;
}


void uiWellTieWavelet::initWaveletViewer( const int vwridx )
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


void uiWellTieWavelet::createWaveletFields( uiGroup* grp )
{
    wvltfld_ = new uiIOObjSel( grp, wvltctio_ );
    wvltfld_->setInput( twtss_.wvltid_ );
    wvltfld_->selectiondone.notify( mCB(this, uiWellTieWavelet, wvtSel));

    viewer_[0]->attach( alignedBelow, wvltfld_ );
    viewer_[1]->attach( ensureBelow, wvltfld_ );
    viewer_[1]->attach( ensureRightOf, wvltfld_ );
}


void uiWellTieWavelet::initWavelets( Wavelet* wvltest )
{
    IOObj* ioobj = IOM().get( MultiID(twtss_.wvltid_) );
    Wavelet* wvlt = Wavelet::get( ioobj );
    ObjectSet<Wavelet> wvlts;
    wvlts += wvlt;
    wvlts += wvlt;
    for ( int idx=0; idx<2; idx++ )
	drawWavelet( wvlts[idx], idx );
}


void uiWellTieWavelet::drawWavelet( Wavelet* wvlt, const int vwridx )
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


void uiWellTieWavelet::wvtSel( CallBacker* )
{
    if ( twtss_.wvltid_ == wvltfld_->getKey() ) return;
    twtss_.wvltid_ =  wvltfld_->getKey();
    IOObj* ioobj = IOM().get( twtss_.wvltid_ );
    Wavelet* wvlt = Wavelet::get( ioobj );
    viewer_[0]->removePack( viewer_[0]->pack(true)->id() ); 
    drawWavelet( wvlt, 0 );
    wvltChanged.trigger();
}

