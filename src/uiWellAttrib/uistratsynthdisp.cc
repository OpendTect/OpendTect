/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratsynthdisp.cc,v 1.98 2012/09/14 16:58:03 cvsbruno Exp $";

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
#include "propertyref.h"
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
    , dispflattened_(false)
    , selectedtrace_(-1)	
    , selectedtraceaux_(0)
    , levelaux_(0)
    , wvltChanged(this)
    , zoomChanged(this)
    , modSelChanged(this)		       
    , layerPropSelNeeded(this)
    , longestaimdl_(0)
    , lasttool_(0)
    , synthgendlg_(0)
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

    wvltfld_ = new uiSeisWaveletSel( topgrp_ );
    wvltfld_->newSelection.notify( mCB(this,uiStratSynthDisp,wvltChg) );
    wvltfld_->setFrame( false );
    wvltfld_->attach( rightOf, layertb, 10 );
    stratsynth_.setWavelet( wvltfld_->getWavelet() );

    scalebut_ = new uiPushButton( topgrp_, "Scale", false );
    scalebut_->activated.notify( mCB(this,uiStratSynthDisp,scalePush) );
    scalebut_->attach( rightOf, wvltfld_ );

    uiGroup* dataselgrp = new uiGroup( this, "Data Selection" );
    dataselgrp->attach( rightBorder );
    dataselgrp->attach( ensureRightOf, topgrp_ );

    datalist_ = new uiLabeledComboBox( dataselgrp, "View ", "" );
    datalist_->box()->selectionChanged.notify(
	    				mCB(this,uiStratSynthDisp,dataSetSel) );
    datalist_->box()->setHSzPol( uiObject::Wide );

    addeditbut_ = new uiToolButton( dataselgrp, "edit", 
	    			"Add/Edit Synthetic DataSet",
				mCB(this,uiStratSynthDisp,addEditSynth) );
    addeditbut_->attach( rightOf, datalist_ );

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

    offsetposfld_ = new uiSynthSlicePos( prestackgrp_, "Offset" );
    offsetposfld_->positionChg.notify( mCB(this,uiStratSynthDisp,offsetChged) );

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
}


void uiStratSynthDisp::updateSyntheticList()
{
    datalist_->box()->setEmpty();
    for ( int idx=0; idx<stratsynth_.nrSynthetics(); idx ++)
	datalist_->box()->addItem( stratsynth_.getSyntheticByIdx(idx)->name() );
}


void uiStratSynthDisp::setDispEach( int de )
{
    dispeach_ = de;
    displayPostStackDirSynthetic( currentsynthetic_ );
}


void uiStratSynthDisp::setSelectedTrace( int st )
{
    selectedtrace_ = st;

    delete vwr_->removeAuxData( selectedtraceaux_ );
    selectedtraceaux_ = 0;

    const StepInterval<double> xrg = vwr_->getDataPackRange(true);
    const StepInterval<double> zrg = vwr_->getDataPackRange(false);

    const float offset = xrg.start;
    if ( !xrg.includes( selectedtrace_ + offset, true ) )
	return;

    selectedtraceaux_ = vwr_->createAuxData( "Selected trace" );
    selectedtraceaux_->zvalue_ = 2;
    vwr_->addAuxData( selectedtraceaux_ );

    const double ptx = selectedtrace_ + offset;
    const double ptz1 = zrg.start;
    const double ptz2 = zrg.stop;

    Geom::Point2D<double> pt1 = Geom::Point2D<double>( ptx, ptz1 );
    Geom::Point2D<double> pt2 = Geom::Point2D<double>( ptx, ptz2 );

    selectedtraceaux_->poly_ += pt1;
    selectedtraceaux_->poly_ += pt2;
    selectedtraceaux_->linestyle_ =
    LineStyle( LineStyle::Dot, 2, Color::DgbColor() );

    vwr_->handleChange( FlatView::Viewer::Annot, true );
}


void uiStratSynthDisp::setDispMrkrs( const char* lnm,
				     const TypeSet<float>& zvals, Color col,
				     bool dispflattened	)
{
    StratSynth::Level* lvl = new StratSynth::Level( lnm, zvals, col );
    stratsynth_.setLevel( lvl );
    levelSnapChanged(0);

    const bool modelchange = dispflattened_ != dispflattened;
    dispflattened_ = dispflattened;
    if ( modelchange )
	doModelChange();
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
    delete vwr_->removeAuxData( levelaux_ );

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
	{
	    vwr_->addAuxData( auxd );
	    levelaux_ = auxd;
	}
    }

    vwr_->handleChange( FlatView::Viewer::Annot, true );
}


void uiStratSynthDisp::wvltChg( CallBacker* )
{
    syntheticDataParChged(0);
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
    displayPostStackDirSynthetic( sd );
    displayPreStackDirSynthetic( sd );
}


void uiStratSynthDisp::displayPostStackDirSynthetic( const SyntheticData* sd )
{
    const bool hadpack = vwr_->pack( true ) || vwr_->pack( false ); 

    vwr_->clearAllPacks(); 
    vwr_->removeAllAuxData( true );

    if ( !sd ) return;

    mDynamicCastGet(const PreStackSyntheticData*,presd,sd);
    mDynamicCastGet(const PostStackSyntheticData*,postsd,sd);

    const float offset = offsetposfld_->getValue();
    const SeisTrcBuf* tbuf = presd ? presd->getTrcBuf( offset, 0 ) 
				   : &postsd->postStackPack().trcBuf();

    if ( !tbuf ) return;

    SeisTrcBuf* disptbuf = new SeisTrcBuf( true );
    tbuf->copyInto( *disptbuf );

    if ( dispeach_ > 1 )
	stratsynth_.decimateTraces( *disptbuf, dispeach_ );
    if ( dispflattened_ )
    {
	stratsynth_.snapLevelTimes( *disptbuf, sd->d2tmodels_ );
	stratsynth_.flattenTraces( *disptbuf );
    }

    SeisTrcBufDataPack* dp = new SeisTrcBufDataPack( disptbuf, Seis::Line, 
				    SeisTrcInfo::TrcNr, "Forward Modeling" );
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
}


void uiStratSynthDisp::displayPreStackDirSynthetic( const SyntheticData* sd )
{
    const int midx = prestackwin_ ? modelposfld_->getValue() : -1;
    CBCapsule<int> caps( midx, this );
    modSelChanged.trigger( &caps );

    if ( !prestackwin_ ) return;

    uiFlatViewer& vwr = prestackwin_->viewer();
    vwr.clearAllPacks();
    vwr.control()->zoomMgr().toStart();

    if ( !sd ) return;
    mDynamicCastGet(const PreStack::GatherSetDataPack*,gsetdp,&sd->getPack())
    if ( !gsetdp ) return;

    PreStack::Gather* gdp = new PreStack::Gather(*gsetdp->getGathers()[midx-1]);
    DPM(DataPackMgr::FlatID()).add( gdp );

    vwr.removeAllAuxData( true );
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

    if ( currentsynthetic_ && currentsynthetic_->isPS() )
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
    displayPreStackDirSynthetic( currentsynthetic_ );
}


void uiStratSynthDisp::setCurrentSynthetic()
{
    SyntheticData* sd = 0;
    if ( stratsynth_.nrSynthetics() == 0 )
    {
	sd = stratsynth_.addDefaultSynthetic();
	updateSyntheticList();
    }
    else
	sd = stratsynth_.getSyntheticByIdx( datalist_->box()->currentItem() );

    currentsynthetic_ = sd;

    if ( synthgendlg_)
	synthgendlg_->putToScreen();
}


void uiStratSynthDisp::doModelChange()
{
    MouseCursorChanger mcs( MouseCursor::Busy );

    d2tmodels_ = 0;

    setCurrentSynthetic();

    mDynamicCastGet(const PreStackSyntheticData*,pssd,currentsynthetic_);
    if ( pssd )
    {
	StepInterval<float> limits( pssd->offsetRange() );
	limits.step = 100;
	offsetposfld_->setLimitSampling( limits );
    }
    prestackgrp_->setSensitive( pssd && pssd->hasOffset() );

    if ( stratsynth_.errMsg() )
	mErrRet( stratsynth_.errMsg(), return )

    topgrp_->setSensitive( currentsynthetic_ );
    datagrp_->setSensitive( currentsynthetic_ );

    displaySynthetic( currentsynthetic_ );
    drawLevel();
}


void uiStratSynthDisp::addEditSynth( CallBacker* )
{
    if ( !synthgendlg_ )
	synthgendlg_ = new uiSynthGenDlg( this, stratsynth_.genParams() );

    synthgendlg_->go();
    synthgendlg_->button( uiDialog::OK )->activated.notify(
			mCB(this,uiStratSynthDisp,syntheticDataParChged) );
    synthgendlg_->genNewReq.notify(
			mCB(this,uiStratSynthDisp,genNewSynthetic) );
}


void uiStratSynthDisp::offsetChged( CallBacker* )
{
    displayPostStackDirSynthetic( currentsynthetic_ );
}


void uiStratSynthDisp::modelPosChged( CallBacker* )
{
    displayPreStackDirSynthetic( currentsynthetic_ );
}


const SeisTrcBuf& uiStratSynthDisp::postStackTraces(const PropertyRef* pr) const
{
    SyntheticData* sd = pr ? stratsynth_.getSynthetic(*pr) :currentsynthetic_;

    static SeisTrcBuf emptytb( true );

    if ( !sd || sd->isPS() ) return emptytb;

    const DataPack& dp = sd->getPack();
    mDynamicCastGet(const SeisTrcBufDataPack*,stbp,&dp);
    if ( !stbp ) return emptytb;

    if ( !sd->d2tmodels_.isEmpty() )
    {
	SeisTrcBuf& tbuf = const_cast<SeisTrcBuf&>( stbp->trcBuf() );
	stratsynth_.snapLevelTimes( tbuf, sd->d2tmodels_ );
    }
    return stbp->trcBuf();
}


const PropertyRefSelection& uiStratSynthDisp::modelPropertyRefs() const
{ return layerModel().propertyRefs(); }


const ObjectSet<const TimeDepthModel>* uiStratSynthDisp::d2TModels() const
{ return d2tmodels_; }


void uiStratSynthDisp::syntheticDataParChged( CallBacker* )
{
    if ( !currentsynthetic_ ) return;

    stratsynth_.setWavelet( wvltfld_->getWavelet() );
    stratsynth_.replaceSynthetic( currentsynthetic_->id_ );

    doModelChange();
}


void uiStratSynthDisp::dataSetSel( CallBacker* )
{
    doModelChange();

    if ( !currentsynthetic_ )
	return;

    currentsynthetic_->fillGenParams( stratsynth_.genParams() );
    wvltfld_->setInput( stratsynth_.genParams().wvltnm_ );   
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


#define mChkPresent( nm )\
{\
if ( datalist_->box()->isPresent(nm) );\
    mErrRet( "Name already exists, please select another name", return );\
}

class uiAddNewSynthDlg : public uiDialog
{
public:
    uiAddNewSynthDlg( uiParent* p, SynthGenParams& sgp )
    : uiDialog( this, uiDialog::Setup("Synthetic Name",mNoDlgTitle,mNoHelpID) )
    , sgp_(sgp)
    {
	namefld_ = new uiGenInput( this, "Name" );
	namefld_->setText( sgp_.genName() );
    }

    bool acceptOK(CallBacker*)
    {
	const char* nm = namefld_->text(); 
	if ( !nm )
	    mErrRet("Please specify a valid name",return false);
	sgp_.name_ = nm;

	return true;
    }

protected:
    SynthGenParams&	sgp_;
    uiGenInput*	 	namefld_;
};


void uiStratSynthDisp::genNewSynthetic( CallBacker* )
{
    if ( !synthgendlg_ ) 
	return;

    uiAddNewSynthDlg dlg( synthgendlg_, stratsynth_.genParams() );
    if ( !dlg.go() )
	return;

    SyntheticData* sd = stratsynth_.addSynthetic();

    if ( sd )
    {
	updateSyntheticList();
	datalist_->box()->setCurrentItem( datalist_->box()->size()-1 );
	synthgendlg_->putToScreen();
    }
}


SyntheticData* uiStratSynthDisp::getCurrentSyntheticData() const        
{                  
    return currentsynthetic_;
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



uiSynthGenDlg::uiSynthGenDlg( uiParent* p, SynthGenParams& gp) 
    : uiDialog(p,uiDialog::Setup("Specify Synthetic Parameters",mNoDlgTitle,
				 "103.4.4").modal(false))
    , sd_(gp)
    , genNewReq(this)
{
    setCtrlStyle( DoAndStay );

    setOkText( "Apply" );

    CallBack cb( mCB(this,uiSynthGenDlg,typeChg) );
    typefld_ = new uiGenInput( this, "Type",
			BoolInpSpec(true,"Post-Stack","Pre-Stack") );
    typefld_->valuechanged.notify( cb );

    stackbox_ = new uiCheckBox( this, "Stack from Pre-Stack" );
    stackbox_->activated.notify( cb );
    stackbox_->attach( rightOf, typefld_ );

    uiSeparator* sep = new uiSeparator( this, "Name separator" );
    sep->attach( stretchedBelow, typefld_ );

    uiRayTracer1D::Setup rsu; rsu.dooffsets_ = true;
    rtsel_ = new uiRayTracerSel( this, rsu );
    rtsel_->usePar( sd_.raypars_ ); 
    rtsel_->attach( centeredBelow, typefld_ );
    rtsel_->attach( ensureBelow, sep );

    nmobox_ = new uiCheckBox( this, "Apply NMO corrections" );
    nmobox_->setChecked( true );
    nmobox_->attach( alignedBelow, rtsel_ );

    uiSeparator* sep2 = new uiSeparator( this, "action separator" );
    sep2->attach( stretchedBelow, nmobox_ );

    namefld_ = new uiGenInput( this, "Name" );
    namefld_ ->attach( centeredBelow, rtsel_ );
    namefld_ ->attach( ensureBelow, sep2 );

    gennewbut_ = new uiPushButton( this, "Add New Synthetic", true );
    gennewbut_->activated.notify( mCB(this,uiSynthGenDlg,genNewCB) );
    gennewbut_->attach( rightOf, namefld_ );

    typeChg(0);
    putToScreen();
}


void uiSynthGenDlg::typeChg( CallBacker* )
{
    const bool isps = !typefld_->getBoolValue();
    stackbox_->display( !isps );
    nmobox_->display( isps );
    const bool needranges = isps || stackbox_->isChecked();
    rtsel_->current()->displayOffsetFlds( needranges );
    rtsel_->current()->setOffsetRange( uiRayTracer1D::Setup().offsetrg_ );
}


void uiSynthGenDlg::putToScreen()
{
    namefld_->setText( sd_.name_ );
    typefld_->setValue( !sd_.isps_ ); 
    rtsel_->usePar( sd_.raypars_ );
}


void uiSynthGenDlg::getFromScreen() 
{
    sd_.raypars_.setEmpty();
    rtsel_->fillPar( sd_.raypars_ );
    const bool isps = !typefld_->getBoolValue();
    const bool dostack = stackbox_->isChecked();
    if ( !isps && !dostack )
    {
	TypeSet<float> emptyset; emptyset += 0;
	sd_.raypars_.set( RayTracer1D::sKeyOffset(), emptyset );
    }

    sd_.raypars_.setYN( Seis::SynthGenBase::sKeyNMO(), 
			    nmobox_->isChecked() || !isps );

    sd_.isps_ = isps; 
    sd_.name_ = namefld_->text();
}


bool uiSynthGenDlg::acceptOK( CallBacker* )
{
    const char* nm = namefld_->text(); 
    if ( !nm )
	mErrRet("Please specify a valid name",return false);

    getFromScreen();
    
    return false;
}


bool uiSynthGenDlg::genNewCB( CallBacker* )
{
    getFromScreen();
    genNewReq.trigger();
    return true;
}


