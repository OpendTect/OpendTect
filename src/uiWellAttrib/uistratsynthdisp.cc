/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uistratsynthdisp.h"
#include "uisynthgendlg.h"
#include "uiseiswvltsel.h"
#include "uisynthtorealscale.h"
#include "uicombobox.h"
#include "uiflatviewer.h"
#include "uiflatviewmainwin.h"
#include "uiflatviewslicepos.h"
#include "uilabel.h"
#include "uimultiflatviewcontrol.h"
#include "uimsg.h"
#include "uipsviewer2dmainwin.h"
#include "uispinbox.h"
#include "uitaskrunner.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"

#include "flatviewzoommgr.h"
#include "flatposdata.h"
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


#define mStratSynth (!useed_ ? stratsynth_ : edstratsynth_)


uiStratSynthDisp::uiStratSynthDisp( uiParent* p, const Strat::LayerModel& lm,
			            const Strat::LayerModel& lmed )
    : uiGroup(p,"LayerModel synthetics display")
    , lm_(lm)  
    , d2tmodels_(0)	    
    , stratsynth_(new StratSynth(lm))
    , edstratsynth_(new StratSynth(lmed))
    , useed_(false)	
    , dispeach_(1)	
    , dispskipz_(0)	
    , dispflattened_(false)
    , selectedtrace_(-1)	
    , selectedtraceaux_(0)
    , levelaux_(0)
    , wvltChanged(this)
    , zoomChanged(this)
    , viewChanged(this)
    , modSelChanged(this)		       
    , synthsChanged(this)		       
    , layerPropSelNeeded(this)
    , longestaimdl_(0)
    , lasttool_(0)
    , synthgendlg_(0)
    , prestackwin_(0)		      
    , currentwvasynthetic_(0)
    , currentvdsynthetic_(0)
    , autoupdate_(true)
    , isbrinefilled_(true)
    , taskrunner_( new uiTaskRunner(this) )
{
    stratsynth_->setTaskRunner( taskrunner_ );
    edstratsynth_->setTaskRunner( taskrunner_ );

    topgrp_ = new uiGroup( this, "Top group" );
    topgrp_->setFrame( true );
    topgrp_->setStretch( 2, 0 );
    
    uiLabeledComboBox* datalblcbx =
	new uiLabeledComboBox( topgrp_, "Wiggle View", "" );
    wvadatalist_ = datalblcbx->box();
    wvadatalist_->selectionChanged.notify(
	    mCB(this,uiStratSynthDisp,wvDataSetSel) );
    wvadatalist_->setHSzPol( uiObject::Wide );

    uiToolButton* edbut = new uiToolButton( topgrp_, "edit", 
	    			"Add/Edit Synthetic DataSet",
				mCB(this,uiStratSynthDisp,addEditSynth) );

    edbut->attach( leftOf, datalblcbx );

    uiGroup* dataselgrp = new uiGroup( this, "Data Selection" );
    dataselgrp->attach( rightBorder );
    dataselgrp->attach( ensureRightOf, topgrp_ );
    
    uiLabeledComboBox* prdatalblcbx =
	new uiLabeledComboBox( dataselgrp, "Variable Density View", "" );
    vddatalist_ = prdatalblcbx->box();
    vddatalist_->selectionChanged.notify(
	    mCB(this,uiStratSynthDisp,vdDataSetSel) );
    vddatalist_->setHSzPol( uiObject::Wide );
    prdatalblcbx->attach( leftBorder );

    uiToolButton* expbut = new uiToolButton( prdatalblcbx, "export", 
	    			"Export Synthetic DataSet",
				mCB(this,uiStratSynthDisp,exportSynth) );
    expbut->attach( rightOf, vddatalist_ );

    datagrp_ = new uiGroup( this, "DataSet group" );
    datagrp_->attach( ensureBelow, topgrp_ );
    datagrp_->attach( ensureBelow, dataselgrp );
    datagrp_->setFrame( true );
    datagrp_->setStretch( 2, 0 );

    uiToolButton* layertb = new uiToolButton( datagrp_, "defraytraceprops", 
				    "Specify input for synthetic creation", 
				    mCB(this,uiStratSynthDisp,layerPropsPush));

    wvltfld_ = new uiSeisWaveletSel( datagrp_ );
    wvltfld_->newSelection.notify( mCB(this,uiStratSynthDisp,wvltChg) );
    wvltfld_->setFrame( false );
    wvltfld_->attach( rightOf, layertb );
    mStratSynth->setWavelet( wvltfld_->getWavelet() );

    scalebut_ = new uiPushButton( datagrp_, "Scale", false );
    scalebut_->activated.notify( mCB(this,uiStratSynthDisp,scalePush) );
    scalebut_->attach( rightOf, wvltfld_ );

    uiLabeledComboBox* lvlsnapcbx =
	new uiLabeledComboBox( datagrp_, "Snap level" );
    levelsnapselfld_ = lvlsnapcbx->box();
    lvlsnapcbx->attach( rightOf, scalebut_ );
    lvlsnapcbx->setStretch( 2, 0 );
    levelsnapselfld_->selectionChanged.notify(
				mCB(this,uiStratSynthDisp,levelSnapChanged) );
    levelsnapselfld_->addItems( VSEvent::TypeNames() );

    prestackgrp_ = new uiGroup( datagrp_, "Pre-Stack View Group" );
    prestackgrp_->attach( rightOf, lvlsnapcbx, 20 );

    offsetposfld_ = new uiSynthSlicePos( prestackgrp_, "Offset" );
    offsetposfld_->positionChg.notify( mCB(this,uiStratSynthDisp,offsetChged));

    prestackbut_ = new uiToolButton( prestackgrp_, "nonmocorr64", 
				"View Offset Direction", 
				mCB(this,uiStratSynthDisp,viewPreStackPush) );
    prestackbut_->attach( rightOf, offsetposfld_);

    vwr_ = new uiFlatViewer( this );
    vwr_->rgbCanvas().disableImageSave();
    vwr_->setExtraBorders( uiRect( 0, 0 , 0, 0 ) );
    vwr_->setInitialSize( uiSize(800,300) ); //TODO get hor sz from laymod disp
    vwr_->setStretch( 2, 2 );
    vwr_->attach( ensureBelow, datagrp_ );
    FlatView::Appearance& app = vwr_->appearance();
    app.setGeoDefaults( true );
    app.setDarkBG( false );
    app.annot_.title_.setEmpty();
    app.annot_.x1_.showAll( true );
    app.annot_.x2_.showAll( true );
    app.annot_.x2_.name_ = "TWT (s)";
    app.ddpars_.show( true, true );
    vwr_->viewChanged.notify( mCB(this,uiStratSynthDisp,viewChg) );

    uiFlatViewStdControl::Setup fvsu( this );
    fvsu.withedit(true).withthumbnail(false).withcoltabed(false)
	.tba((int)uiToolBar::Right ).withflip(false);
    control_ = new uiMultiFlatViewControl( *vwr_, fvsu );
    control_->zoomChanged.notify( mCB(this,uiStratSynthDisp,zoomChg) );

    displayPostStackSynthetic( currentwvasynthetic_, true );
    displayPostStackSynthetic( currentvdsynthetic_, false );

    //mTriggerInstanceCreatedNotifier();
}


uiStratSynthDisp::~uiStratSynthDisp()
{
    delete stratsynth_;
    delete edstratsynth_;
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
    mStratSynth->clearSynthetics();
    wvadatalist_->setEmpty();
    vddatalist_->setEmpty();
}


void uiStratSynthDisp::updateSyntheticList( bool wva )
{
    uiComboBox* datalist = wva ? wvadatalist_ : vddatalist_;
    BufferString curitem = datalist->text();
    datalist->setEmpty();
    for ( int idx=0; idx<mStratSynth->nrSynthetics(); idx ++)
    {
	const SyntheticData* sd = mStratSynth->getSyntheticByIdx( idx );
	if ( !sd ) continue;

	mDynamicCastGet(const PropertyRefSyntheticData*,prsd,sd);
	if ( wva && prsd ) continue;
	datalist->addItem( sd->name() );
    }

    datalist->setCurrentItem( curitem );
}


void uiStratSynthDisp::setDisplayZSkip( float zskip, bool withmodchg )
{
    dispskipz_ = zskip;
    if ( withmodchg )
	modelChanged();
}


void uiStratSynthDisp::setDispEach( int de )
{
    dispeach_ = de;
    displayPostStackSynthetic( currentwvasynthetic_, true );
    displayPostStackSynthetic( currentvdsynthetic_, false );
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
    StratSynth::Level* lvl = new StratSynth::Level( lnm, zvals, col );
    mStratSynth->setLevel( lvl );

    const bool domodelchg = dispflattened_ || dispflattened;
    dispflattened_ = dispflattened;
    if ( domodelchg )
    {
	doModelChange();
	control_->zoomMgr().toStart();
	return;
    }

    levelSnapChanged(0);
}


void uiStratSynthDisp::setZoomView( const uiWorldRect& wr )
{
    Geom::Point2D<double> centre = wr.centre();
    Geom::Size2D<double> newsz = wr.size();
    control_->setNewView( centre, newsz );
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
    const StratSynth::Level* lvl = mStratSynth->getLevel();
    if ( !lvl )  return;
    StratSynth::Level* edlvl = const_cast<StratSynth::Level*>( lvl );
    VSEvent::Type tp;
    VSEvent::parseEnumType( levelsnapselfld_->text(), tp );
    edlvl->snapev_ = tp;
    drawLevel();
}


const char* uiStratSynthDisp::levelName()  const
{
    const StratSynth::Level* lvl = mStratSynth->getLevel();
    return lvl ? lvl->name() : 0;
}


void uiStratSynthDisp::displayFRText()
{
    FlatView::AuxData* filltxtdata =
	vwr_->createAuxData( isbrinefilled_ ? "Brine filled"
					    : "Hydrocarbon filled" );
    filltxtdata->namepos_ = 0;
    uiWorldPoint txtpos =
	vwr_->boundingBox().bottomRight() - uiWorldPoint(10,10);
    filltxtdata->poly_ += txtpos;

    vwr_->addAuxData( filltxtdata );
    vwr_->handleChange( FlatView::Viewer::Annot, true );
}


void uiStratSynthDisp::drawLevel()
{
    delete vwr_->removeAuxData( levelaux_ );

    const StratSynth::Level* lvl = mStratSynth->getLevel();
    if ( d2tmodels_ && !d2tmodels_->isEmpty() && lvl )
    {
	SeisTrcBuf& tbuf = const_cast<SeisTrcBuf&>( curTrcBuf() );
	FlatView::AuxData* auxd = vwr_->createAuxData("Level markers");
	mStratSynth->snapLevelTimes( tbuf, *d2tmodels_ );

	auxd->linestyle_.type_ = LineStyle::None;
	for ( int imdl=0; imdl<tbuf.size(); imdl ++ )
	{
	    if ( tbuf.get(imdl)->isNull() )
		continue;
	    const float tval =
		dispflattened_ ? 0 :  tbuf.get(imdl)->info().pick;

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

    vwr_->handleChange( FlatView::Viewer::Auxdata, true );
}


void uiStratSynthDisp::setCurrentWavelet()
{
    currentwvasynthetic_ = currentvdsynthetic_ = 0;
    mStratSynth->setWavelet( wvltfld_->getWavelet() );
    SyntheticData* wvasd = mStratSynth->getSynthetic( wvadatalist_->text() );
    SyntheticData* vdsd = mStratSynth->getSynthetic( vddatalist_->text() );
    if ( !vdsd && !wvasd ) return;
    FixedString wvasynthnm( wvasd->name() );
    FixedString vdsynthnm( vdsd->name() );

    if ( wvasd )
    {
	wvasd->setWavelet( wvltfld_->getName() );
	currentwvasynthetic_ = wvasd;
	if ( synthgendlg_ )
	    synthgendlg_->updateWaveletName();
	currentwvasynthetic_->fillGenParams( mStratSynth->genParams() );
	wvltChanged.trigger();
	updateSynthetic( wvasynthnm, true );
    }

    if ( vdsynthnm == wvasynthnm )
    {
	setCurrentSynthetic( false );
	return;
    }

    mDynamicCastGet(const PropertyRefSyntheticData*,prsd,vdsd);
    if ( vdsd && !prsd )
    {
	vdsd->setWavelet( wvltfld_->getName() );
	currentvdsynthetic_ = vdsd;
	if ( vdsynthnm != wvasynthnm )
	{
	    currentvdsynthetic_->fillGenParams( mStratSynth->genParams() );
	    updateSynthetic( vdsynthnm, false );
	}
    }
}


void uiStratSynthDisp::wvltChg( CallBacker* )
{
    setCurrentWavelet();
    displaySynthetic( currentwvasynthetic_ );
    displayPostStackSynthetic( currentvdsynthetic_, false );
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
    if ( mStratSynth->getLevel() ) levelname = mStratSynth->getLevel()->name();
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


void uiStratSynthDisp::setSnapLevelSensitive( bool yn )
{
    levelsnapselfld_->setSensitive( yn );
}


float uiStratSynthDisp::centralTrcShift() const
{
    if ( !dispflattened_ ) return 0.0;
    bool forward = false;
    int forwardidx = mNINT32( vwr_->curView().centre().x );
    int backwardidx = forwardidx-1;
    const SeisTrcBuf& trcbuf = postStackTraces();
    if ( !trcbuf.size() ) return mUdf(float);
    while ( true )
    {
	if ( backwardidx<0 || forwardidx>=trcbuf.size() )
	    return mUdf(float);
	const int centrcidx = forward ? forwardidx : backwardidx;
	const SeisTrc* centtrc = trcbuf.size() ? trcbuf.get( centrcidx ) :  0;
	if ( centtrc && !mIsUdf(centtrc->info().pick) )
	    return centtrc->info().pick;
	forward ? forwardidx++ : backwardidx--;
	forward = !forward;
    }

    return mUdf(float);
}


const uiWorldRect& uiStratSynthDisp::curView( bool indpth ) const
{
    static uiWorldRect wr; wr = vwr_->curView();
    if ( !indpth )
	return wr;

    if ( d2tmodels_ && !d2tmodels_->isEmpty() )
    {
	int mdlidx = longestaimdl_;
	if ( mdlidx >= d2tmodels_->size() )
	    mdlidx = d2tmodels_->size()-1;

	const float flattenedshift = centralTrcShift();
	for ( int idx=0; idx<d2tmodels_->size(); idx++ )
	{
	    const TimeDepthModel& d2t = *(*d2tmodels_)[idx];
	    const double top = d2t.getDepth((float)wr.top()+flattenedshift)
			      - d2t.getDepth(flattenedshift)-dispskipz_;
	    const double bottom =
		d2t.getDepth((float)wr.bottom()+flattenedshift)
		- d2t.getDepth(flattenedshift);
	    if ( idx==0 || top<wr.top() )
		wr.setTop( top );
	    if ( idx==0 || bottom>wr.bottom() )
		wr.setBottom( bottom );
	}
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
    displayPostStackSynthetic( sd );
    displayPreStackSynthetic( sd );
}


void uiStratSynthDisp::displayPostStackSynthetic( const SyntheticData* sd,
						     bool wva )
{
    const bool hadpack = vwr_->pack( wva ); 
    if ( hadpack )
	vwr_->removePack( vwr_->packID(wva) ); 
    vwr_->removeAllAuxData();
    d2tmodels_ = 0;
    if ( !sd )
    {
	SeisTrcBuf* disptbuf = new SeisTrcBuf( true );
	SeisTrcBufDataPack* dp = new SeisTrcBufDataPack( disptbuf, Seis::Line, 
					SeisTrcInfo::TrcNr, "Forward Modeling");
	dp->posData().setRange( true, StepInterval<double>(1.0,1.0,1.0) );
	DPM( DataPackMgr::FlatID() ).add( dp );
	vwr_->setPack( wva, dp->id(), false, !hadpack );
	return;
    }

    mDynamicCastGet(const PreStackSyntheticData*,presd,sd);
    mDynamicCastGet(const PostStackSyntheticData*,postsd,sd);

    const float offset = mCast( float, offsetposfld_->getValue() );
    const SeisTrcBuf* tbuf = presd ? presd->getTrcBuf( offset, 0 ) 
				   : &postsd->postStackPack().trcBuf();

    if ( !tbuf ) return;

    SeisTrcBuf* disptbuf = new SeisTrcBuf( true );
    tbuf->copyInto( *disptbuf );

    mStratSynth->decimateTraces( *disptbuf, dispeach_ );
    if ( dispflattened_ )
    {
	mStratSynth->snapLevelTimes( *disptbuf, sd->d2tmodels_ );
	mStratSynth->flattenTraces( *disptbuf );
    }

    mStratSynth->trimTraces( *disptbuf, centralTrcShift(), sd->d2tmodels_,
	    		    dispskipz_ );

    SeisTrcBufDataPack* dp = new SeisTrcBufDataPack( disptbuf, Seis::Line, 
				    SeisTrcInfo::TrcNr, "Forward Modeling" );
    DPM( DataPackMgr::FlatID() ).add( dp );
    dp->setName( sd->name() );

    d2tmodels_ = &sd->d2tmodels_;
    for ( int idx=0; idx<d2tmodels_->size(); idx++ )
    {
	int maxaimodelsz =  0;
	if ( (*d2tmodels_)[idx]->size() > maxaimodelsz )
	    { maxaimodelsz = (*d2tmodels_)[idx]->size(); longestaimdl_ = idx; }
    }


    vwr_->setPack( wva, dp->id(), false, !hadpack );
    
    mDynamicCastGet(const PropertyRefSyntheticData*,prsd,sd);
    ColTab::MapperSetup& mapper =
	wva ? vwr_->appearance().ddpars_.wva_.mappersetup_
	    : vwr_->appearance().ddpars_.vd_.mappersetup_;
    mapper.cliprate_ = Interval<float>(0.0,0.0);
    mapper.autosym0_ = true;
    mapper.symmidval_ = prsd ? mUdf(float) : 0.0f;

    const Interval<double> xrg = vwr_->getDataPackRange( true );
    const Interval<double> zrg = vwr_->getDataPackRange( false );
    vwr_->setSelDataRanges( xrg, zrg );
    uiWorldRect wr( xrg.start, zrg.stop, xrg.stop, zrg.start );
    vwr_->setView( wr );
    vwr_->handleChange( FlatView::Viewer::DisplayPars );
    displayFRText();

    levelSnapChanged( 0 );
}


void uiStratSynthDisp::displayPreStackSynthetic( const SyntheticData* sd )
{
    if ( !prestackwin_ ) return;

    if ( !sd ) return;
    mDynamicCastGet(const PreStack::GatherSetDataPack*,gsetdp,&sd->getPack())
    mDynamicCastGet(const PreStackSyntheticData*,presd,sd)
    if ( !gsetdp || !presd ) return;

    const PreStack::GatherSetDataPack& angledp = presd->angleData();
    prestackwin_->removeGathers();
    TypeSet<PreStackView::GatherInfo> gatherinfos;
    const ObjectSet<PreStack::Gather>& gathers = gsetdp->getGathers();
    const ObjectSet<PreStack::Gather>& anglegathers = angledp.getGathers();
    for ( int idx=0; idx<gathers.size(); idx++ )
    {
	PreStack::Gather* gather = new PreStack::Gather( *gathers[idx] );
	gather->setName( sd->name() );
	BufferString anggnm( sd->name(), "(Angle Data)" );
	PreStack::Gather* anglegather= new PreStack::Gather(*anglegathers[idx]);
	anglegather->setName( anggnm );
	gather->setZRange( gathers[idx]->zRange() );
	DPM(DataPackMgr::FlatID()).add( gather );
	DPM(DataPackMgr::FlatID()).add( anglegather );

	PreStackView::GatherInfo gatherinfo;
	gatherinfo.isstored_ = false;
	gatherinfo.gathernm_ = sd->name();
	gatherinfo.bid_ = gather->getBinID();
	gatherinfo.wvadpid_ = gather->id();
	gatherinfo.vddpid_ = anglegather->id();
	gatherinfo.isselected_ = true;
	gatherinfos += gatherinfo;
    }

    prestackwin_->setGathers( gatherinfos );
    for ( int idx=0; idx<prestackwin_->nrViewers(); idx++ )
    {
	uiFlatViewer& vwr = prestackwin_->viewer( idx );
	ColTab::MapperSetup& vdmapper =
	    vwr.appearance().ddpars_.vd_.mappersetup_;
	vdmapper.cliprate_ = Interval<float>(0.0,0.0);
	vdmapper.autosym0_ = false;
	vdmapper.symmidval_ = mUdf(float);
	vwr.appearance().ddpars_.vd_.ctab_ = "Rainbow";
	ColTab::MapperSetup& wvamapper =
	    vwr.appearance().ddpars_.wva_.mappersetup_;
	wvamapper.cliprate_ = Interval<float>(0.0,0.0);
	wvamapper.autosym0_ = true;
	wvamapper.symmidval_ = 0.0f;
    }
}


void uiStratSynthDisp::selPreStackDataCB( CallBacker* cb )
{
    BufferStringSet allgnms, selgnms;
    for ( int idx=0; idx<mStratSynth->nrSynthetics(); idx++ )
	allgnms.addIfNew( mStratSynth->getSyntheticByIdx(idx)->name() );
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

    if ( !currentwvasynthetic_ || !currentwvasynthetic_->isPS() )
	return;
    prestackwin_ =
	new PreStackView::uiSyntheticViewer2DMainWin(this,"Prestack view");
    if ( prestackwin_ )
	prestackwin_->seldatacalled_.notify(
		mCB(this,uiStratSynthDisp,selPreStackDataCB) );
    displayPreStackSynthetic( currentwvasynthetic_ );
    prestackwin_->show();
}


void uiStratSynthDisp::setCurrentSynthetic( bool wva )
{
    SyntheticData* sd = mStratSynth->getSynthetic( wva ? wvadatalist_->text()
	    					      : vddatalist_->text() );
    if ( wva )
	currentwvasynthetic_ = sd;
    else
	currentvdsynthetic_ = sd;
    SyntheticData* cursynth = wva ? currentwvasynthetic_ : currentvdsynthetic_;

    if ( !cursynth ) return;

    NotifyStopper notstop( wvltfld_->newSelection );
    if ( wva )
    {
	wvltfld_->setInput( cursynth->waveletName() );
	mStratSynth->setWavelet( wvltfld_->getWavelet() );
    }
}

void uiStratSynthDisp::updateFields()
{
    mDynamicCastGet(const PreStackSyntheticData*,pssd,currentwvasynthetic_);
    if ( pssd )
    {
	StepInterval<float> limits( pssd->offsetRange() );
	limits.step = 100;
	offsetposfld_->setLimitSampling( limits );
    }

    prestackgrp_->setSensitive( pssd && pssd->hasOffset() );

    topgrp_->setSensitive( currentwvasynthetic_ );
    datagrp_->setSensitive( currentwvasynthetic_ );
}


void uiStratSynthDisp::doModelChange()
{
    MouseCursorChanger mcs( MouseCursor::Busy );

    d2tmodels_ = 0;
    if ( !autoupdate_ ) return;
    
    if ( mStratSynth->errMsg() )
	mErrRet( mStratSynth->errMsg(), return )
    if ( mStratSynth->infoMsg() )
    {
	uiMsgMainWinSetter mws( mainwin() );
	uiMSG().warning( mStratSynth->infoMsg() );
    }

    updateSyntheticList( true );
    updateSyntheticList( false );
    setCurrentSynthetic( true );
    setCurrentSynthetic( false );

    updateFields();
    displaySynthetic( currentwvasynthetic_ );
    displayPostStackSynthetic( currentvdsynthetic_, false );
}


void uiStratSynthDisp::updateSynthetic( const char* synthnm, bool wva )
{
    FixedString syntheticnm( synthnm );
    uiComboBox* datalist = wva ? wvadatalist_ : vddatalist_;
    if ( !datalist->isPresent(syntheticnm) )
	return;
    mStratSynth->removeSynthetic( syntheticnm );
    SyntheticData* sd = mStratSynth->addSynthetic();
    if ( !sd )
	mErrRet(mStratSynth->errMsg(), return );
    updateSyntheticList( wva );
    synthsChanged.trigger();

    datalist->setCurrentItem( syntheticnm );
    setCurrentSynthetic( wva );
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
	syntheticnm = wvadatalist_->text();

    const BufferString curvdsynthnm( currentvdsynthetic_->name().buf() );
    const BufferString curwvasynthnm( currentwvasynthetic_->name().buf() );
    SyntheticData* cursd = mStratSynth->getSynthetic( syntheticnm );
    SynthGenParams curgp;
    cursd->fillGenParams( curgp );
    if ( !(curgp == mStratSynth->genParams()) )
    {
	updateSynthetic( syntheticnm, true );
	updateSyntheticList( false );
    }
    else
    {
	wvadatalist_->setCurrentItem( syntheticnm );
	setCurrentSynthetic( true );
    }

    displaySynthetic( currentwvasynthetic_ );
    if ( curwvasynthnm == curvdsynthnm )
    {
	vddatalist_->setCurrentItem( syntheticnm );
	setCurrentSynthetic( false );
	displayPostStackSynthetic( currentvdsynthetic_, false );
    }
}


void uiStratSynthDisp::syntheticRemoved( CallBacker* cb )
{
    mCBCapsuleUnpack(BufferString,synthname,cb);
    mStratSynth->removeSynthetic( synthname );
    synthsChanged.trigger();
    updateSyntheticList( true );
    updateSyntheticList( false );
    setCurrentSynthetic( true );
    setCurrentSynthetic( false );
    displayPostStackSynthetic( currentwvasynthetic_, true );
    displayPostStackSynthetic( currentvdsynthetic_, false );
}


void uiStratSynthDisp::addEditSynth( CallBacker* )
{
    if ( !synthgendlg_ )
    {
	synthgendlg_ = new uiSynthGenDlg( this, *mStratSynth);
	synthgendlg_->synthRemoved.notify(
		mCB(this,uiStratSynthDisp,syntheticRemoved) );
	synthgendlg_->synthChanged.notify(
		mCB(this,uiStratSynthDisp,syntheticChanged) );
	synthgendlg_->genNewReq.notify(
			    mCB(this,uiStratSynthDisp,genNewSynthetic) );
    }

    synthgendlg_->updateSynthNames();
    synthgendlg_->putToScreen();
    synthgendlg_->go();
}


void uiStratSynthDisp::exportSynth( CallBacker* )
{
    uiMsgMainWinSetter mws( mainwin() );
    uiMSG().error( "TODO: implement" );
}


void uiStratSynthDisp::offsetChged( CallBacker* )
{
    displayPostStackSynthetic( currentwvasynthetic_, true );
    if ( !strcmp(wvadatalist_->text(),vddatalist_->text()) &&
	 currentwvasynthetic_ && currentwvasynthetic_->isPS() )
	displayPostStackSynthetic( currentvdsynthetic_, false );
}


const SeisTrcBuf& uiStratSynthDisp::postStackTraces(const PropertyRef* pr) const
{
    SyntheticData* sd = pr ? mStratSynth->getSynthetic(*pr)
			   : currentwvasynthetic_;

    static SeisTrcBuf emptytb( true );
    if ( !sd || sd->isPS() ) return emptytb;

    const DataPack& dp = sd->getPack();
    mDynamicCastGet(const SeisTrcBufDataPack*,stbp,&dp);
    if ( !stbp ) return emptytb;

    if ( !sd->d2tmodels_.isEmpty() )
    {
	SeisTrcBuf& tbuf = const_cast<SeisTrcBuf&>( stbp->trcBuf() );
	mStratSynth->snapLevelTimes( tbuf, sd->d2tmodels_ );
    }
    return stbp->trcBuf();
}


const PropertyRefSelection& uiStratSynthDisp::modelPropertyRefs() const
{ return layerModel().propertyRefs(); }


const ObjectSet<const TimeDepthModel>* uiStratSynthDisp::d2TModels() const
{ return d2tmodels_; }


void uiStratSynthDisp::vdDataSetSel( CallBacker* )
{
    setCurrentSynthetic( false );
    displayPostStackSynthetic( currentvdsynthetic_, false );
}


void uiStratSynthDisp::wvDataSetSel( CallBacker* )
{
    setCurrentSynthetic( true );
    displayPostStackSynthetic( currentwvasynthetic_, true );
    updateFields();
    //TODO check if it works doModelChange();
}


const ObjectSet<SyntheticData>& uiStratSynthDisp::getSynthetics() const
{
    return mStratSynth->synthetics();
}


const Wavelet* uiStratSynthDisp::getWavelet() const
{
    return mStratSynth->wavelet();
}


const MultiID& uiStratSynthDisp::waveletID() const
{
    return wvltfld_->getID();
}


void uiStratSynthDisp::genNewSynthetic( CallBacker* )
{
    if ( !synthgendlg_ ) 
	return;

    MouseCursorChanger mcchger( MouseCursor::Wait );
    SyntheticData* sd = mStratSynth->addSynthetic();
    if ( !sd )
	mErrRet(mStratSynth->errMsg(), return )
    updateSyntheticList( true );
    updateSyntheticList( false );
    synthsChanged.trigger();
    synthgendlg_->putToScreen();
    synthgendlg_->updateSynthNames();
}


SyntheticData* uiStratSynthDisp::getCurrentSyntheticData( bool wva ) const
{
    return wva ? currentwvasynthetic_ : currentvdsynthetic_; 
}


void uiStratSynthDisp::fillPar( IOPar& par, const StratSynth* stratsynth ) const
{
    IOPar stratsynthpar;
    stratsynthpar.set( sKeySnapLevel(), levelsnapselfld_->currentItem());
    int nr_nonproprefsynths = 0;
    for ( int idx=0; idx<stratsynth->nrSynthetics(); idx++ )
    {
	const SyntheticData* sd = stratsynth->getSyntheticByIdx( idx );
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
    par.removeWithKey( sKeySynthetics() );
    par.mergeComp( stratsynthpar, sKeySynthetics() );
}


void uiStratSynthDisp::fillPar( IOPar& par, bool useed ) const
{
    StratSynth* stratsynth = useed ? edstratsynth_ : stratsynth_;
    fillPar( par, stratsynth );
}


void uiStratSynthDisp::fillPar( IOPar& par ) const
{
    fillPar( par, mStratSynth );
}


bool uiStratSynthDisp::prepareElasticModel()
{
    return mStratSynth->createElasticModels();
}


bool uiStratSynthDisp::usePar( const IOPar& par ) 
{
    PtrMan<IOPar> stratsynthpar = par.subselect( sKeySynthetics() );
    if ( !mStratSynth->hasElasticModels() )
	return false;
    if ( !stratsynthpar )
	mStratSynth->addDefaultSynthetic();
    else
    {
	int nrsynths;
	stratsynthpar->get( sKeyNrSynthetics(), nrsynths );
	mStratSynth->clearSynthetics();
	currentvdsynthetic_ = 0;
	currentwvasynthetic_ = 0;
	wvadatalist_->setEmpty();
	vddatalist_->setEmpty();
	for ( int idx=0; idx<nrsynths; idx++ )
	{
	    PtrMan<IOPar> synthpar =
		stratsynthpar->subselect(IOPar::compKey(sKeySyntheticNr(),idx));
	    if ( !synthpar ) continue;
	    SynthGenParams genparams;
	    genparams.usePar( *synthpar );
	    wvltfld_->setInput( genparams.wvltnm_ );
	    mStratSynth->setWavelet( wvltfld_->getWavelet() );
	    SyntheticData* sd = mStratSynth->addSynthetic( genparams );
	    if ( !sd )
	    {
		mErrRet(mStratSynth->errMsg(),);
		continue;
	    }

	    wvadatalist_->addItem( sd->name() );
	}

	if ( !nrsynths )
	    mStratSynth->addDefaultSynthetic();
    }

    if ( !mStratSynth->nrSynthetics() )
    {
	displaySynthetic( 0 );
	displayPostStackSynthetic( 0, false );
	return false;
    }

    mStratSynth->generateOtherQuantities();
    synthsChanged.trigger();
    
    if ( stratsynthpar )
    {
	int snaplvl = 0;
	stratsynthpar->get( sKeySnapLevel(), snaplvl );
	levelsnapselfld_->setCurrentItem( snaplvl );
    }

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
