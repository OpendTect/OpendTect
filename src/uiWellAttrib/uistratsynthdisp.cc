/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uistratsynthdisp.h"
#include "uiseiswvltsel.h"
#include "uisynthtorealscale.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uigraphicsview.h"
#include "uiflatviewer.h"
#include "uiflatviewmainwin.h"
#include "uiflatviewslicepos.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimultiflatviewcontrol.h"
#include "uimsg.h"
#include "uipsviewer2dmainwin.h"
#include "uipsviewer2dposdlg.h"
#include "uiraytrace1d.h"
#include "uiseparator.h"
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

static const char* sKeySnapLevel()	{ return "Snap Level"; }
static const char* sKeyNrSynthetics()	{ return "Nr of Synthetics"; }
static const char* sKeySyntheticNr()	{ return "Synthetics Nr"; }
static const char* sKeySynthetics()	{ return "Synthetics"; }

//mDefineInstanceCreatedNotifierAccess(uiStratSynthDisp)


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
    , viewChanged(this)
    , modSelChanged(this)		       
    , layerPropSelNeeded(this)
    , longestaimdl_(0)
    , lasttool_(0)
    , synthgendlg_(0)
    , prestackwin_(0)		      
    , currentsynthetic_(0)
    , taskrunner_( new uiTaskRunner(this) )
{
    stratsynth_.setTaskRunner( taskrunner_ );

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

    vwr_ = new uiFlatViewer( this );
    vwr_->rgbCanvas().disableImageSave();
    vwr_->setExtraBorders( uiRect( 0, 0 , 0, 0 ) );
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
    vwr_->viewChanged.notify( mCB(this,uiStratSynthDisp,viewChg) );

    uiFlatViewStdControl::Setup fvsu( this );
    fvsu.withedit(true).withthumbnail(false).withcoltabed(false)
	.tba((int)uiToolBar::Right ).withflip(false);
    control_ = new uiMultiFlatViewControl( *vwr_, fvsu );
    control_->zoomChanged.notify( mCB(this,uiStratSynthDisp,zoomChg) );

    offsetChged(0);

    //mTriggerInstanceCreatedNotifier();
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
    BufferString curitem = datalist_->box()->text();
    datalist_->box()->setEmpty();
    for ( int idx=0; idx<stratsynth_.nrSynthetics(); idx ++)
    {
	const SyntheticData* sd = stratsynth_.getSyntheticByIdx( idx );
	if ( !sd ) continue;

	mDynamicCastGet(const PropertyRefSyntheticData*,prsd,sd);
	if ( prsd ) continue;
	datalist_->box()->addItem( sd->name() );
    }

    datalist_->box()->setCurrentItem( curitem );
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

    const float offset = mCast( float, xrg.start );
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
       				     bool dispflattened )
{
    const bool modelchange = dispflattened_ != dispflattened;
    StratSynth::Level* lvl = new StratSynth::Level( lnm, zvals, col );
    stratsynth_.setLevel( lvl );
    levelSnapChanged(0);

    dispflattened_ = dispflattened;
    if ( modelchange )
    {
	doModelChange();
	vwr_->setView( vwr_->boundingBox() );
	control_->zoomMgr().toStart();
    }
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
	    const float tval = dispflattened_ ? 0 : imdl < tbuf.size() ? 
		tbuf.get(imdl)->info().pick : mUdf(float);

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


void uiStratSynthDisp::setCurrentWavelet()
{
    currentsynthetic_ = 0;
    stratsynth_.setWavelet( wvltfld_->getWavelet() );
    SyntheticData* sd = stratsynth_.getSynthetic( datalist_->box()->text() );
    if ( !sd ) return;

    sd->setWavelet( wvltfld_->getName() );
    currentsynthetic_ = sd;

    wvltChanged.trigger();
    if ( synthgendlg_ )
	synthgendlg_->updateWaveletName();
    currentsynthetic_->fillGenParams( stratsynth_.genParams() );

}


void uiStratSynthDisp::wvltChg( CallBacker* )
{
    setCurrentWavelet();
    syntheticChanged( 0 );
    displaySynthetic( currentsynthetic_ );
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


void uiStratSynthDisp::viewChg( CallBacker* )
{
    viewChanged.trigger();
}


void uiStratSynthDisp::zoomChg( CallBacker* )
{
    zoomChanged.trigger();
}


float uiStratSynthDisp::centralTrcShift() const
{
    if ( !dispflattened_ ) return 0.0;
    const int centrcidx = mNINT32( vwr_->curView().centre().x );
    const SeisTrc* centtrc =
	postStackTraces().size() ? postStackTraces().get( centrcidx ) :  0;
    if ( !centtrc ) return 0.0f;
    return centtrc->info().pick;
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
	const float flattenedshift = centralTrcShift();
	wr.setTop( d2t.getDepth((float)wr.top()+flattenedshift)-
		   d2t.getDepth(flattenedshift) );
	wr.setBottom( d2t.getDepth((float)wr.bottom()+flattenedshift) -
		      d2t.getDepth(flattenedshift) );
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
    d2tmodels_ = 0;
    if ( !sd ) return;

    mDynamicCastGet(const PreStackSyntheticData*,presd,sd);
    mDynamicCastGet(const PostStackSyntheticData*,postsd,sd);

    const float offset = mCast( float, offsetposfld_->getValue() );
    const SeisTrcBuf* tbuf = presd ? presd->getTrcBuf( offset, 0 ) 
				   : &postsd->postStackPack().trcBuf();

    if ( !tbuf ) return;

    SeisTrcBuf* disptbuf = new SeisTrcBuf( true );
    tbuf->copyInto( *disptbuf );

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

    ColTab::MapperSetup& mapper = vwr_->appearance().ddpars_.vd_.mappersetup_;
    mapper.cliprate_ = Interval<float>(0.0,0.0);
    mapper.autosym0_ = true;
    mapper.symmidval_ = 0.0;

    vwr_->setPack( true, dp->id(), false, !hadpack );
    vwr_->setPack( false, dp->id(), false, !hadpack );
    levelSnapChanged( 0 );
}


void uiStratSynthDisp::displayPreStackDirSynthetic( const SyntheticData* sd )
{
    if ( !prestackwin_ ) return;

    if ( !sd ) return;
    mDynamicCastGet(const PreStack::GatherSetDataPack*,gsetdp,&sd->getPack())
    if ( !gsetdp ) return;

    prestackwin_->removeDataPacks();
    TypeSet<PreStackView::GatherInfo> gatherinfos;
    const ObjectSet<PreStack::Gather>& gathers = gsetdp->getGathers();
    for ( int idx=0; idx<gathers.size(); idx++ )
    {
	PreStack::Gather* gather = new PreStack::Gather( *gathers[idx] );
	gather->setZRange( gathers[idx]->zRange() );
	DPM(DataPackMgr::FlatID()).add( gather );

	PreStackView::GatherInfo gatherinfo;
	gatherinfo.isstored_ = false;
	gatherinfo.gathernm_ = sd->name();
	gatherinfo.bid_ = gather->getBinID();
	gatherinfo.dpid_ = gather->id();
	gatherinfo.isselected_ = true;
	gatherinfos += gatherinfo;
    }

    prestackwin_->setDataPacks( gatherinfos );
    /* TODO have to apply some where;
    ColTab::MapperSetup& mapper = vwr.appearance().ddpars_.vd_.mappersetup_;
    mapper.cliprate_ = Interval<float>(0.0,0.0);
    mapper.autosym0_ = true;
    mapper.symmidval_ = 0.0;*/
}


void uiStratSynthDisp::selPreStackDataCB( CallBacker* cb )
{
    BufferStringSet allgnms, selgnms;
    for ( int idx=0; idx<stratsynth_.nrSynthetics(); idx++ )
	allgnms.addIfNew( stratsynth_.getSyntheticByIdx(idx)->name() );
    prestackwin_->getGatherNames( selgnms );
    PreStackView::uiViewer2DSelDataDlg seldlg( prestackwin_, allgnms, selgnms );
    if ( seldlg.go() )
	prestackwin_->setGatherNames( selgnms );
}


void uiStratSynthDisp::viewPreStackPush( CallBacker* cb )
{
    if ( prestackwin_ )
    {
	delete prestackwin_; 
	prestackwin_ = 0;
    }


    if ( !currentsynthetic_ || !currentsynthetic_->isPS() )
	return;
    prestackwin_ =
	new PreStackView::uiSyntheticViewer2DMainWin(this,"Prestack view");
    if ( prestackwin_ )
	prestackwin_->seldatacalled_.notify(
		mCB(this,uiStratSynthDisp,selPreStackDataCB) );
    displayPreStackDirSynthetic( currentsynthetic_ );
    prestackwin_->show();
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
	sd = stratsynth_.getSynthetic( datalist_->box()->text() );

    currentsynthetic_ = sd;
    if ( !currentsynthetic_ ) return;

    NotifyStopper notstop( wvltfld_->newSelection );
    wvltfld_->setInput( currentsynthetic_->waveletName() );
    stratsynth_.setWavelet( wvltfld_->getWavelet() );

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


void uiStratSynthDisp::syntheticChanged( CallBacker* cb )
{
    BufferString syntheticnm;
    if ( cb )
    {
	mCBCapsuleUnpack(BufferString,synthname,cb);
	syntheticnm = synthname;
    }
    else
	syntheticnm = datalist_->box()->text();

    if ( datalist_->box()->isPresent(syntheticnm) )
    {
	datalist_->box()->setCurrentItem( syntheticnm );
	stratsynth_.removeSynthetic( syntheticnm );
	SyntheticData* sd = stratsynth_.addSynthetic();
	if ( !sd )
	    mErrRet(stratsynth_.errMsg(), return );
	setCurrentSynthetic();
	displaySynthetic( currentsynthetic_ );
    }
}


void uiStratSynthDisp::syntheticRemoved( CallBacker* cb )
{
    mCBCapsuleUnpack(BufferString,synthname,cb);
    stratsynth_.removeSynthetic( synthname );
    if ( datalist_->box()->isPresent(synthname) )
    {
	datalist_->box()->setEmpty();
	
	for ( int idx=0; idx<stratsynth_.nrSynthetics(); idx ++)
	{
	    const SyntheticData* sd = stratsynth_.getSyntheticByIdx( idx );
	    if ( !sd ) continue;
	    mDynamicCastGet(const PropertyRefSyntheticData*,prsd,sd);
	    if ( prsd ) continue;
	    datalist_->box()->addItem( sd->name() );
	}
    }

    synthgendlg_->updateSynthNames();
}


void uiStratSynthDisp::addEditSynth( CallBacker* )
{
    if ( !synthgendlg_ )
    {
	synthgendlg_ = new uiSynthGenDlg( this, stratsynth_ );
	synthgendlg_->synthRemoved.notify(
		mCB(this,uiStratSynthDisp,syntheticRemoved) );
	synthgendlg_->synthChanged.notify(
		mCB(this,uiStratSynthDisp,syntheticChanged) );
	synthgendlg_->genNewReq.notify(
			    mCB(this,uiStratSynthDisp,genNewSynthetic) );
    }

    synthgendlg_->putToScreen();
    synthgendlg_->go();
}


void uiStratSynthDisp::offsetChged( CallBacker* )
{
    displayPostStackDirSynthetic( currentsynthetic_ );
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

#define mChkPresent( nm )\
{\
if ( datalist_->box()->isPresent(nm) );\
    mErrRet( "Name already exists, please select another name", return );\
}

void uiStratSynthDisp::genNewSynthetic( CallBacker* )
{
    if ( !synthgendlg_ ) 
	return;

    MouseCursorChanger mcchger( MouseCursor::Wait );
    SyntheticData* sd = stratsynth_.addSynthetic();
    if ( !sd )
	mErrRet(stratsynth_.errMsg(), return )
    else
    {
	updateSyntheticList();
	synthgendlg_->putToScreen();
	synthgendlg_->updateSynthNames();
    }
}


SyntheticData* uiStratSynthDisp::getCurrentSyntheticData() const
{
    return currentsynthetic_; 
}


void uiStratSynthDisp::fillPar( IOPar& par ) const
{
    IOPar stratsynthpar;
    stratsynthpar.set( sKeySnapLevel(), levelsnapselfld_->box()->currentItem());
    int nr_nonproprefsynths = 0;
    for ( int idx=0; idx<stratsynth_.nrSynthetics(); idx++ )
    {
	SyntheticData* sd = stratsynth_.getSyntheticByIdx( idx );
	if ( !sd ) continue;
	mDynamicCastGet(const PropertyRefSyntheticData*,prsd,sd);
	if ( prsd ) continue;
	nr_nonproprefsynths++;
	SynthGenParams genparams;
	sd->fillGenParams( genparams );
	IOPar synthpar;
	genparams.fillPar( synthpar );
	stratsynthpar.mergeComp( synthpar, IOPar::compKey(sKeySyntheticNr(),
		    		 nr_nonproprefsynths-1) );
    }

    stratsynthpar.set( sKeyNrSynthetics(), nr_nonproprefsynths ); 
    par.mergeComp( stratsynthpar, sKeySynthetics() );
}


bool uiStratSynthDisp::usePar( const IOPar& par ) 
{
    PtrMan<IOPar> stratsynthpar = par.subselect( sKeySynthetics() );
    if ( !stratsynthpar ) return false;

    int snaplvl = 0;
    stratsynthpar->get( sKeySnapLevel(), snaplvl );

    int nrsynths;
    stratsynthpar->get( sKeyNrSynthetics(), nrsynths );
    stratsynth_.clearSynthetics();
    datalist_->box()->setEmpty();
    for ( int idx=0; idx<nrsynths; idx++ )
    {
	PtrMan<IOPar> synthpar =
	    stratsynthpar->subselect( IOPar::compKey(sKeySyntheticNr(),idx) );
	if ( !synthpar ) continue;
	SynthGenParams genparams;
	genparams.usePar( *synthpar );
	wvltfld_->setInput( genparams.wvltnm_ );
	stratsynth_.setWavelet( wvltfld_->getWavelet() );
	SyntheticData* sd = stratsynth_.addSynthetic( genparams );
	if ( !sd )
	{
	    mErrRet(stratsynth_.errMsg(),);
	    continue;
	}

	datalist_->box()->addItem( sd->name() );
    }

    if ( !stratsynth_.nrSynthetics() )
    {
	displaySynthetic( 0 );
	return false;
    }

    setCurrentSynthetic();
    displaySynthetic( currentsynthetic_ );
    levelsnapselfld_->box()->setCurrentItem( snaplvl );

    return true;
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


void uiSynthSlicePos::setValue( int val ) const
{
    sliceposbox_->setValue( val );
}


int uiSynthSlicePos::getValue() const
{
    return sliceposbox_->getValue();
}




uiSynthGenDlg::uiSynthGenDlg( uiParent* p, StratSynth& gp) 
    : uiDialog(p,uiDialog::Setup("Specify Synthetic Parameters",mNoDlgTitle,
				 "103.4.4").modal(false))
    , stratsynth_(gp)
    , genNewReq(this)
    , synthRemoved(this)
    , synthChanged(this)
{
    setCtrlStyle( DoAndStay );

    setOkText( "Apply" );
    uiGroup* syntlistgrp = new uiGroup( this, "Synthetics List" );
    uiLabeledListBox* llb =
	new uiLabeledListBox( syntlistgrp, "Synthetics", false,
			      uiLabeledListBox::AboveMid );
    synthnmlb_ = llb->box();
    synthnmlb_->selectionChanged.notify(
	    mCB(this,uiSynthGenDlg,changeSyntheticsCB) );
    uiPushButton* rembut =
	new uiPushButton( syntlistgrp, "Remove selected",
			  mCB(this,uiSynthGenDlg,removeSyntheticsCB), true );
    rembut->attach( leftAlignedBelow, llb );

    uiSeparator* versep = new uiSeparator( this );
    versep->attach( rightTo, syntlistgrp );

    uiGroup* pargrp = new uiGroup( this, "Parameter Group" );
    pargrp->attach( rightTo, versep );
    CallBack cb( mCB(this,uiSynthGenDlg,typeChg) );
    typefld_ = new uiGenInput( pargrp, "Synthethic type",
			BoolInpSpec(true,"Post-Stack","Pre-Stack") );
    typefld_->valuechanged.notify( cb );

    stackfld_ = new uiCheckBox( pargrp, "Stack from Pre-Stack" );
    stackfld_->activated.notify( cb );
    stackfld_->attach( rightOf, typefld_ );

    uiRayTracer1D::Setup rsu; rsu.dooffsets_ = true;
    rtsel_ = new uiRayTracerSel( pargrp, rsu );
    rtsel_->usePar( stratsynth_.genParams().raypars_ ); 
    rtsel_->attach( alignedBelow, typefld_ );
    rtsel_->offsetChanged.notify( mCB(this,uiSynthGenDlg,parsChanged) );

    
    
    wvltfld_ = new uiSeisWaveletSel( pargrp );
    wvltfld_->newSelection.notify( mCB(this,uiSynthGenDlg,parsChanged) );
    wvltfld_->attach( alignedBelow, rtsel_ );
    wvltfld_->setFrame( false );
    
    nmofld_ = new uiGenInput( pargrp, "Apply NMO corrections",
			     BoolInpSpec(true) );
    mAttachCB( nmofld_->valuechanged, uiSynthGenDlg, typeChg);
    nmofld_->attach( alignedBelow, wvltfld_ );
    
    stretchmutelimitfld_ = new uiGenInput(pargrp, "Stretch mute (%)",
					  FloatInpSpec() );
    stretchmutelimitfld_->attach( alignedBelow, nmofld_ );
    
    mutelenfld_ = new uiGenInput( pargrp, "Mute taper-length (ms)",
				      FloatInpSpec() );
    mutelenfld_->attach( alignedBelow, stretchmutelimitfld_ );

    namefld_ = new uiGenInput( pargrp, "Name" );
    namefld_ ->attach( alignedBelow, mutelenfld_ );
    namefld_->valuechanged.notify( mCB(this,uiSynthGenDlg,nameChanged) );

    gennewbut_ = new uiPushButton( pargrp, "&Add as new", true );
    gennewbut_->activated.notify( mCB(this,uiSynthGenDlg,genNewCB) );
    gennewbut_->attach( alignedBelow, namefld_ );

    updateSynthNames();
    synthnmlb_->setSelected( 0, true );
    stratsynth_.genParams().name_ = synthnmlb_->getText();
}


void uiSynthGenDlg::updateSynthNames() 
{
    synthnmlb_->setEmpty();
    for ( int idx=0; idx<stratsynth_.nrSynthetics(); idx++ )
    {
	const SyntheticData* sd = stratsynth_.getSyntheticByIdx( idx );
	if ( !sd ) continue;

	mDynamicCastGet(const PropertyRefSyntheticData*,prsd,sd);
	if ( prsd ) continue;
	synthnmlb_->addItem( sd->name() );
    }
}


void uiSynthGenDlg::changeSyntheticsCB( CallBacker* )
{
    SyntheticData* sd = stratsynth_.getSynthetic( synthnmlb_->getText() );
    if ( !sd ) return;
    sd->fillGenParams( stratsynth_.genParams() );
    putToScreen();
}


void uiSynthGenDlg::nameChanged( CallBacker* )
{
    stratsynth_.genParams().name_ = namefld_->text();
}


void uiSynthGenDlg::parsChanged( CallBacker* )
{
    if ( !getFromScreen() ) return;
    BufferString nm;
    stratsynth_.genParams().createName( nm );
    namefld_->setText( nm );
}


void uiSynthGenDlg::removeSyntheticsCB( CallBacker* )
{
    if ( synthnmlb_->size()==1 )
	return uiMSG().error( "Cannot remove all synthetics" );

    BufferString synthname( synthnmlb_->getText() );
    synthnmlb_->removeItem( synthnmlb_->currentItem() );
    synthRemoved.trigger( synthname );
}


void uiSynthGenDlg::updateFieldSensitivity()
{
    const bool isps = !typefld_->getBoolValue();
    stackfld_->display( !isps );
    nmofld_->display( isps );
    const bool needranges = isps || stackfld_->isChecked();
    rtsel_->current()->displayOffsetFlds( needranges );
    rtsel_->current()->setOffsetRange( uiRayTracer1D::Setup().offsetrg_ );
    
    bool showmute = true;
    if ( isps && !nmofld_->getBoolValue() )
	showmute = false;
    mutelenfld_->display( showmute );
    stretchmutelimitfld_->display( showmute );
}


void uiSynthGenDlg::typeChg( CallBacker* )
{
    updateFieldSensitivity();
    parsChanged( 0 );
}


void uiSynthGenDlg::putToScreen()
{
    wvltfld_->setInput( stratsynth_.genParams().wvltnm_ );
    namefld_->setText( stratsynth_.genParams().name_ );
    typefld_->setValue( !stratsynth_.genParams().isps_ ); 
    
    const bool isps = stratsynth_.genParams().isps_;
    TypeSet<float> offsets;
    stratsynth_.genParams().raypars_.get( RayTracer1D::sKeyOffset(), offsets );
        
    NotifyStopper stop( stackfld_->activated );
    stackfld_->setChecked( !isps && offsets.size()>1 );
    
    bool donmo = true;
    stratsynth_.genParams().raypars_.getYN(
	    Seis::SynthGenBase::sKeyNMO(),donmo );
    nmofld_->setValue( donmo );
    
    float mutelen = Seis::SynthGenBase::cStdMuteLength();
    stratsynth_.genParams().raypars_.get(
	    Seis::SynthGenBase::sKeyMuteLength(), mutelen );
    mutelenfld_->setValue(
	    mIsUdf(mutelen) ? mutelen : mutelen *ZDomain::Time().userFactor());
    
    float stretchlimit = Seis::SynthGenBase::cStdStretchLimit();
    stratsynth_.genParams().raypars_.get(
			Seis::SynthGenBase::sKeyStretchLimit(), stretchlimit );
    stretchmutelimitfld_->setValue( mToPercent( stretchlimit ) );

    const bool needranges = isps || offsets.size()>1;
    rtsel_->current()->displayOffsetFlds( needranges );
    rtsel_->usePar( stratsynth_.genParams().raypars_ );
    updateFieldSensitivity();
}


bool uiSynthGenDlg::getFromScreen() 
{
    const char* nm = namefld_->text(); 
    if ( !nm )
	mErrRet("Please specify a valid name",return false);
    
    if ( mIsUdf(mutelenfld_->getfValue() ) || mutelenfld_->getfValue()<0 )
	mErrRet( "The mutelength must be more than zero.", return false );
    
    if ( mIsUdf(stretchmutelimitfld_->getfValue()) ||
	 stretchmutelimitfld_->getfValue()<0 )
	mErrRet( "The stretch mute must be more than 0%", return false );
    stratsynth_.genParams().raypars_.setEmpty();
    rtsel_->fillPar( stratsynth_.genParams().raypars_ );
    const bool isps = !typefld_->getBoolValue();
    const bool dostack = stackfld_->isChecked();
    if ( !isps && !dostack )
	RayTracer1D::setIOParsToZeroOffset( stratsynth_.genParams().raypars_ );

    stratsynth_.genParams().wvltnm_ = wvltfld_->getName();
    stratsynth_.setWavelet( wvltfld_->getWavelet() );
    stratsynth_.genParams().raypars_.setYN( Seis::SynthGenBase::sKeyNMO(), 
				    nmofld_->getBoolValue() || !isps );
    stratsynth_.genParams().isps_ = isps; 
    stratsynth_.genParams().name_ = namefld_->text();
    
    stratsynth_.genParams().raypars_.set( Seis::SynthGenBase::sKeyMuteLength(),
		     mutelenfld_->getfValue() / ZDomain::Time().userFactor() );
    stratsynth_.genParams().raypars_.set(
	    Seis::SynthGenBase::sKeyStretchLimit(),
	    mFromPercent( stretchmutelimitfld_->getfValue()) );
    return true;
}


void uiSynthGenDlg::updateWaveletName() 
{
    wvltfld_->setInput( stratsynth_.genParams().wvltnm_ );
    BufferString nm;
    stratsynth_.genParams().createName( nm );
    namefld_->setText( nm );
}


bool uiSynthGenDlg::acceptOK( CallBacker* )
{
    if ( !getFromScreen() ) return false;
    BufferString synthname( synthnmlb_->getText() );
    synthChanged.trigger( synthname );
    
    return true;
}


bool uiSynthGenDlg::isCurSynthChanged() const
{
    const int selidx = synthnmlb_->nextSelected(-1);
    BufferString selstr = synthnmlb_->textOfItem( selidx );
    SyntheticData* sd = stratsynth_.getSynthetic( selstr );
    if ( !sd ) return true;
    SynthGenParams genparams;
    sd->fillGenParams( genparams );
    return !(genparams == stratsynth_.genParams());
}


bool uiSynthGenDlg::rejectOK( CallBacker* )
{
    const char* nm = namefld_->text(); 
    if ( !nm )
	mErrRet("Please specify a valid name",return false);

    if ( !getFromScreen() ) return false;
    if ( !isCurSynthChanged() )
	return true;
    BufferString msg( "Selected synthetic has been changed. "
	    	      "Do you want to Apply the changes?" );
    if ( uiMSG().askGoOn(msg,"Apply","Do not Apply") )
	acceptOK( 0 );

    return true;
}


bool uiSynthGenDlg::genNewCB( CallBacker* )
{
    if ( !getFromScreen() ) return false;

    if ( synthnmlb_->isPresent(stratsynth_.genParams().name_) )
    {
	BufferString msg( "Synthectic data of name '" );
	msg += stratsynth_.genParams().name_;
	msg += "' is already present. Please choose a different name";
	uiMSG().error( msg );
	return false;
    }

    genNewReq.trigger();
    return true;
}

