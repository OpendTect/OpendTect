/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uistratsynthdisp.cc,v 1.100 2012-07-12 15:04:45 cvsbruno Exp $";

#include "uistratsynthdisp.h"
#include "uiseiswvltsel.h"
#include "uisynthtorealscale.h"
#include "uicombobox.h"
#include "uiflatviewer.h"
#include "uiflatviewmainwin.h"
#include "uimultiflatviewcontrol.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uiflatviewslicepos.h"
#include "uiraytrace1d.h"
#include "uispinbox.h"
#include "uiseparator.h"
#include "uitaskrunner.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"

#include "flatposdata.h"
#include "flatviewzoommgr.h"
#include "ioman.h"
#include "ptrman.h"
#include "prestackgather.h"
#include "survinfo.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "synthseis.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "velocitycalc.h"
#include "wavelet.h"


static const int cMarkerSize = 6;

mDefineInstanceCreatedNotifierAccess(uiStratSynthDisp)


uiStratSynthDisp::uiStratSynthDisp( uiParent* p, const Strat::LayerModel& lm )
    : uiGroup(p,"LayerModel synthetics display")
    , lm_(lm)  
    , d2tmodels_(0)	    
    , stratsynth_(*new StratSynth(lm))
    , dispeach_(1)	
    , wvltChanged(this)
    , zoomChanged(this)
    , modSelChanged(this)		       
    , layerPropSelNeeded(this)
    , longestaimdl_(0)
    , lasttool_(0)
    , raytrcpardlg_(0)
    , prestackwin_(0)		      
    , currentsynthetic_(0)
{
    uiTaskRunner* tr = new uiTaskRunner( this );
    stratsynth_.setTaskRunner( tr );

    topgrp_ = new uiGroup( this, "Top group" );
    topgrp_->setFrame( true );
    topgrp_->setStretch( 2, 0 );

    uiToolButton* layertb = new uiToolButton( topgrp_, "defraytraceprops", 
				    "Specify synthetic layers properties", 
				    mCB(this,uiStratSynthDisp,layerPropsPush) );

    uiToolButton* rttb = new uiToolButton( topgrp_, "raytrace", 
				    "Specify ray tracer parameters", 
				    mCB(this,uiStratSynthDisp,rayTrcParPush) );
    rttb->attach( rightOf, layertb );

    wvltfld_ = new uiSeisWaveletSel( topgrp_ );
    wvltfld_->newSelection.notify( mCB(this,uiStratSynthDisp,wvltChg) );
    wvltfld_->setFrame( false );
    wvltfld_->attach( rightOf, rttb, 10 );

    scalebut_ = new uiPushButton( topgrp_, "Scale", false );
    scalebut_->activated.notify( mCB(this,uiStratSynthDisp,scalePush) );
    scalebut_->attach( rightOf, wvltfld_ );

    uiGroup* dataselgrp = new uiGroup( this, "Data Selection" );
    dataselgrp->attach( rightBorder );
    dataselgrp->attach( ensureRightOf, topgrp_ );

    datalist_ = new uiLabeledComboBox( dataselgrp, "View ", "" );
    datalist_->setStretch( 0, 0 );
    datalist_->box()->selectionChanged.notify(
	    				mCB(this,uiStratSynthDisp,dataSetSel) );

    addasnewbut_ = new uiPushButton( dataselgrp, "Add as new", false);
    addasnewbut_->activated.notify(mCB(this,uiStratSynthDisp,addSynth2List));
    addasnewbut_->attach( rightOf, datalist_ );
    addasnewbut_->setSensitive( false );

    datagrp_ = new uiGroup( this, "DataSet group" );
    datagrp_->attach( ensureBelow, topgrp_ );
    datagrp_->attach( ensureBelow, dataselgrp );
    datagrp_->setFrame( true );
    datagrp_->setStretch( 2, 0 );

    levelsnapselfld_ = new uiLabeledComboBox( datagrp_, "Snap level" );
    levelsnapselfld_->attach( leftBorder );
    levelsnapselfld_->setStretch( 2, 0 );
    levelsnapselfld_->box()->selectionChanged.notify(
				mCB(this,uiStratSynthDisp,levelSnapChanged) );
    levelsnapselfld_->box()->addItems( VSEvent::TypeNames() );

    prestackgrp_ = new uiGroup( datagrp_, "Pre-Stack View Group" );
    prestackgrp_->attach( rightOf, levelsnapselfld_, 20 );

    stackbox_ = new uiCheckBox( prestackgrp_, "Stack" );
    stackbox_->activated.notify( mCB(this,uiStratSynthDisp,offsetChged ) );

    stackfld_ = new uiStackGrp( prestackgrp_ );
    stackfld_->attach( rightOf, stackbox_ );
    stackfld_->rangeChg.notify( mCB(this,uiStratSynthDisp,offsetChged ) );

    offsetposfld_ = new uiSynthSlicePos( prestackgrp_, "Offset" );
    offsetposfld_->positionChg.notify( mCB(this,uiStratSynthDisp,offsetChged) );
    offsetposfld_->attach( rightOf, stackbox_ );

    prestackbut_ = new uiToolButton( prestackgrp_, "nonmocorr64", 
				"View Offset Direction", 
				mCB(this,uiStratSynthDisp,viewPreStackPush) );
    prestackbut_->attach( rightOf, offsetposfld_);

    cleanSynthetics();

    vwr_ = new uiFlatViewer( this );
    vwr_->setInitialSize( uiSize(600,250) ); //TODO get hor sz from laymod disp
    vwr_->setStretch( 2, 2 );
    vwr_->attach( ensureBelow, datagrp_ );
    FlatView::Appearance& app = vwr_->appearance();
    app.setGeoDefaults( true );
    app.setDarkBG( false );
    app.annot_.title_.setEmpty();
    app.annot_.x1_.showAll( true );
    app.annot_.x2_.showAll( true );
    app.annot_.x2_.name_ = "TWT (s)";
    app.ddpars_.show( true, false );
    app.ddpars_.wva_.mappersetup_.symmidval_ =
	app.ddpars_.vd_.mappersetup_.symmidval_ = 0;

    uiFlatViewStdControl::Setup fvsu( this );
    fvsu.withedit(true).withthumbnail(false).withcoltabed(false).tba( 
	    						(int)uiToolBar::Right );
    control_ = new uiMultiFlatViewControl( *vwr_, fvsu );
    control_->zoomChanged.notify( mCB(this,uiStratSynthDisp,zoomChg) );

    topgrp_->setSensitive( false );
    datagrp_->setSensitive( false );
    offsetChged(0);

    mTriggerInstanceCreatedNotifier();
}


uiStratSynthDisp::~uiStratSynthDisp()
{
    delete &stratsynth_;
}


void uiStratSynthDisp::addViewerToControl( uiFlatViewer& vwr )
{
    if ( control_ )
	control_->addViewer( vwr );
}


const Strat::LayerModel& uiStratSynthDisp::layerModel() const
{
    return lm_;
}


void uiStratSynthDisp::layerPropsPush( CallBacker* )
{
    layerPropSelNeeded.trigger();
}


void uiStratSynthDisp::addTool( const uiToolButtonSetup& bsu )
{
    uiToolButton* tb = new uiToolButton( datagrp_, bsu );
    if ( lasttool_ )
	tb->attach( leftOf, lasttool_ );
    else
	tb->attach( rightBorder );

    tb->attach( ensureRightOf, prestackbut_ ); 

    lasttool_ = tb;
}


void uiStratSynthDisp::cleanSynthetics()
{
    stratsynth_.clearSynthetics();
    datalist_->box()->setEmpty();
    datalist_->box()->addItem( "Free view" );
    datalist_->setSensitive( false );
}


void uiStratSynthDisp::setDispEach( int de )
{
    dispeach_ = de;
}


void uiStratSynthDisp::setDispMrkrs( const char* lnm,
				     const TypeSet<float>& zvals, Color col )
{
    StratSynth::Level* lvl = new StratSynth::Level( lnm, zvals, col );
    stratsynth_.setLevel( lvl );
    levelSnapChanged(0);
}


void uiStratSynthDisp::setZDataRange( const Interval<double>& zrg, bool indpth )
{
    Interval<double> newzrg; newzrg.set( zrg.start, zrg.stop );
    if ( indpth && d2tmodels_ && !d2tmodels_->isEmpty() )
    {
	int mdlidx = longestaimdl_;
	if ( mdlidx >= d2tmodels_->size() )
	    mdlidx = d2tmodels_->size()-1;

	const TimeDepthModel& d2t = *(*d2tmodels_)[mdlidx];
	newzrg.start = d2t.getTime( (float)zrg.start );
	newzrg.stop = d2t.getTime( (float)zrg.stop );
    }
    const Interval<double> xrg = vwr_->getDataPackRange( true );
    vwr_->setSelDataRanges( xrg, newzrg ); 
    vwr_->handleChange( FlatView::Viewer::All );
}


void uiStratSynthDisp::levelSnapChanged( CallBacker* )
{
    const StratSynth::Level* lvl = stratsynth_.getLevel();
    if ( !lvl )  return;
    StratSynth::Level* edlvl = const_cast<StratSynth::Level*>( lvl );
    VSEvent::Type tp;
    VSEvent::parseEnumType( levelsnapselfld_->box()->text(), tp );
    edlvl->snapev_ = tp;
    drawLevel();
}


const char* uiStratSynthDisp::levelName()  const
{
    const StratSynth::Level* lvl = stratsynth_.getLevel();
    return lvl ? lvl->name() : 0;
}


void uiStratSynthDisp::drawLevel()
{
    while ( vwr_->nrAuxData() )
	delete vwr_->removeAuxData( 0 );

    const StratSynth::Level* lvl = stratsynth_.getLevel();
    if ( d2tmodels_ && !d2tmodels_->isEmpty() && lvl )
    {
	SeisTrcBuf& tbuf = const_cast<SeisTrcBuf&>( curTrcBuf() );
	FlatView::AuxData* auxd = vwr_->createAuxData("Level markers");
	stratsynth_.snapLevelTimes( tbuf, *d2tmodels_ );

	auxd->linestyle_.type_ = LineStyle::None;
	for ( int imdl=0; imdl<tbuf.size(); imdl ++ )
	{
	    const float tval = imdl < tbuf.size() ? tbuf.get(imdl)->info().pick 
						  : mUdf(float);

	    auxd->markerstyles_ += MarkerStyle2D( MarkerStyle2D::Target,
						  cMarkerSize, lvl->col_ );
	    auxd->poly_ += FlatView::Point( imdl+1, tval );
	}
	if ( auxd->isEmpty() )
	    delete auxd;
	else
	    vwr_->addAuxData( auxd );
    }

    vwr_->handleChange( FlatView::Viewer::Annot, true );
}


void uiStratSynthDisp::wvltChg( CallBacker* )
{
    doModelChange();
    wvltChanged.trigger();
}


void uiStratSynthDisp::scalePush( CallBacker* )
{
    haveUserScaleWavelet();
}


bool uiStratSynthDisp::haveUserScaleWavelet()
{
    uiMsgMainWinSetter mws( mainwin() );
    SeisTrcBuf& tbuf = const_cast<SeisTrcBuf&>( curTrcBuf() );
    if ( tbuf.isEmpty() )
    {
	uiMSG().error( "Please generate layer models first.\n"
		"The scaling tool compares the amplitudes at the selected\n"
		"Stratigraphic Level to real amplitudes along a horizon" );
	return false;
    }
    BufferString levelname; 
    if ( stratsynth_.getLevel() ) levelname = stratsynth_.getLevel()->name();
    if ( levelname.isEmpty() || matchString( "--", levelname) )
    {
	uiMSG().error( "Please select a Stratigraphic Level.\n"
		"The scaling tool compares the amplitudes there\n"
		"to real amplitudes along a horizon" );
	return false;
    }

    bool is2d = SI().has2D();
    if ( is2d && SI().has3D() )
    {
	int res = uiMSG().question( "Type of seismic data to use", "2D", "3D",
					"Cancel", "Specify geometry" );
	if ( res < 0 ) return false;
	is2d = res == 1;
    }

    bool rv = false;
    uiSynthToRealScale dlg( this, is2d, tbuf, wvltfld_->getID(), levelname );
    if ( dlg.go() )
    {
	MultiID mid( dlg.selWvltID() );
	if ( mid.isEmpty() )
	    pErrMsg( "Huh" );
	else
	{
	    rv = true;
	    wvltfld_->setInput( mid );
	}
	vwr_->handleChange( FlatView::Viewer::All );
    }
    return rv;
}


void uiStratSynthDisp::zoomChg( CallBacker* )
{
    zoomChanged.trigger();
}


const uiWorldRect& uiStratSynthDisp::curView( bool indpth ) const
{
    static uiWorldRect wr; wr = vwr_->curView();
    if ( indpth && d2tmodels_ && !d2tmodels_->isEmpty() )
    {
	int mdlidx = longestaimdl_;
	if ( mdlidx >= d2tmodels_->size() )
	    mdlidx = d2tmodels_->size()-1;

	const TimeDepthModel& d2t = *(*d2tmodels_)[mdlidx];
	wr.setTop( d2t.getDepth( (float)wr.top() ) );
	wr.setBottom( d2t.getDepth( (float)wr.bottom() ) );
    }
    return wr;
}


const SeisTrcBuf& uiStratSynthDisp::curTrcBuf() const
{
    const FlatDataPack* dp = vwr_->pack( true );
    mDynamicCastGet(const SeisTrcBufDataPack*,tbdp,dp)
    if ( !tbdp )
    {
	static SeisTrcBuf emptybuf( false );
	return emptybuf;
    }
    return tbdp->trcBuf();
}


#define mErrRet(s,act) \
{ uiMsgMainWinSetter mws( mainwin() ); if ( s ) uiMSG().error(s); act; }

void uiStratSynthDisp::modelChanged()
{
    cleanSynthetics();
    doModelChange();
}


void uiStratSynthDisp::displaySynthetic( const SyntheticData* sd )
{
    displayPostStackSynthetic( sd );
    displayPreStackSynthetic( sd );
}


void uiStratSynthDisp::displayPostStackSynthetic( const SyntheticData* sd )
{
    const bool hadpack = vwr_->pack( true ) || vwr_->pack( false ); 

    vwr_->clearAllPacks(); 
    vwr_->control()->zoomMgr().toStart();
    while ( vwr_->nrAuxData() )
	delete vwr_->removeAuxData( 0 );

    if ( !sd ) return;
    mDynamicCastGet(const SeisTrcBufDataPack*,stbp,sd->getPack( false ));
    const SeisTrcBuf* tbuf = stbp ? &stbp->trcBuf() : 0;
    if ( !tbuf ) return;

    SeisTrcBuf* disptbuf = new SeisTrcBuf( true );
    for ( int idx=0; idx<tbuf->size(); idx++ )
    {
	if ( idx%dispeach_ )
	    continue;
	disptbuf->add( new SeisTrc( *tbuf->get( idx ) ) );
    }

    SeisTrcBufDataPack* dp = new SeisTrcBufDataPack( disptbuf, Seis::Line, 
	    				SeisTrcInfo::TrcNr, stbp->category() );
    dp->setName( sd->name() );
    DPM( DataPackMgr::FlatID() ).add( dp );

    d2tmodels_ = &sd->d2tmodels_;
    for ( int idx=0; idx<d2tmodels_->size(); idx++ )
    {
	int maxaimodelsz =  0;
	if ( (*d2tmodels_)[idx]->size() > maxaimodelsz )
	    { maxaimodelsz = (*d2tmodels_)[idx]->size(); longestaimdl_ = idx; }
    }

    vwr_->setPack( true, dp->id(), false, !hadpack );
    vwr_->setPack( false, dp->id(), false, !hadpack );
    vwr_->setViewToBoundingBox();
}


void uiStratSynthDisp::displayPreStackSynthetic( const SyntheticData* sd )
{
    const int midx = prestackwin_ ? modelposfld_->getValue() : -1;
    CBCapsule<int> caps( midx, this );
    modSelChanged.trigger( &caps );

    if ( !prestackwin_ ) return;

    uiFlatViewer& vwr = prestackwin_->viewer();
    vwr.clearAllPacks();
    vwr.control()->zoomMgr().toStart();

    if ( !sd ) return;
    mDynamicCastGet(const PreStack::GatherSetDataPack*,gsetdp,sd->getPack(true))
    if ( !gsetdp ) return;

    PreStack::Gather* gdp = new PreStack::Gather(*gsetdp->getGathers()[midx-1]);
    DPM(DataPackMgr::FlatID()).add( gdp );

    vwr.appearance() = vwr_->appearance();

    while ( vwr.nrAuxData() )
	delete vwr.removeAuxData( 0 );

    vwr.setPack( false, gdp->id(), false ); 
    vwr.setPack( true, gdp->id(), false ); 
}


void uiStratSynthDisp::viewPreStackPush( CallBacker* )
{
    if ( prestackwin_ )
    {
	delete prestackwin_; 
	prestackwin_ = 0;
    }

    if ( currentsynthetic_ )
    {
	uiFlatViewMainWin::Setup su( "Pre-Stack view", false );
	prestackwin_ = new uiFlatViewMainWin( 0, su );

	uiFlatViewStdControl::Setup fvsu( prestackwin_ );
	fvsu.withthumbnail( false ).withcoltabed( false )
		.tba( (int)uiToolBar::Top );
	uiFlatViewStdControl* ctrl = new uiFlatViewStdControl( 
					prestackwin_->viewer(), fvsu );
	uiToolBar* tb = ctrl->toolBar();
	modelposfld_ = new uiSynthSlicePos( tb, "Model" );
	tb->addObject( modelposfld_->mainObject() );
	modelposfld_->positionChg.notify( 
		mCB(this,uiStratSynthDisp,modelPosChged) );
	StepInterval<float> ls(  1, layerModel().size(), 1 );
	modelposfld_->setLimitSampling( ls );

	prestackwin_->setInitialSize( 300, 500 );
	prestackwin_->start();
    }
    displayPreStackSynthetic( currentsynthetic_ );
}


void uiStratSynthDisp::doModelChange()
{
    MouseCursorChanger mcs( MouseCursor::Busy );

    stratsynth_.setWavelet( wvltfld_->getWavelet() );
    d2tmodels_ = 0;

    const int seldataidx = datalist_->box()->currentItem(); 
    currentsynthetic_ = stratsynth_.getSynthetic( seldataidx );

    topgrp_->setSensitive( seldataidx == 0 );
    datagrp_->setSensitive( currentsynthetic_ );
    addasnewbut_->setSensitive( currentsynthetic_ && seldataidx == 0 );

    if ( currentsynthetic_ )
    {
	StepInterval<float> limits( currentsynthetic_->offsetRange() );
	limits.step = 100;
	offsetposfld_->setLimitSampling( limits );
	stackfld_->setLimitRange( limits );
	const bool hasoffset = limits.width() > 0;
	prestackgrp_->setSensitive( hasoffset );
	currentsynthetic_->setPostStack( limits.start );
    }
    if ( stratsynth_.errMsg() )
	mErrRet( stratsynth_.errMsg(), return )

    displaySynthetic( currentsynthetic_ );
}


mClass uiAddNewSynthDlg : public uiDialog
{
public:
    uiAddNewSynthDlg( uiParent* p, const char* wvlt, const BufferStringSet& nms)
	: uiDialog( this, uiDialog::Setup( "Synthetic Name", 
		    mNoDlgTitle, mNoHelpID) )
	, nms_(nms)					     
    {
	BufferString wvtbasedname( "Synthetic" );
	wvtbasedname += "_"; 
	wvtbasedname += wvlt; 
	namefld_ = new uiGenInput( this, "Name" );
	namefld_->setText( wvtbasedname );
    }

    bool acceptOK(CallBacker*)
    {
	const char* nm = getSynthName(); 
	if ( !nm )
	    mErrRet("Please specify a valid name",return false);
	if ( nms_.isPresent( nm ) )
	    mErrRet("Name is already present, please specify another name",
		    return false);

	return true;
    }

    const char* getSynthName()
    { return namefld_->text(); }

protected:
    const BufferStringSet& nms_;
    uiGenInput*	 namefld_;
};


void uiStratSynthDisp::addSynth2List( CallBacker* )
{
    if ( !currentsynthetic_ ) 
	return;

    BufferStringSet synthnms; 
    for ( int idx=0; idx<datalist_->box()->size(); idx++ )
	synthnms.add( datalist_->box()->textOfItem( idx ) );

    uiAddNewSynthDlg dlg( this, wvltfld_->getName(), synthnms );
    if ( !dlg.go() )
	return;

    const char* nm = dlg.getSynthName();
    currentsynthetic_->setName( nm );
    stratsynth_.addSynthetics();
    
    datalist_->box()->addItem( nm );
    datalist_->setSensitive( stratsynth_.synthetics().size() > 1 );
}


void uiStratSynthDisp::rayTrcParPush( CallBacker* )
{
    if ( !raytrcpardlg_ )
	raytrcpardlg_ = new uiRayTrcParamsDlg( this, stratsynth_.rayPars() );

    raytrcpardlg_->go();
    raytrcpardlg_->button( uiDialog::OK )->activated.notify(
			mCB(this,uiStratSynthDisp,rayTrcParChged) );
}


void uiStratSynthDisp::offsetChged( CallBacker* )
{
    const bool dostack = stackbox_->isChecked();
    offsetposfld_->display( !dostack );
    stackfld_->display( dostack );

    if ( !currentsynthetic_ ) return;

    currentsynthetic_->setPostStack( offsetposfld_->getValue(), 
			    dostack ? &stackfld_->getRange() : 0 );

    displayPostStackSynthetic( currentsynthetic_ );
}


void uiStratSynthDisp::modelPosChged( CallBacker* )
{
    displayPreStackSynthetic( currentsynthetic_ );
}


const SeisTrcBuf& uiStratSynthDisp::postStackTraces() const
{
    static SeisTrcBuf emptytb( true );
    if ( !currentsynthetic_ ) return emptytb;

    const DataPack* dp = currentsynthetic_->getPack( false );
    mDynamicCastGet(const SeisTrcBufDataPack*,stbp,dp);
    if ( !stbp ) return emptytb;

    if ( d2tmodels_ && !d2tmodels_->isEmpty() )
    {
	SeisTrcBuf& tbuf = const_cast<SeisTrcBuf&>( stbp->trcBuf() );
	stratsynth_.snapLevelTimes( tbuf, *d2tmodels_ );
    }
    return stbp->trcBuf();
}


const ObjectSet<const TimeDepthModel>* uiStratSynthDisp::d2TModels() const
{ return d2tmodels_; }


void uiStratSynthDisp::rayTrcParChged( CallBacker* )
{
    doModelChange();
}


void uiStratSynthDisp::dataSetSel( CallBacker* )
{
    doModelChange();
}


const ObjectSet<SyntheticData>& uiStratSynthDisp::getSynthetics() const
{
    return stratsynth_.synthetics();
}


const Wavelet* uiStratSynthDisp::getWavelet() const
{
    return stratsynth_.wavelet();
}


const MultiID& uiStratSynthDisp::waveletID() const
{
    return wvltfld_->getID();
}


void uiStratSynthDisp::genSyntheticsFor( const Strat::LayerModel& lm , 
					SeisTrcBuf& seisbuf) 
{
    if ( !stratsynth_.generate( lm, seisbuf ) && stratsynth_.errMsg() )
	mErrRet( stratsynth_.errMsg(), return )
}


uiSynthSlicePos::uiSynthSlicePos( uiParent* p, const char* lbltxt )
    : uiGroup( p, "Slice Pos" )
    , positionChg(this)  
{
    label_ = new uiLabel( this, lbltxt );
    sliceposbox_ = new uiSpinBox( this, 0, "Slice position" );
    sliceposbox_->valueChanging.notify( mCB(this,uiSynthSlicePos,slicePosChg));
    sliceposbox_->valueChanged.notify( mCB(this,uiSynthSlicePos,slicePosChg));
    sliceposbox_->attach( rightOf, label_ );

    uiLabel* steplabel = new uiLabel( this, "Step" );
    steplabel->attach( rightOf, sliceposbox_ );

    slicestepbox_ = new uiSpinBox( this, 0, "Slice step" );
    slicestepbox_->attach( rightOf, steplabel );

    prevbut_ = new uiToolButton( this, "prevpos", "Previous position",
				mCB(this,uiSynthSlicePos,prevCB) );
    prevbut_->attach( rightOf, slicestepbox_ );
    nextbut_ = new uiToolButton( this, "nextpos", "Next position",
				 mCB(this,uiSynthSlicePos,nextCB) );
    nextbut_->attach( rightOf, prevbut_ );
}


void uiSynthSlicePos::slicePosChg( CallBacker* )
{
    positionChg.trigger();
}


void uiSynthSlicePos::prevCB( CallBacker* )
{
    uiSpinBox* posbox = sliceposbox_;
    uiSpinBox* stepbox = slicestepbox_;
    posbox->setValue( posbox->getValue()-stepbox->getValue() );
}


void uiSynthSlicePos::nextCB( CallBacker* )
{
    uiSpinBox* posbox = sliceposbox_;
    uiSpinBox* stepbox = slicestepbox_;
    posbox->setValue( posbox->getValue()+stepbox->getValue() );
}


void uiSynthSlicePos::setLimitSampling( StepInterval<float> lms )
{
    limitsampling_ = lms;
    sliceposbox_->setInterval( lms.start, lms.stop );
    slicestepbox_->setValue( lms.step );
}


int uiSynthSlicePos::getValue() const
{
    return sliceposbox_->getValue();
}


uiStackGrp::uiStackGrp( uiParent* p )
    : uiGroup( p, "Stack group" )
    , rangeChg(this)  
{
    BufferString olb = "offset range";
    offsetfld_ = new uiGenInput( this, olb , IntInpIntervalSpec() );
    offsetfld_->valuechanged.notify( mCB(this,uiStackGrp,valChgCB) );
}


void uiStackGrp::valChgCB( CallBacker* )
{
    offsetrg_.start = offsetfld_->getIInterval().start;
    offsetrg_.stop = offsetfld_->getIInterval().stop;
    offsetrg_.limitTo( limitrg_ );
    rangeChg.trigger();
}


const Interval<float>& uiStackGrp::getRange() const
{
   return offsetrg_; 
}


void uiStackGrp::setLimitRange( Interval<float> rg )
{ 
    offsetfld_->setValue( rg );
    limitrg_ = rg; 
    offsetrg_ = rg;
}




uiRayTrcParamsDlg::uiRayTrcParamsDlg( uiParent* p, IOPar& par ) 
    : uiDialog(p,uiDialog::Setup(
		"Specify ray tracer parameters",mNoDlgTitle,
		"103.4.4").modal(false))
    , raypars_(par)
{
    setCtrlStyle( DoAndStay );

    uiRayTracer1D::Setup rsu; rsu.dooffsets_ = true;
    rtsel_ = new uiRayTracerSel( this, rsu );
    rtsel_->usePar( raypars_ ); 

    uiSeparator* sep = new uiSeparator( this, "NMO corr separator" );
    sep->attach( stretchedBelow, rtsel_ );

    nmobox_ = new uiCheckBox( this, "NMO corrections" );
    nmobox_->setChecked( true );
    nmobox_->attach( centeredBelow, rtsel_ );
    nmobox_->attach( ensureBelow, sep );
}


bool uiRayTrcParamsDlg::acceptOK( CallBacker* )
{
    raypars_.setEmpty();
    rtsel_->fillPar( raypars_ );
    raypars_.setYN( Seis::SynthGenBase::sKeyNMO(), nmobox_->isChecked() );

    return false;
}


