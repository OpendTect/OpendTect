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
    , synthsChanged(this)		       
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
    
    uiLabeledComboBox* datalblcbx =
	new uiLabeledComboBox( topgrp_, "Wiggle View", "" );
    wvadatalist_ = datalblcbx->box();
    wvadatalist_->selectionChanged.notify(
	    mCB(this,uiStratSynthDisp,wvDataSetSel) );
    wvadatalist_->setHSzPol( uiObject::Wide );

    addeditbut_ = new uiToolButton( topgrp_, "edit", 
	    			"Add/Edit Synthetic DataSet",
				mCB(this,uiStratSynthDisp,addEditSynth) );

    addeditbut_->attach( leftOf, datalblcbx );

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
    stratsynth_.setWavelet( wvltfld_->getWavelet() );

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
    wvadatalist_->setEmpty();
    vddatalist_->setEmpty();
}


void uiStratSynthDisp::updateVDSyntheticList()
{
    BufferString curitem = vddatalist_->text();
    vddatalist_->setEmpty();
    
    for ( int idx=0; idx<stratsynth_.nrSynthetics(); idx ++)
    {
	const SyntheticData* sd = stratsynth_.getSyntheticByIdx( idx );
	if ( !sd ) continue;
	vddatalist_->addItem( sd->name() );
    }

    vddatalist_->setCurrentItem( curitem );
}


void uiStratSynthDisp::setCurrentVDSynthetic()
{
    const SyntheticData* sd = stratsynth_.getSynthetic( vddatalist_->text() );
    if ( !sd ) return;
    const bool hadpack = vwr_->pack( false ); 
    if ( hadpack )
	vwr_->removePack( vwr_->packID(false) ); 

    mDynamicCastGet(const PreStackSyntheticData*,presd,sd);
    mDynamicCastGet(const PostStackSyntheticData*,postsd,sd);
    const float offset = mCast( float, offsetposfld_->getValue() );
    const SeisTrcBuf* tbuf = presd ? presd->getTrcBuf( offset, 0 ) 
				   : &postsd->postStackPack().trcBuf();
    if ( !tbuf ) return;

    SeisTrcBuf* disptbuf = new SeisTrcBuf( true );
    tbuf->copyInto( *disptbuf );
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

    vwr_->setPack( false, dp->id(), false, !hadpack );
    ColTab::MapperSetup& mapper = vwr_->appearance().ddpars_.vd_.mappersetup_;
    mapper.cliprate_ = Interval<float>(0.0,0.0);
    mapper.autosym0_ = true;
    
    mDynamicCastGet(const PropertyRefSyntheticData*,prsd,sd);
    mapper.symmidval_ = prsd ? mUdf(float) : 0;
    vwr_->handleChange( FlatView::Viewer::DisplayPars );
}


void uiStratSynthDisp::updateWVASyntheticList()
{
    BufferString curitem = wvadatalist_->text();
    wvadatalist_->setEmpty();
    for ( int idx=0; idx<stratsynth_.nrSynthetics(); idx ++)
    {
	const SyntheticData* sd = stratsynth_.getSyntheticByIdx( idx );
	if ( !sd ) continue;

	mDynamicCastGet(const PropertyRefSyntheticData*,prsd,sd);
	if ( prsd ) continue;
	wvadatalist_->addItem( sd->name() );
    }

    wvadatalist_->setCurrentItem( curitem );
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
    StratSynth::Level* lvl = new StratSynth::Level( lnm, zvals, col );
    stratsynth_.setLevel( lvl );

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
    VSEvent::parseEnumType( levelsnapselfld_->text(), tp );
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

    vwr_->handleChange( FlatView::Viewer::Annot, true );
}


void uiStratSynthDisp::setCurrentWavelet()
{
    currentsynthetic_ = 0;
    stratsynth_.setWavelet( wvltfld_->getWavelet() );
    SyntheticData* sd = stratsynth_.getSynthetic( wvadatalist_->text() );
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
    const bool hadpack = vwr_->pack( true ); 
    if ( hadpack )
	vwr_->removePack( vwr_->packID(true) ); 
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


    vwr_->setPack( true, dp->id(), false, !hadpack );
    
    ColTab::MapperSetup& mapper = vwr_->appearance().ddpars_.wva_.mappersetup_;
    mapper.cliprate_ = Interval<float>(0.0,0.0);
    mapper.autosym0_ = true;
    mapper.symmidval_ = 0.0;

    vwr_->handleChange( FlatView::Viewer::DisplayPars );

    FlatView::AuxData* filltxtdata =
	vwr_->createAuxData( isbrinefilled_ ? "Brine filled"
					    : "Hydrocarbon filled" );
    filltxtdata->namepos_ = 0;
    uiWorldPoint txtpos =
	vwr_->boundingBox().bottomRight() - uiWorldPoint(10,10);
    filltxtdata->poly_ += txtpos;

    vwr_->addAuxData( filltxtdata );
    levelSnapChanged( 0 );
}


void uiStratSynthDisp::displayPreStackDirSynthetic( const SyntheticData* sd )
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


void uiStratSynthDisp::setCurrentWVASynthetic()
{
    SyntheticData* sd = 0;
    if ( stratsynth_.nrSynthetics() == 0 )
    {
	sd = stratsynth_.addDefaultSynthetic();
	synthsChanged.trigger();
	updateWVASyntheticList();
	updateVDSyntheticList();
    }
    else
	sd = stratsynth_.getSynthetic( wvadatalist_->text() );

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

    setCurrentWVASynthetic();

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
    const Interval<double> xrg = vwr_->getDataPackRange( true );
    const Interval<double> zrg = vwr_->getDataPackRange( false );
    vwr_->setSelDataRanges( xrg, zrg ); 
    uiWorldRect wr( xrg.start, zrg.stop, xrg.stop, zrg.start );
    vwr_->setView( wr );
    drawLevel();

    updateVDSyntheticList();
    setCurrentVDSynthetic();
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

    if ( wvadatalist_->isPresent(syntheticnm) )
    {
	wvadatalist_->setCurrentItem( syntheticnm );
	stratsynth_.removeSynthetic( syntheticnm );
	SyntheticData* sd = stratsynth_.addSynthetic();
	if ( !sd )
	    mErrRet(stratsynth_.errMsg(), return );
	synthsChanged.trigger();
	updateWVASyntheticList();
	updateVDSyntheticList();
	setCurrentWVASynthetic();
	displaySynthetic( currentsynthetic_ );
    }
}


void uiStratSynthDisp::syntheticRemoved( CallBacker* cb )
{
    mCBCapsuleUnpack(BufferString,synthname,cb);
    stratsynth_.removeSynthetic( synthname );
    synthsChanged.trigger();
    if ( wvadatalist_->isPresent(synthname) )
    {
	wvadatalist_->setEmpty();
	
	for ( int idx=0; idx<stratsynth_.nrSynthetics(); idx ++)
	{
	    const SyntheticData* sd = stratsynth_.getSyntheticByIdx( idx );
	    if ( !sd ) continue;
	    mDynamicCastGet(const PropertyRefSyntheticData*,prsd,sd);
	    if ( prsd ) continue;
	    wvadatalist_->addItem( sd->name() );
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

    synthgendlg_->updateSynthNames();
    synthgendlg_->putToScreen();
    synthgendlg_->go();
}


void uiStratSynthDisp::offsetChged( CallBacker* )
{
    displayPostStackDirSynthetic( currentsynthetic_ );
    if ( !strcmp(wvadatalist_->text(),vddatalist_->text()) &&
	 currentsynthetic_ && currentsynthetic_->isPS() )
	setCurrentVDSynthetic();
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


void uiStratSynthDisp::vdDataSetSel( CallBacker* )
{
    setCurrentVDSynthetic();
}


void uiStratSynthDisp::wvDataSetSel( CallBacker* )
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
	updateWVASyntheticList();
	updateVDSyntheticList();
	synthsChanged.trigger();
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
    stratsynthpar.set( sKeySnapLevel(), levelsnapselfld_->currentItem());
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
    par.removeWithKey( sKeySynthetics() );
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
    wvadatalist_->setEmpty();
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

	wvadatalist_->addItem( sd->name() );
    }

    if ( !stratsynth_.nrSynthetics() )
    {
	displaySynthetic( 0 );
	return false;
    }

    stratsynth_.generateOtherQuantities();
    synthsChanged.trigger();
    setCurrentWVASynthetic();
    displaySynthetic( currentsynthetic_ );
    setCurrentVDSynthetic();
    levelsnapselfld_->setCurrentItem( snaplvl );

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
    BufferStringSet types( SynthGenParams::SynthTypeNames() );
    uiLabeledComboBox* lblcbx =
	new uiLabeledComboBox( pargrp, types, "Synthethic type" );
    typefld_ = lblcbx->box();
    typefld_->selectionChanged.notify( mCB(this,uiSynthGenDlg,typeChg) );

    psselfld_ = new uiLabeledComboBox( pargrp, "Input PreStack" );
    psselfld_->attach( alignedBelow, lblcbx );

    uiRayTracer1D::Setup rsu; rsu.dooffsets_ = true;
    rtsel_ = new uiRayTracerSel( pargrp, rsu );
    rtsel_->usePar( stratsynth_.genParams().raypars_ ); 
    rtsel_->attach( alignedBelow, psselfld_ );
    rtsel_->offsetChanged.notify( mCB(this,uiSynthGenDlg,parsChanged) );

    wvltfld_ = new uiSeisWaveletSel( pargrp );
    wvltfld_->newSelection.notify( mCB(this,uiSynthGenDlg,parsChanged) );
    wvltfld_->attach( alignedBelow, rtsel_ );
    wvltfld_->setFrame( false );
    
    nmofld_ = new uiGenInput( pargrp, "Apply NMO corrections",
			      BoolInpSpec(true) );
    mAttachCB( nmofld_->valuechanged, uiSynthGenDlg, parsChanged);
    nmofld_->attach( alignedBelow, wvltfld_ );
    
    FloatInpSpec inpspec;
    inpspec.setLimits( Interval<float>(1,500) );
    stretchmutelimitfld_ = new uiGenInput(pargrp, "Stretch mute (%)", inpspec );
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


void uiSynthGenDlg::getPSNames( BufferStringSet& synthnms ) 
{
    synthnms.erase();

    for ( int synthidx=0; synthidx<stratsynth_.nrSynthetics(); synthidx++ )
    {
	SynthGenParams genparams;
	SyntheticData* synth = stratsynth_.getSyntheticByIdx( synthidx );
	if ( !synth ) continue;
	synth->fillGenParams( genparams );
	if ( !genparams.isPreStack() ) continue;
	bool donmo = false;
	genparams.raypars_.getYN( Seis::SynthGenBase::sKeyNMO(),donmo );
	if ( !donmo ) continue;
	synthnms.add( genparams.name_ );
    }
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

    SynthGenParams cursgp = stratsynth_.genParams();
    int nrofzerooffs = 0;
    for ( int idx=0; idx<stratsynth_.nrSynthetics(); idx++ )
    {
	SyntheticData* sd = stratsynth_.getSyntheticByIdx( idx );
	if ( !sd ) continue;
	SynthGenParams sgp;
	sd->fillGenParams( sgp );
	if ( sgp.synthtype_ == SynthGenParams::ZeroOffset )
	{
	    nrofzerooffs++;
	    continue;
	}
	if ( cursgp.isPreStack() &&
	     sgp.synthtype_ == SynthGenParams::AngleStack &&
	     sgp.inpsynthnm_ == cursgp.name_ )
	{
	    BufferString msg( sgp.name_.buf(), "will also be removed as "
					       "it is dependent on ",
			      cursgp.name_.buf() );
	    msg += "Do you want to remove the synthetics?";
	    if ( !uiMSG().askGoOn(msg) )
		return;
	    BufferString synthname( sgp.name_ );
	    synthnmlb_->removeItem( synthnmlb_->currentItem() );
	    synthRemoved.trigger( synthname );
	    break;
	}
    }

    if ( cursgp.synthtype_ == SynthGenParams::ZeroOffset && nrofzerooffs<=1 )
    {
	BufferString msg( "Cannot remove ", cursgp.name_.buf(),
			  " as there should be atleast one 0 offset synthetic");
	return uiMSG().error( msg );
    }

    BufferString synthname( synthnmlb_->getText() );
    synthnmlb_->removeItem( synthnmlb_->currentItem() );
    synthRemoved.trigger( synthname );
}


void uiSynthGenDlg::updateFieldSensitivity()
{
    SynthGenParams::SynthType synthtype =
	(SynthGenParams::SynthType)typefld_->currentItem();
    const bool isps = synthtype==SynthGenParams::PreStack;
    const bool isas = synthtype==SynthGenParams::AngleStack;
    nmofld_->display( isps );
    rtsel_->display( isps );
    rtsel_->current()->displayOffsetFlds( isps || isas );
    rtsel_->current()->setOffsetRange( uiRayTracer1D::Setup().offsetrg_ );
    psselfld_->display( isas );
    wvltfld_->display( !isas );
    mutelenfld_->display( isps );
    stretchmutelimitfld_->display( isps );
}


void uiSynthGenDlg::typeChg( CallBacker* )
{
    updateFieldSensitivity();
    stratsynth_.genParams().synthtype_ =
	(SynthGenParams::SynthType)typefld_->currentItem();
    stratsynth_.genParams().raypars_.setEmpty();
    if ( typefld_->currentItem()==1 )
	rtsel_->setCurrent( 0 );
    rtsel_->fillPar( stratsynth_.genParams().raypars_ );
    putToScreen();
    BufferString nm;
    stratsynth_.genParams().createName( nm );
    namefld_->setText( nm );
}


void uiSynthGenDlg::putToScreen()
{
    const SynthGenParams& genparams = stratsynth_.genParams();
    wvltfld_->setInput( genparams.wvltnm_ );
    namefld_->setText( genparams.name_ );
    
    const bool isps = genparams.isPreStack();
    typefld_->setCurrentItem( (int)genparams.synthtype_ );

    if ( genparams.synthtype_ == SynthGenParams::AngleStack )
    {
	BufferStringSet psnms;
	getPSNames( psnms );
	psselfld_->box()->setEmpty();
	psselfld_->box()->addItems( psnms );
	psselfld_->box()->setCurrentItem( genparams.inpsynthnm_ );
	updateFieldSensitivity();
	return;
    }

    TypeSet<float> offsets;
    genparams.raypars_.get( RayTracer1D::sKeyOffset(), offsets );
        
    bool donmo = false;
    genparams.raypars_.getYN( Seis::SynthGenBase::sKeyNMO(), donmo );
    nmofld_->setValue( donmo );
    
    if ( isps )
    {
	float mutelen = Seis::SynthGenBase::cStdMuteLength();
	genparams.raypars_.get(
		Seis::SynthGenBase::sKeyMuteLength(), mutelen );
	mutelenfld_->setValue(
	    mIsUdf(mutelen) ? mutelen : mutelen *ZDomain::Time().userFactor());
	
	float stretchlimit = Seis::SynthGenBase::cStdStretchLimit();
	genparams.raypars_.get(
			Seis::SynthGenBase::sKeyStretchLimit(), stretchlimit );
	stretchmutelimitfld_->setValue( mToPercent( stretchlimit ) );
    }

    rtsel_->usePar( genparams.raypars_ );
    updateFieldSensitivity();
}


bool uiSynthGenDlg::getFromScreen() 
{
    const char* nm = namefld_->text(); 
    if ( !nm )
	mErrRet("Please specify a valid name",return false);
    
    if ( mutelenfld_->attachObj()->isDisplayed() &&
	 (mIsUdf(mutelenfld_->getfValue() ) || mutelenfld_->getfValue()<0) )
	mErrRet( "The mutelength must be more than zero.", return false );
    
    if ( stretchmutelimitfld_->attachObj()->isDisplayed() &&
	(mIsUdf(stretchmutelimitfld_->getfValue()) ||
	 stretchmutelimitfld_->getfValue()<0) )
	mErrRet( "The stretch mute must be more than 0%", return false );
    
    stratsynth_.genParams().raypars_.setEmpty();
    
    SynthGenParams& genparams = stratsynth_.genParams();
    genparams.synthtype_ = (SynthGenParams::SynthType)typefld_->currentItem(); 
    const bool isps = !typefld_->currentItem();

    if ( genparams.synthtype_ == SynthGenParams::AngleStack )
    {
	if ( psselfld_->box()->isEmpty() )
	    mErrRet( "Cannot generate an angle stack synthetics without any "
		     "NMO corrected PreStack.", return false );
	SyntheticData* inppssd = stratsynth_.getSynthetic(
		psselfld_->box()->textOfItem(psselfld_->box()->currentItem()) );
	if ( !inppssd )
	    mErrRet("Problem with Input Pre-Stack synthetic data",return false);
	
	inppssd->fillGenParams( genparams );
	genparams.name_ = nm;
	genparams.synthtype_ = SynthGenParams::AngleStack;
	genparams.inpsynthnm_ = inppssd->name();
	return true;
    }

    rtsel_->fillPar( genparams.raypars_ );

    if ( !isps )
	RayTracer1D::setIOParsToZeroOffset( genparams.raypars_ );

    genparams.wvltnm_ = wvltfld_->getName();
    stratsynth_.setWavelet( wvltfld_->getWavelet() );
    bool donmo = isps ? nmofld_->getBoolValue() : false;
    genparams.raypars_.setYN( Seis::SynthGenBase::sKeyNMO(), donmo );
    genparams.name_ = namefld_->text();
    
    if ( isps )
    {
	genparams.raypars_.set( Seis::SynthGenBase::sKeyMuteLength(),
		     mutelenfld_->getfValue() / ZDomain::Time().userFactor() );
	genparams.raypars_.set(
		Seis::SynthGenBase::sKeyStretchLimit(),
		mFromPercent( stretchmutelimitfld_->getfValue()) );
    }

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
    if ( selidx < 0 ) return false;
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

