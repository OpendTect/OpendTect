/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/

#include "uistratsynthdisp.h"

#include "uicombobox.h"
#include "uiflatviewer.h"
#include "uiflatviewmainwin.h"
#include "uiflatviewslicepos.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uimultiflatviewcontrol.h"
#include "uipsviewer2dmainwin.h"
#include "uiseiswvltsel.h"
#include "uispinbox.h"
#include "uistratlayermodel.h"
#include "uistratsynthexport.h"
#include "uisynthgendlg.h"
#include "uisynthtorealscale.h"
#include "uitaskrunner.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"

#include "envvars.h"
#include "flatposdata.h"
#include "flatviewaxesdrawer.h"
#include "flatviewzoommgr.h"
#include "prestackgather.h"
#include "propertyref.h"
#include "ptrman.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "stratlayer.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "stratlith.h"
#include "stratsynth.h"
#include "stratsynthlevel.h"
#include "survinfo.h"
#include "syntheticdataimpl.h"
#include "waveletio.h"


static const int cMarkerSize = 6;

static const char* sKeySnapLevel()	{ return "Snap Level"; }
static const char* sKeyViewArea()	{ return "Start View Area"; }
static const char* sKeyNone()		{ return "None"; }
static const char* sKeyRainbow()	{ return "Rainbow"; }
static const char* sKeySeismics()	{ return "Seismics"; }
static const char* sKeyDecimation()	{ return "Decimation"; }


uiStratSynthDisp::uiStratSynthDisp( uiParent* p,
				    const Strat::LayerModelProvider& lmp )
    : uiGroup(p,"LayerModel synthetics display")
    , lmp_(lmp)
    , stratsynth_(new StratSynth(lmp,false))
    , edstratsynth_(new StratSynth(lmp,true))
    , viewChanged(this)
    , modSelChanged(this)
    , synthsChanged(this)
    , dispParsChanged(this)
    , layerPropSelNeeded(this)
    , relzoomwr_(0,0,1,1)
    , savedzoomwr_(mUdf(double),0,0,0)
{
    topgrp_ = new uiGroup( this, "Top group" );
    topgrp_->setStretch( 2, 0 );

    auto* datalblcbx = new uiLabeledComboBox( topgrp_, tr("Wiggle View"), "" );
    wvadatalist_ = datalblcbx->box();
    mAttachCB( wvadatalist_->selectionChanged, uiStratSynthDisp::wvDataSetSel);
    wvadatalist_->setHSzPol( uiObject::Wide );

    auto* edbut = new uiToolButton( topgrp_, "edit",
				tr("Add/Edit Synthetic DataSet"),
				mCB(this,uiStratSynthDisp,addEditSynth) );

    edbut->attach( leftOf, datalblcbx );

    auto* dataselgrp = new uiGroup( this, "Data Selection" );
    dataselgrp->attach( rightBorder );
    dataselgrp->attach( ensureRightOf, topgrp_ );

    auto* prdatalblcbx =
	new uiLabeledComboBox( dataselgrp, tr("Variable Density View"), "" );
    vddatalist_ = prdatalblcbx->box();
    mAttachCB( vddatalist_->selectionChanged, uiStratSynthDisp::vdDataSetSel );
    vddatalist_->setHSzPol( uiObject::Wide );
    prdatalblcbx->attach( leftBorder );

    auto* expbut = new uiToolButton( prdatalblcbx, "export",
			uiStrings::phrExport( tr("Synthetic DataSet(s)")),
			mCB(this,uiStratSynthDisp,exportSynth) );
    expbut->attach( rightOf, vddatalist_ );

    datagrp_ = new uiGroup( this, "DataSet group" );
    datagrp_->attach( ensureBelow, topgrp_ );
    datagrp_->attach( ensureBelow, dataselgrp );
    datagrp_->setFrame( true );
    datagrp_->setStretch( 2, 0 );

    scalebut_ = new uiPushButton( datagrp_, tr("Scale"),
				  mCB(this,uiStratSynthDisp,scalePush), false );

    auto* lvlsnapcbx = new uiLabeledComboBox( datagrp_, VSEvent::TypeNames(),
					      tr("Snap level") );
    levelsnapselfld_ = lvlsnapcbx->box();
    lvlsnapcbx->attach( rightOf, scalebut_ );
    lvlsnapcbx->setStretch( 2, 0 );
    mAttachCB( levelsnapselfld_->selectionChanged,
	       uiStratSynthDisp::levelSnapChanged );

    prestackgrp_ = new uiGroup( datagrp_, "Prestack View Group" );
    prestackgrp_->attach( rightOf, lvlsnapcbx, 20 );

    offsetposfld_ = new uiSynthSlicePos( prestackgrp_, uiStrings::sOffset() );
    mAttachCB( offsetposfld_->positionChg, uiStratSynthDisp::offsetChged );

    prestackbut_ = new uiToolButton( prestackgrp_, "nonmocorr64",
				tr("View Offset Direction"),
				mCB(this,uiStratSynthDisp,viewPreStackPush) );
    prestackbut_->attach( rightOf, offsetposfld_);

    vwr_ = new uiFlatViewer( this );
    vwr_->rgbCanvas().disableImageSave();
    vwr_->setInitialSize( uiSize(800,300) ); //TODO get hor sz from laymod disp

    const int fontheight = vwr_->getAxesDrawer().getNeededHeight();
    vwr_->setExtraBorders( uiSize(0,-fontheight), uiSize(0,-fontheight) );

    vwr_->setStretch( 2, 2 );
    vwr_->attach( ensureBelow, datagrp_ );
    mAttachCB( vwr_->dispPropChanged, uiStratSynthDisp::parsChangedCB );
    mAttachCB( vwr_->viewChanged, uiStratSynthDisp::viewChg );
    mAttachCB( vwr_->rgbCanvas().reSize, uiStratSynthDisp::updateTextPosCB );
    setDefaultAppearance( vwr_->appearance() );

    uiFlatViewStdControl::Setup fvsu( this );
    fvsu.withcoltabed(false).tba((int)uiToolBar::Right)
	.withflip(false).withsnapshot(false);
    control_ = new uiMultiFlatViewControl( *vwr_, fvsu );
    control_->setViewerType( vwr_, true );
}


uiStratSynthDisp::~uiStratSynthDisp()
{
    detachAllNotifiers();
    delete stratsynth_;
    delete edstratsynth_;
    taskrunner_.release();
    delete d2tmodels_;
}


void uiStratSynthDisp::set( uiStratLayerModel& uislm )
{
    mAttachCB( uislm.newModels, uiStratSynthDisp::newModelsCB );
}


void uiStratSynthDisp::newModelsCB( CallBacker* )
{
    doModelChange();
}


void uiStratSynthDisp::makeInfoMsg( BufferString& mesg, IOPar& pars )
{
    FixedString valstr = pars.find( sKey::TraceNr() );
    int modelidx = 0;
    if ( valstr.isEmpty() )
	return;
    modelidx = toInt(valstr)-1;
    BufferString modelnrstr( 24, true );
    od_sprintf( modelnrstr.getCStr(), modelnrstr.bufSize(),
		"Model Number:%5d", modelidx+1 );
    mesg.add( modelnrstr );
    valstr = pars.find( "Z" );
    if ( !valstr ) valstr = pars.find( "Z-Coord" );
    float zval = mUdf(float);
    if ( valstr )
    {
	BufferString depthstr( 16, true );
	zval = toFloat( valstr );
	od_sprintf( depthstr.getCStr(), depthstr.bufSize(),
		   "Depth : %6.0f", zval );
	depthstr.add( SI().getZUnitString() );
	mesg.addSpace().add( depthstr );
    }

    if ( modelidx<0 || layerModel().size()<=modelidx || mIsUdf(zval) )
	return;

    mesg.addSpace();
    int nrinfos = 0;
#define mAddSep() if ( nrinfos++ ) mesg += ";\t";

    FixedString vdstr = pars.find( "Variable density data" );
    FixedString wvastr = pars.find( "Wiggle/VA data" );
    FixedString vdvalstr = pars.find( "VD Value" );
    FixedString wvavalstr = pars.find( "WVA Value" );
    const bool issame = vdstr && wvastr && vdstr==wvastr;
    if ( vdvalstr )
    {
	mAddSep();
	if ( issame )
	    { if ( vdstr.isEmpty() ) vdstr = wvastr; }
	else
	    { if ( vdstr.isEmpty() ) vdstr = "VD Val"; }
	float val = !vdvalstr.isEmpty() ? vdvalstr.toFloat() : mUdf(float);
	mesg += "Val="; mesg += mIsUdf(val) ? "undef" : vdvalstr.buf();
	mesg += " ("; mesg += vdstr; mesg += ")";
    }
    if ( wvavalstr && !issame )
    {
	mAddSep();
	float val = !wvavalstr.isEmpty() ? wvavalstr.toFloat() : mUdf(float);
	mesg += "Val="; mesg += mIsUdf(val) ? "undef" : wvavalstr.buf();
	if ( wvastr.isEmpty() ) wvastr = "WVA Val";
	mesg += " ("; mesg += wvastr; mesg += ")";
    }

    float val;
    if ( pars.get(sKey::Offset(),val) )
    {
	mAddSep(); mesg += "Offs="; mesg += val;
	mesg += " "; mesg += SI().getXYUnitString();
    }

    if ( d2tmodels_ && d2tmodels_->validIdx(modelidx) )
    {
	zval /= SI().showZ2UserFactor();
	const float depth = (*d2tmodels_)[modelidx]->getDepth( zval );
	const Strat::LayerSequence& curseq = layerModel().sequence( modelidx );
	for ( int lidx=0; lidx<curseq.size(); lidx++ )
	{
	    const Strat::Layer* layer = curseq.layers()[lidx];
	    if ( layer->zTop()<=depth && layer->zBot()>depth )
	    {
		mesg.addTab().add( "Layer:" ).add( layer->name() );
		mesg.add( "; Lithology:").add( layer->lithology().name() );
		if ( !layer->content().isUnspecified() )
		    mesg.add( "; Content:").add( layer->content().name() );
		break;
	    }
	}
    }
}


void uiStratSynthDisp::addViewerToControl( uiFlatViewer& vwr )
{
    if ( control_ )
    {
	control_->addViewer( vwr );
	control_->setViewerType( &vwr, false );
    }
}


const Strat::LayerModel& uiStratSynthDisp::layerModel() const
{
    return lmp_.getCurrent();
}


void uiStratSynthDisp::layerPropsPush( CallBacker* )
{
    layerPropSelNeeded.trigger();
}


void uiStratSynthDisp::addTool( const uiToolButtonSetup& bsu )
{
    uiButton* tb = bsu.getButton( datagrp_ );
    if ( lasttool_ )
	tb->attach( leftOf, lasttool_ );
    else
	tb->attach( rightBorder );

    tb->attach( ensureRightOf, prestackbut_ );

    lasttool_ = tb;
}


void uiStratSynthDisp::cleanSynthetics()
{
    setCurSynthetic( nullptr, true );
    setCurSynthetic( nullptr, false );
    curSS().clearSynthetics();
    altSS().clearSynthetics();
    deleteAndZeroPtr( d2tmodels_ );
    wvadatalist_->setEmpty();
    vddatalist_->setEmpty();
}


void uiStratSynthDisp::updateSyntheticList( bool wva )
{
    uiComboBox* datalist = wva ? wvadatalist_ : vddatalist_;
    BufferString curitem = datalist->text();
    datalist->setEmpty();
    datalist->addItem( uiStrings::sNone() );
    for ( const auto* sd : curSS().synthetics() )
    {
	if ( wva && sd->isStratProp() )
	    continue;

	datalist->addItem( sd->name() );
    }

    datalist->setCurrentItem( curitem );
}


void uiStratSynthDisp::setDisplayZSkip( float zskip, bool withmodchg )
{
    dispskipz_ = zskip;
    if ( withmodchg )
	doModelChange();
}


void uiStratSynthDisp::setDispEach( int de )
{
    dispeach_ = de;
    ConstRefMan<SyntheticData> currentwvasynthetic = currentwvasynthetic_.get();
    ConstRefMan<SyntheticData> currentvdsynthetic = currentvdsynthetic_.get();
    displayPostStackSynthetic( currentwvasynthetic, true );
    displayPostStackSynthetic( currentvdsynthetic, false );
}


void uiStratSynthDisp::setSelectedTrace( int st )
{
    selectedtrace_ = st;

    delete vwr_->removeAuxData( selectedtraceaux_ );
    selectedtraceaux_ = nullptr;

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
	OD::LineStyle( OD::LineStyle::Dot, 2, OD::Color::DgbColor() );

    vwr_->handleChange( mCast(unsigned int,FlatView::Viewer::Auxdata) );
}



void uiStratSynthDisp::handleFlattenChange()
{
    resetRelativeViewRect();
    doModelChange();
    control_->zoomMgr().toStart();
}


void uiStratSynthDisp::setFlattened( bool flattened, bool trigger )
{
    dispflattened_ = flattened;
    control_->setFlattened( flattened );
    if ( trigger )
	handleFlattenChange();
}


void uiStratSynthDisp::setDispMrkrs( const char* lnm,
				    const TypeSet<float>& zvals, OD::Color col )
{
    StratSynthLevel* lvl = new StratSynthLevel( lnm, col, &zvals );
    curSS().setLevel( lvl );
    levelSnapChanged(0);
}


void uiStratSynthDisp::setRelativeViewRect( const uiWorldRect& relwr )
{
    relzoomwr_ = relwr;
    uiWorldRect abswr;
    getAbsoluteViewRect( abswr );
    vwr_->setView( abswr );
}


void uiStratSynthDisp::setAbsoluteViewRect( const uiWorldRect& wr )
{
    uiWorldRect abswr = wr;
    uiWorldRect bbwr = vwr_->boundingBox();
    bbwr.sortCorners();
    abswr.sortCorners();
    if ( mIsZero(bbwr.width(),1e-3) || mIsZero(bbwr.height(),1e-3) ||
	 !bbwr.contains(abswr,1e-3) )
	return;

    const double left = (abswr.left() - bbwr.left())/bbwr.width();
    const double right = (abswr.right() - bbwr.left())/bbwr.width();
    const double top = (abswr.top() - bbwr.top())/bbwr.height();
    const double bottom = (abswr.bottom() - bbwr.top())/bbwr.height();
    relzoomwr_ = uiWorldRect( left, top, right, bottom );
}


void uiStratSynthDisp::getAbsoluteViewRect( uiWorldRect& abswr ) const
{
    uiWorldRect bbwr = vwr_->boundingBox();
    bbwr.sortCorners();
    if ( mIsZero(bbwr.width(),1e-3) || mIsZero(bbwr.height(),1e-3) )
	return;
    const double left = bbwr.left() + relzoomwr_.left()*bbwr.width();
    const double right = bbwr.left() + relzoomwr_.right()*bbwr.width();
    const double top = bbwr.top() + relzoomwr_.top()*bbwr.height();
    const double bottom = bbwr.top() + relzoomwr_.bottom()*bbwr.height();
    abswr = uiWorldRect( left, top, right, bottom );
}


void uiStratSynthDisp::resetRelativeViewRect()
{
    relzoomwr_ = uiWorldRect( 0, 0, 1, 1 );
}


void uiStratSynthDisp::updateRelativeViewRect()
{
    setAbsoluteViewRect( curView(false) );
}


void uiStratSynthDisp::setZoomView( const uiWorldRect& relwr )
{
    relzoomwr_ = relwr;
    uiWorldRect abswr;
    getAbsoluteViewRect( abswr );
    Geom::Point2D<double> centre = abswr.centre();
    Geom::Size2D<double> newsz = abswr.size();
    control_->setActiveVwr( 0 );
    control_->setNewView( centre, newsz, control_->activeVwr() );
}


void uiStratSynthDisp::setZDataRange( const Interval<double>& zrg, bool indpth )
{
    Interval<double> newzrg; newzrg.set( zrg.start, zrg.stop );
    if ( indpth && d2tmodels_ && !d2tmodels_->isEmpty() )
    {
	int mdlidx = longestaimdl_;
	if ( !d2tmodels_->validIdx(mdlidx) )
	    mdlidx = d2tmodels_->size()-1;

	const TimeDepthModel& d2t = *(*d2tmodels_)[mdlidx];
	newzrg.start = d2t.getTime( (float)zrg.start );
	newzrg.stop = d2t.getTime( (float)zrg.stop );
    }
    const Interval<double> xrg = vwr_->getDataPackRange( true );
    vwr_->setSelDataRanges( xrg, newzrg );
    vwr_->handleChange( mCast(unsigned int,FlatView::Viewer::All) );
}


void uiStratSynthDisp::levelSnapChanged( CallBacker* )
{
    const StratSynthLevel* lvl = curSS().getLevel();
    if ( !lvl )
	return;

    auto* edlvl = const_cast<StratSynthLevel*>( lvl );
    VSEvent::Type tp;
    VSEvent::parseEnumType( levelsnapselfld_->text(), tp );
    edlvl->snapev_ = tp;

    ConstRefMan<SyntheticData> currentwvasynthetic = currentwvasynthetic_.get();
    ConstRefMan<SyntheticData> currentvdsynthetic = currentvdsynthetic_.get();
    if ( currentwvasynthetic || currentvdsynthetic )
	drawLevel();
}


const char* uiStratSynthDisp::levelName()  const
{
    const StratSynthLevel* lvl = curSS().getLevel();
    return lvl ? lvl->name().buf() : 0;
}


void uiStratSynthDisp::displayFRText()
{ displayFRText( true, isbrinefilled_ ); }


void uiStratSynthDisp::displayFRText( bool yn, bool isbrine )
{
    if ( !frtxtitm_ )
    {
	uiGraphicsScene& scene = vwr_->rgbCanvas().scene();
	frtxtitm_ = scene.addItem( new uiTextItem );
	frtxtitm_->setAlignment( mAlignment(Left,VCenter) );
	frtxtitm_->setPenColor( OD::Color::Black() );
	frtxtitm_->setZValue( 999999 );
	frtxtitm_->setMovable( true );
    }

    frtxtitm_->setVisible( yn );
    if ( yn )
    {
	frtxtitm_->setText( isbrine ? tr("Brine filled")
				    : tr("Hydrocarbon filled") );
	updateTextPosCB( nullptr );
    }
}


void uiStratSynthDisp::updateTextPosCB( CallBacker* )
{
    if ( !frtxtitm_)
	return;

    frtxtitm_->setPos( 20, 10 );
}


void uiStratSynthDisp::drawLevel()
{
    ConstRefMan<SyntheticData> currentwvasynthetic = currentwvasynthetic_.get();
    ConstRefMan<SyntheticData> currentvdsynthetic = currentvdsynthetic_.get();
    const SyntheticData* synthseis = currentwvasynthetic ? currentwvasynthetic
							 : currentvdsynthetic;
    if ( !synthseis )
	return;

    delete vwr_->removeAuxData( levelaux_ ); levelaux_ = nullptr;

    const Seis::SynthGenDataPack& synthgendp = synthseis->synthGenDP();
    const int offsidx = prestackgrp_->sensitive()
		      ? synthgendp.getOffsetIdx( offsetposfld_->getValue() )
		      : -1;
    const StratSynthLevel* lvl = curSS().getLevel();
    if ( lvl )
    {
	SeisTrcBuf& tbuf = const_cast<SeisTrcBuf&>( curTrcBuf() );
	FlatView::AuxData* auxd = vwr_->createAuxData("Level markers");
	curSS().getLevelTimes( synthgendp.getModels(), dispeach_, tbuf,offsidx);

	auxd->linestyle_.type_ = OD::LineStyle::None;
	for ( int imdl=0; imdl<tbuf.size(); imdl ++ )
	{
	    if ( tbuf.get(imdl)->isNull() )
		continue;
	    const float tval =
		dispflattened_ ? 0.f :	tbuf.get(imdl)->info().pick;

	    auxd->markerstyles_ += MarkerStyle2D( MarkerStyle2D::Target,
						  cMarkerSize, lvl->col_ );
	    auxd->poly_ += FlatView::Point( (imdl*dispeach_)+1, tval );
	}
	if ( auxd->isEmpty() )
	    delete auxd;
	else
	{
	    auxd->zvalue_ = uiFlatViewer::auxDataZVal();
	    vwr_->addAuxData( auxd );
	    levelaux_ = auxd;
	}
    }

    vwr_->handleChange( mCast(unsigned int,FlatView::Viewer::Auxdata) );
}


void uiStratSynthDisp::setCurSynthetic( const SyntheticData* sd, bool wva )
{
    WeakPtr<SyntheticData>& cursynth = wva ? currentwvasynthetic_
					   : currentvdsynthetic_;
    cursynth = const_cast<SyntheticData*>( sd );
}


void uiStratSynthDisp::scalePush( CallBacker* )
{
    haveUserScaleWavelet();
}


bool uiStratSynthDisp::haveUserScaleWavelet()
{
    uiMsgMainWinSetter mws( mainwin() );

    ConstRefMan<SyntheticData> currentwvasynthetic = currentwvasynthetic_.get();
    if ( !currentwvasynthetic || currentwvasynthetic->isPS() )
    {
	uiMSG().error(tr("Please select a post-stack synthetic in wiggle view. "
			 "The scaling tool compares the amplitudes of the "
			 "synthetic data at the selected Stratigraphic Level "
			 "to real amplitudes along a horizon"));
	return false;
    }

    mDynamicCastGet(const PostStackSyntheticData*,pssd,
		    currentwvasynthetic.ptr());
    const SeisTrcBuf& tbuf = pssd->postStackPack().trcBuf();
    if ( tbuf.isEmpty() )
    {
	uiMSG().error(tr("Synthetic seismic has no trace. "
			 "Please regenerate the synthetic."));
	return false;
    }

    BufferString levelname;
    if ( curSS().getLevel() )
	levelname = curSS().getLevel()->name();
    if ( levelname.isEmpty() || levelname.startsWith( "--" ) )
    {
	uiMSG().error(tr("Please select a Stratigraphic Level.\n"
			 "The scaling tool compares the amplitudes there\n"
			 "to real amplitudes along a horizon"));
	return false;
    }

    bool is2d = SI().has2D();
    if ( is2d && SI().has3D() )
    {
	const int res = uiMSG().ask2D3D( tr("Use 2D or 3D data?"), true );
	if ( res < 0 )
	    return false;

	is2d = res == 1;
    }

    bool rv = false;
    PtrMan<SeisTrcBuf> scaletbuf = tbuf.clone();
    const int dispeach = 1;
    curSS().getLevelTimes( currentwvasynthetic->synthGenDP().getModels(),
			   dispeach, *scaletbuf );
/*    uiSynthToRealScale dlg(this,is2d,*scaletbuf,wvltfld_->getID(),levelname);
    if ( dlg.go() )
    {
	MultiID mid( dlg.selWvltID() );
	if ( mid.isUdf() )
	    pErrMsg( "Huh" );
	else
	{
	    rv = true;
	    wvltfld_->setInput( mid );
	}
	vwr_->handleChange( sCast(unsigned int,FlatView::Viewer::All) );
    } */ //TODO :reenable

    return rv;
}


void uiStratSynthDisp::parsChangedCB( CallBacker* )
{
    ConstRefMan<SyntheticData> currentvdsynthetic = currentvdsynthetic_.get();
    if ( currentvdsynthetic )
    {
	auto& disppars = const_cast<SynthFVSpecificDispPars&>(
					currentvdsynthetic->dispPars() );
	disppars.ctab_ = vwr_->appearance().ddpars_.vd_.ctab_;
	disppars.vdmapper_ = vwr_->appearance().ddpars_.vd_.mappersetup_;
    }

    ConstRefMan<SyntheticData> currentwvasynthetic = currentwvasynthetic_.get();
    if ( currentwvasynthetic )
    {
	auto& disppars = const_cast<SynthFVSpecificDispPars&>(
					currentwvasynthetic->dispPars() );
	disppars.wvamapper_ = vwr_->appearance().ddpars_.wva_.mappersetup_;
	disppars.overlap_ = vwr_->appearance().ddpars_.wva_.overlap_;
    }

    dispParsChanged.trigger();
}


void uiStratSynthDisp::viewChg( CallBacker* )
{
    updateRelativeViewRect();
    viewChanged.trigger();
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
    const SeisTrcBuf& trcbuf = curTrcBuf();
    if ( !trcbuf.size() ) return 0.0f;
    while ( true )
    {
	if ( backwardidx<0 || forwardidx>=trcbuf.size() )
	    return 0.0f;
	const int centrcidx = forward ? forwardidx : backwardidx;
	const SeisTrc* centtrc = trcbuf.size() ? trcbuf.get( centrcidx ) :  0;
	if ( centtrc && !mIsUdf(centtrc->info().pick) )
	    return centtrc->info().pick;
	forward ? forwardidx++ : backwardidx--;
	forward = !forward;
    }

    return 0.0f;
}


const uiWorldRect& uiStratSynthDisp::curView( bool indpth ) const
{
    mDefineStaticLocalObject( Threads::Lock, lock, (true) );
    Threads::Locker locker( lock );

    mDefineStaticLocalObject( uiWorldRect, timewr, );
    timewr = vwr_->curView();
    ConstRefMan<SyntheticData> currentwvasynthetic = currentwvasynthetic_.get();
    if ( !indpth || !currentwvasynthetic )
	return timewr;

    mDefineStaticLocalObject( uiWorldRect, depthwr, );
    depthwr.setLeft( timewr.left() );
    depthwr.setRight( timewr.right() );
    ObjectSet<const TimeDepthModel> curd2tmodels;
    getCurD2TModel( *currentwvasynthetic.ptr(), curd2tmodels, -1 );
    if ( !curd2tmodels.isEmpty() )
    {
	Interval<float> twtrg( mCast(float,timewr.top()),
			       mCast(float,timewr.bottom()) );
	Interval<double> depthrg( curd2tmodels[0]->getDepth(twtrg.start),
				  curd2tmodels[0]->getDepth(twtrg.stop) );
	for ( int idx=1; idx<curd2tmodels.size(); idx++ )
	{
	    const TimeDepthModel& d2t = *curd2tmodels[idx];
	    Interval<double> curdepthrg( d2t.getDepth(twtrg.start),
					 d2t.getDepth(twtrg.stop) );
	    if ( !curdepthrg.isUdf() )
		depthrg.include( curdepthrg );
	}

	if ( dispflattened_ )
	    depthrg.shift( SI().seismicReferenceDatum() );
	depthwr.setTop( depthrg.start );
	depthwr.setBottom( depthrg.stop );
    }

    return depthwr;
}


const SeisTrcBuf& uiStratSynthDisp::curTrcBuf() const
{
    ConstDataPackRef<SeisTrcBufDataPack> tbdp = vwr_->obtainPack( true, true );
    if ( !tbdp )
    {
	mDefineStaticLocalObject( SeisTrcBuf, emptybuf, (false) );
	return emptybuf;
    }

    if ( tbdp->trcBuf().isEmpty() )
	tbdp = vwr_->obtainPack( false, true );

    if ( !tbdp )
    {
	mDefineStaticLocalObject( SeisTrcBuf, emptybuf, (false) );
	return emptybuf;
    }
    return tbdp->trcBuf();
}


#define mErrRet(s,act) \
{ uiMsgMainWinSetter mws( mainwin() ); if (!s.isEmpty()) uiMSG().error(s); act;}

void uiStratSynthDisp::modelChanged()
{
    doModelChange();
}


void uiStratSynthDisp::reDisplayPostStackSynthetic( bool wva )
{
    WeakPtr<SyntheticData>& synthview = wva ? currentwvasynthetic_
					    : currentvdsynthetic_;
    ConstRefMan<SyntheticData> currentsynthetic = synthview.get();
    displayPostStackSynthetic( currentsynthetic.ptr() );
}


void uiStratSynthDisp::displaySynthetic( const SyntheticData* sd )
{
    if ( sd->isPS() )
	displayPreStackSynthetic( sd );
    else
	displayPostStackSynthetic( sd );
}


int uiStratSynthDisp::getOffsetIdx( const SyntheticData& sd ) const
{
    const Seis::SynthGenDataPack& synthgendp = sd.synthGenDP();
    const float offset = prestackgrp_->sensitive()
		       ? mCast( float, offsetposfld_->getValue() )
		       : 0.f;
    return prestackgrp_->sensitive() ? synthgendp.getOffsetIdx( offset ) : -1;
}


void uiStratSynthDisp::getCurD2TModel( const SyntheticData& sd,
		ObjectSet<const TimeDepthModel>& d2tmodels, int offsidx ) const
{
    d2tmodels.erase();
    const ReflectivityModelSet& refmodels = sd.getRefModels();
    const int nrmodels = refmodels.nrModels();
    const PreStackSyntheticData* presd = nullptr;
    if ( sd.isPS() )
	mDynamicCast(const PreStackSyntheticData*,presd,&sd);

    if ( !sd.isPS() || (presd && presd->isNMOCorrected()) || offsidx < 0 )
    {
	for ( int imdl=0; imdl<nrmodels; imdl++ )
	{
	    const ReflectivityModelBase* refmodel = refmodels.get( imdl );
	    d2tmodels.add( &refmodel->getDefaultModel() );
	}

	return;
    }

    for ( int imdl=0; imdl<nrmodels; imdl++ )
    {
	const ReflectivityModelBase* refmodel = refmodels.get( imdl );
	const TimeDepthModel* tdmodel = refmodel->get( offsidx );
	if ( !tdmodel )
	{
	    pErrMsg("Cannot find D2T Model for corresponding offset" );
	    return;
	}

	d2tmodels.add( tdmodel );
    }
}


void uiStratSynthDisp::displayPostStackSynthetic( const SyntheticData* sd,
						  bool wva )
{
    const bool hadpack = vwr_->hasPack( wva );
    if ( hadpack )
	vwr_->removePack( vwr_->packID(wva) );

    if ( !sd )
    {
	vwr_->handleChange( sCast(unsigned int,FlatView::Viewer::All) );
	return;
    }

    deleteAndZeroPtr( d2tmodels_ );

    const Seis::SynthGenDataPack& synthgendp = sd->synthGenDP();
    const float offset = prestackgrp_->sensitive()
		       ? mCast( float, offsetposfld_->getValue() )
		       : 0.f;
    const int offsidx = getOffsetIdx( *sd );
    mDynamicCastGet(const PreStackSyntheticData*,presd,sd);
    mDynamicCastGet(const PostStackSyntheticData*,postsd,sd);
    const SeisTrcBuf* tbuf = presd ? presd->getTrcBuf( offset, 0 )
				   : &postsd->postStackPack().trcBuf();
    if ( !tbuf )
	return;

    auto* disptbuf = new SeisTrcBuf( true );
    tbuf->copyInto( *disptbuf );
    ObjectSet<const TimeDepthModel> curd2tmodels;
    getCurD2TModel( *sd, curd2tmodels, offsidx );
    auto* zerooffsd2tmodels = new ObjectSet<const TimeDepthModel>;
    getCurD2TModel( *sd, *zerooffsd2tmodels, -1 );
    d2tmodels_ = zerooffsd2tmodels;
    float lasttime =  -mUdf(float);
    for ( int idx=0; idx<curd2tmodels.size(); idx++ )
    {
	if ( curd2tmodels[idx]->getLastTime() > lasttime )
	    longestaimdl_ = idx;
    }

    curSS().decimateTraces( *disptbuf, dispeach_ );
    reSampleTraces( *sd, *disptbuf );
    if ( dispflattened_ )
    {
	curSS().getLevelTimes( synthgendp.getModels(), dispeach_, *disptbuf,
			       offsidx );
	curSS().flattenTraces( *disptbuf );
    }
    else
	curSS().trimTraces( *disptbuf, curd2tmodels, dispskipz_);


    auto* dp = new SeisTrcBufDataPack( disptbuf, Seis::Line,
				    SeisTrcInfo::TrcNr, "Forward Modeling" );
    DPM( DataPackMgr::FlatID() ).add( dp );
    dp->setName( sd->name() );
    if ( !wva )
	vwr_->appearance().ddpars_.vd_.ctab_ = sd->dispPars().ctab_;
    else
	vwr_->appearance().ddpars_.wva_.overlap_ = sd->dispPars().overlap_;

    ColTab::MapperSetup& mapper =
	wva ? vwr_->appearance().ddpars_.wva_.mappersetup_
	    : vwr_->appearance().ddpars_.vd_.mappersetup_;
    const bool ispropsd = sd->isStratProp() || sd->isAttribute();
    auto* dispsd = const_cast<SyntheticData*>( sd );
    ColTab::MapperSetup& dispparsmapper =
	!wva ? dispsd->dispPars().vdmapper_ : dispsd->dispPars().wvamapper_;
    const bool rgnotsaved = (mIsZero(dispparsmapper.range_.start,mDefEps) &&
			     mIsZero(dispparsmapper.range_.stop,mDefEps)) ||
			     dispparsmapper.range_.isUdf();
    if ( !rgnotsaved )
	mapper = dispparsmapper;
    else
    {
	mapper.range_ = Interval<float>::udf();
	const float cliprate = wva ? 0.0f : 0.025f;
	mapper.cliprate_ = Interval<float>(cliprate,cliprate);
	mapper.autosym0_ = true;
	mapper.type_ = ColTab::MapperSetup::Auto;
	mapper.symmidval_ = ispropsd ? mUdf(float) : 0.0f;
	if ( sd->dispPars().ctab_.isEmpty() )
	{
	    dispsd->dispPars().ctab_ =
		vwr_->appearance().ddpars_.vd_.ctab_ =
			ispropsd ? sKeyRainbow() : sKeySeismics();
	}
    }

    vwr_->setPack( wva, dp->id(), !hadpack );
    vwr_->setVisible( wva, true );
    control_->setD2TModels( *d2tmodels_ );
    NotifyStopper notstop( vwr_->viewChanged );
    if ( mIsZero(relzoomwr_.left(),1e-3) &&
	 mIsEqual(relzoomwr_.width(),1.0,1e-3) &&
	 mIsEqual(relzoomwr_.height(),1.0,1e-3) )
	vwr_->setViewToBoundingBox();
    else
	setRelativeViewRect( relzoomwr_ );

    levelSnapChanged( nullptr );
}


void uiStratSynthDisp::setSavedViewRect()
{
    if ( mIsUdf(savedzoomwr_.left()) )
	return;
    setAbsoluteViewRect( savedzoomwr_ );
    setZoomView( relzoomwr_ );
}


void uiStratSynthDisp::reSampleTraces( const SyntheticData& sd,
				       SeisTrcBuf& tbuf ) const
{
    if ( longestaimdl_>=layerModel().size() || longestaimdl_<0 )
	return;

    Interval<float> depthrg = layerModel().sequence(longestaimdl_).zRange();

    const int offsidx = getOffsetIdx( sd );
    ObjectSet<const TimeDepthModel> curd2tmodels;
    const bool isstratprop = sd.isStratProp();
    getCurD2TModel( sd, curd2tmodels, offsidx );
    if ( !curd2tmodels.validIdx(longestaimdl_) )
	return;

    const TimeDepthModel& d2t = *curd2tmodels[longestaimdl_];
    const float reqlastzval =
	d2t.getTime( layerModel().sequence(longestaimdl_).zRange().stop );
    for ( int idx=0; idx<tbuf.size(); idx++ )
    {
	SeisTrc& trc = *tbuf.get( idx );

	const float lastzval = trc.info().sampling.atIndex( trc.size()-1 );
	const int lastsz = trc.size();
	if ( lastzval > reqlastzval )
	    continue;
	const int newsz = trc.info().sampling.nearestIndex( reqlastzval );
	trc.reSize( newsz, true );
	if ( isstratprop )
	{
	    const float lastval = trc.get( lastsz-1, 0 );
	    for ( int xtrasampidx=lastsz; xtrasampidx<newsz; xtrasampidx++ )
		trc.set( xtrasampidx, lastval, 0 );
	}
    }
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

	auto* gather = new PreStack::Gather( *gathers[idx] );
	auto* anglegather= new PreStack::Gather( *anglegathers[idx] );
	PreStackView::GatherInfo gatherinfo;
	gatherinfo.isstored_ = false;
	gatherinfo.gathernm_ = sd->name();
	gatherinfo.bid_ = gather->getBinID();
	gatherinfo.wvadpid_ = DPM( DataPackMgr::FlatID() )
						.addAndObtain( gather )->id();
	gatherinfo.vddpid_ = DPM( DataPackMgr::FlatID() ).addAndObtain(
							    anglegather )->id();
	gatherinfo.isselected_ = true;
	gatherinfos += gatherinfo;
    }

    prestackwin_->setGathers( gatherinfos );
}


void uiStratSynthDisp::setPreStackMapper()
{
    for ( int idx=0; idx<prestackwin_->nrViewers(); idx++ )
    {
	uiFlatViewer& vwr = prestackwin_->viewer( idx );
	ColTab::MapperSetup& vdmapper =
	    vwr.appearance().ddpars_.vd_.mappersetup_;
	vdmapper.cliprate_ = Interval<float>(0.0,0.0);
	vdmapper.autosym0_ = false;
	vdmapper.symmidval_ = mUdf(float);
	vdmapper.type_ = ColTab::MapperSetup::Fixed;
	vdmapper.range_ = Interval<float>(0,60);
	vwr.appearance().ddpars_.vd_.ctab_ = sKeyRainbow();
	ColTab::MapperSetup& wvamapper =
	    vwr.appearance().ddpars_.wva_.mappersetup_;
	wvamapper.cliprate_ = Interval<float>(0.0,0.0);
	wvamapper.autosym0_ = true;
	wvamapper.symmidval_ = 0.0f;
	vwr.handleChange( mCast(unsigned int,FlatView::Viewer::DisplayPars) );
    }
}


void uiStratSynthDisp::selPreStackDataCB( CallBacker* )
{
    BufferStringSet allgnms, selgnms;
    for ( int idx=0; idx<curSS().nrSynthetics(); idx++ )
    {
	const SyntheticData* sd = curSS().getSyntheticByIdx( idx );
	mDynamicCastGet(const PreStackSyntheticData*,presd,sd);
	if ( !presd ) continue;
	allgnms.addIfNew( sd->name() );
    }

    TypeSet<PreStackView::GatherInfo> ginfos = prestackwin_->gatherInfos();

    prestackwin_->getGatherNames( selgnms );
    for ( int idx=0; idx<selgnms.size(); idx++ )
    {
	const int gidx = allgnms.indexOf( selgnms[idx]->buf() );
	if ( gidx<0 )
	    continue;
	allgnms.removeSingle( gidx );
    }

    PreStackView::uiViewer2DSelDataDlg seldlg( prestackwin_, allgnms, selgnms);
    if ( !seldlg.go() )
	return;

    prestackwin_->removeGathers();
    TypeSet<PreStackView::GatherInfo> newginfos;
    for ( int synthidx=0; synthidx<selgnms.size(); synthidx++ )
    {
	const SyntheticData* sd =
	    curSS().getSynthetic( selgnms[synthidx]->buf() );
	if ( !sd ) continue;
	mDynamicCastGet(const PreStackSyntheticData*,presd,sd);
	mDynamicCastGet(const PreStack::GatherSetDataPack*,gsetdp,
			&sd->getPack())
	if ( !gsetdp || !presd ) continue;
	const PreStack::GatherSetDataPack& angledp = presd->angleData();
	for ( int idx=0; idx<ginfos.size(); idx++ )
	{
	    PreStackView::GatherInfo ginfo = ginfos[idx];
	    ginfo.gathernm_ = sd->name();
	    const PreStack::Gather* gather = gsetdp->getGather( ginfo.bid_);
	    const PreStack::Gather* anglegather =
		angledp.getGather( ginfo.bid_);
	    ginfo.vddpid_ = anglegather->id();
	    ginfo.wvadpid_ = gather->id();
	    newginfos.addIfNew( ginfo );
	}
    }

    prestackwin_->setGathers( newginfos, false );
}


void uiStratSynthDisp::preStackWinClosedCB( CallBacker* )
{
    prestackwin_ = nullptr;
}


void uiStratSynthDisp::viewPreStackPush( CallBacker* cb )
{
    ConstRefMan<SyntheticData> currentwvasynthetic = currentwvasynthetic_.get();
    if ( !currentwvasynthetic || !currentwvasynthetic->isPS() )
	return;

    if ( !prestackwin_ )
    {
	prestackwin_ =
	    new PreStackView::uiSyntheticViewer2DMainWin(this,"Prestack view");
	mAttachCB( prestackwin_->seldatacalled_,
		   uiStratSynthDisp::selPreStackDataCB );
	mAttachCB( prestackwin_->windowClosed,
		   uiStratSynthDisp::preStackWinClosedCB );
    }

    displayPreStackSynthetic( currentwvasynthetic.ptr() );
    prestackwin_->show();
}


void uiStratSynthDisp::setCurrentSynthetic( bool wva )
{
    ConstRefMan<SyntheticData> sd =
			curSS().getSynthetic( wva ? wvadatalist_->text()
						  : vddatalist_->text() );
    setCurSynthetic( sd, wva );
}


void uiStratSynthDisp::updateFields()
{
    ConstRefMan<SyntheticData> currentwvasynthetic = currentwvasynthetic_.get();
    mDynamicCastGet(const PreStackSyntheticData*,pssd,
		    currentwvasynthetic.ptr());
    if ( pssd )
    {
	StepInterval<float> limits( pssd->offsetRange() );
	const float offsetstep = pssd->offsetRangeStep();
	limits.step = mIsUdf(offsetstep) ? 100 : offsetstep;
	offsetposfld_->setLimitSampling( limits );
    }

    prestackgrp_->setSensitive( pssd && pssd->hasOffset() );
    datagrp_->setSensitive( currentwvasynthetic );
    scalebut_->setSensitive( !pssd );
}


void uiStratSynthDisp::copySyntheticDispPars()
{
    for ( int sidx=0; sidx<curSS().nrSynthetics(); sidx++ )
    {
	const SyntheticData* cursd = curSS().getSyntheticByIdx( sidx );
	BufferString sdnm( cursd->name() );
	if ( useed_ )
	    sdnm.remove( StratSynth::sKeyFRNameSuffix() );
	else
	    sdnm += StratSynth::sKeyFRNameSuffix();

	SynthFVSpecificDispPars* sddisp = curSS().dispPars( cursd->name() );
	const SynthFVSpecificDispPars* altdisp = altSS().dispPars( sdnm );
	if ( !sddisp || !altdisp )
	    continue;

	*sddisp = *altdisp;
    }
}


void uiStratSynthDisp::showFRResults()
{
    const int wvacuritm = wvadatalist_->currentItem();
    const int vdcuritm = vddatalist_->currentItem();
    updateSyntheticList( true );
    updateSyntheticList( false );
    if ( wvadatalist_->size() <= 1 )
	return;
    copySyntheticDispPars();
    wvadatalist_->setCurrentItem( wvacuritm );
    vddatalist_->setCurrentItem( vdcuritm );
    setCurrentSynthetic( true );
    setCurrentSynthetic( false );
    ConstRefMan<SyntheticData> currentwvasynthetic = currentwvasynthetic_.get();
    ConstRefMan<SyntheticData> currentvdsynthetic = currentvdsynthetic_.get();
    displaySynthetic( currentwvasynthetic );
    displayPostStackSynthetic( currentvdsynthetic, false );
}


void uiStratSynthDisp::doModelChange()
{
    if ( !autoupdate_ && !forceupdate_ )
	return;
    if ( !curSS().errMsg().isOK() )
	mErrRet( curSS().errMsg(), return )

    MouseCursorChanger mcs( MouseCursor::Busy );
    showInfoMsg( false );
    updateSyntheticList( true );
    updateSyntheticList( false );
    if ( wvadatalist_->size() <= 1 )
	return;
    wvadatalist_->setCurrentItem( 1 );
    vddatalist_->setCurrentItem( 1 );
    setCurrentSynthetic( true );
    setCurrentSynthetic( false );

    updateFields();
    ConstRefMan<SyntheticData> currentwvasynthetic = currentwvasynthetic_.get();
    ConstRefMan<SyntheticData> currentvdsynthetic = currentvdsynthetic_.get();
    displaySynthetic( currentwvasynthetic );
    displayPostStackSynthetic( currentvdsynthetic, false );
}


void uiStratSynthDisp::addEditSynth( CallBacker* )
{
    if ( !synthgendlg_ )
    {
	synthgendlg_ = new uiSynthGenDlg( this, curSS() );
	uiSynthParsGrp* uiparsgrp = synthgendlg_->grp();
	mAttachCB( uiparsgrp->synthAdded, uiStratSynthDisp::syntheticAdded );
	mAttachCB( uiparsgrp->synthRenamed,
		   uiStratSynthDisp::syntheticRenamed );
	mAttachCB( uiparsgrp->synthChanged,
		   uiStratSynthDisp::syntheticChanged );
	mAttachCB( uiparsgrp->synthRemoved,
		   uiStratSynthDisp::syntheticRemoved );
	mAttachCB( uiparsgrp->synthDisabled,
		   uiStratSynthDisp::syntheticDisabled );
	mAttachCB( uiparsgrp->elPropSel,
		   uiStratSynthDisp::layerPropsPush );
    }

    synthgendlg_->go();
}


void uiStratSynthDisp::updateAltSynthetic( const char* oldnm, const char* newnm,
					   bool nameonly )
{
    if ( FixedString(oldnm) == sKeyNone() )
	return;

    if ( !nameonly )
	deleteAndZeroPtr( d2tmodels_ );

    if ( !altSS().hasElasticModels() )
	return;

    ConstRefMan<SyntheticData> synthetic = curSS().getSynthetic( newnm );
    if ( !synthetic )
	return;

    if ( (nameonly && !altSS().updateSyntheticName(oldnm,newnm)) ||
       (!nameonly && !altSS().updateSynthetic(oldnm,synthetic->getGenParams())))
    {
	mErrRet(altSS().errMsg(), return );
    }

    showInfoMsg( true );
}


void uiStratSynthDisp::updateDispSynthetic( const char* oldnm,
					    const char* newnm, bool nameonly )
{
    ConstRefMan<SyntheticData> currentwvasynthetic = currentwvasynthetic_.get();
    ConstRefMan<SyntheticData> currentvdsynthetic = currentvdsynthetic_.get();
    const BufferString curwvasdnm( currentwvasynthetic ?
			currentwvasynthetic->name().buf() : "" );
    const BufferString curvdsdnm( currentvdsynthetic ?
			currentvdsynthetic->name().buf() : "" );

    const FixedString oldsyntheticnm( oldnm );
    if ( (!wvadatalist_->isPresent(oldsyntheticnm) &&
	  !vddatalist_->isPresent(oldsyntheticnm) ) ||
	 oldsyntheticnm == sKeyNone() )
	return;

    bool wvawasactive = false;
    if ( wvadatalist_->isPresent(oldsyntheticnm) )
    {
	wvawasactive = FixedString( wvadatalist_->text() ) == oldsyntheticnm;
	if ( oldsyntheticnm != newnm )
	{
	    const int curidx = wvadatalist_->currentItem();
	    updateSyntheticList( true );
	    if ( wvawasactive )
		wvadatalist_->setCurrentItem( newnm );
	    else if ( curidx >=0 && curidx < wvadatalist_->size() )
		wvadatalist_->setCurrentItem( curidx );
	}

	if ( !nameonly )
	    setCurrentSynthetic( true );
    }

    bool vdwasactive = false;
    if ( vddatalist_->isPresent(oldsyntheticnm) )
    {
	vdwasactive = FixedString( vddatalist_->text() ) == oldsyntheticnm;
	if ( oldsyntheticnm != newnm )
	{
	    const int curidx = vddatalist_->currentItem();
	    updateSyntheticList( false );
	    if ( vdwasactive )
		vddatalist_->setCurrentItem( newnm );
	    else if ( curidx >=0 && curidx < vddatalist_->size() )
		vddatalist_->setCurrentItem( curidx );
	}

	if ( !nameonly )
	    setCurrentSynthetic( false );
    }

    showInfoMsg( false );
    if ( nameonly )
    {
	synthsChanged.trigger();
	return;
    }

    updateFields();
    ConstRefMan<SyntheticData> wvasynthetic =
			       curSS().getSynthetic( wvadatalist_->text() );
    if ( wvasynthetic )
	displaySynthetic( wvasynthetic.ptr() );

    ConstRefMan<SyntheticData> vdsynthetic =
			       curSS().getSynthetic( vddatalist_->text() );
    if ( vdsynthetic )
	displayPostStackSynthetic( vdsynthetic.ptr(), false );

    synthsChanged.trigger();
}


void uiStratSynthDisp::syntheticAdded( CallBacker* cb )
{
    if ( !synthgendlg_ || !cb )
	return;

    MouseCursorChanger mcchger( MouseCursor::Wait );
    showInfoMsg( false );
    if ( altSS().hasElasticModels() )
    {
	mCBCapsuleUnpack(BufferString,syntheticnm,cb);
	ConstRefMan<SyntheticData> synthetic =
				   curSS().getSynthetic( syntheticnm.buf() );
	if ( synthetic )
	{
	    const SyntheticData* altsd =
			     altSS().addSynthetic( synthetic->getGenParams() );
	    if ( !altsd )
		mErrRet(altSS().errMsg(), return )

	    showInfoMsg( true );
	}
    }

    updateSyntheticList( true );
    updateSyntheticList( false );
    synthsChanged.trigger();
}


void uiStratSynthDisp::syntheticRenamed( CallBacker* cb )
{
    if ( !cb )
	return;

    mCBCapsuleUnpack(BufferStringSet,syntheticnms,cb);
    if ( syntheticnms.size() != 2 )
	return;

    const BufferString& oldnm = *syntheticnms.first();
    const BufferString& newnm = *syntheticnms.last();
    updateAltSynthetic( oldnm, newnm, true );
    updateDispSynthetic( oldnm, newnm, true );
}


void uiStratSynthDisp::syntheticChanged( CallBacker* cb )
{
    if ( !cb )
	return;

    mCBCapsuleUnpack(BufferStringSet,syntheticnms,cb);
    if ( syntheticnms.size() != 2 )
	return;

    const BufferString& oldnm = *syntheticnms.first();
    const BufferString& newnm = *syntheticnms.last();
    updateAltSynthetic( oldnm, newnm, false );
    updateDispSynthetic( oldnm, newnm, false );
}


void uiStratSynthDisp::syntheticRemoved( CallBacker* cb )
{
    const BufferString curwvasdnm( wvadatalist_->text() );
    const BufferString curvdsdnm( vddatalist_->text() );

    mCBCapsuleUnpack(BufferString,synthname,cb);

    if ( curwvasdnm==synthname )
	setCurSynthetic( nullptr, true );
    if ( curvdsdnm==synthname )
	setCurSynthetic( nullptr, false );
    deleteAndZeroPtr( d2tmodels_ );
    altSS().removeSynthetic( synthname );
    synthsChanged.trigger();
    updateSyntheticList( true );
    updateSyntheticList( false );
    if ( wvadatalist_->size() <= 1 )
	return;

    wvadatalist_->setCurrentItem( 1 );
    vddatalist_->setCurrentItem( 1 );
    setCurrentSynthetic( true );
    setCurrentSynthetic( false );
    ConstRefMan<SyntheticData> currentwvasynthetic = currentwvasynthetic_.get();
    ConstRefMan<SyntheticData> currentvdsynthetic = currentvdsynthetic_.get();
    displayPostStackSynthetic( currentwvasynthetic, true );
    displayPostStackSynthetic( currentvdsynthetic, false );
}


void uiStratSynthDisp::syntheticDisabled( CallBacker* cb )
{
    if ( !cb )
	return;

    mCBCapsuleUnpack(BufferString,synthname,cb);
    curSS().disableSynthetic( synthname );
    altSS().disableSynthetic( synthname );
}


void uiStratSynthDisp::exportSynth( CallBacker* )
{
    if ( layerModel().isEmpty() )
	mErrRet( tr("No valid layer model present"), return )
    uiStratSynthExport dlg( this, curSS() );
    dlg.go();
}


void uiStratSynthDisp::offsetChged( CallBacker* )
{
    ConstRefMan<SyntheticData> currentwvasynthetic = currentwvasynthetic_.get();
    ConstRefMan<SyntheticData> currentvdsynthetic = currentvdsynthetic_.get();
    displayPostStackSynthetic( currentwvasynthetic, true );
    if ( FixedString(wvadatalist_->text())==vddatalist_->text() &&
	 currentwvasynthetic && currentwvasynthetic->isPS() )
	displayPostStackSynthetic( currentvdsynthetic, false );
}


const PropertyRefSelection& uiStratSynthDisp::modelPropertyRefs() const
{
    return layerModel().propertyRefs();
}


SynthFVSpecificDispPars* uiStratSynthDisp::dispPars( const char* synthnm )
{
    RefMan<SyntheticData> sd = getSyntheticData( synthnm );
    return sd ? &sd->dispPars() : nullptr;
}


const ObjectSet<const TimeDepthModel>* uiStratSynthDisp::d2TModels() const
{
    return d2tmodels_;
}


void uiStratSynthDisp::vdDataSetSel( CallBacker* )
{
    setCurrentSynthetic( false );
    ConstRefMan<SyntheticData> currentvdsynthetic = currentvdsynthetic_.get();
    displayPostStackSynthetic( currentvdsynthetic, false );
}


void uiStratSynthDisp::wvDataSetSel( CallBacker* )
{
    setCurrentSynthetic( true );
    updateFields();
    ConstRefMan<SyntheticData> currentwvasynthetic = currentwvasynthetic_.get();
    displayPostStackSynthetic( currentwvasynthetic, true );
}


const ObjectSet<const SyntheticData>& uiStratSynthDisp::getSynthetics() const
{
    return curSS().synthetics();
}


void uiStratSynthDisp::showInfoMsg( bool foralt )
{
    StratSynth& ss = foralt ? altSS() : curSS();
    if ( !ss.infoMsg().isOK() )
    {
	uiMsgMainWinSetter mws( mainwin() );
	uiMSG().warning( ss.infoMsg() );
	ss.clearInfoMsg();
    }
}


RefMan<SyntheticData> uiStratSynthDisp::getSyntheticData( const char* nm )
{
    const SyntheticData* sd = curSS().getSynthetic( nm );
    RefMan<SyntheticData> ret = const_cast<SyntheticData*>( sd );
    return ret;
}


ConstRefMan<SyntheticData> uiStratSynthDisp::getSyntheticData(
							const char* nm ) const
{
    ConstRefMan<SyntheticData> ret = curSS().getSynthetic( nm );
    return ret;
}


ConstRefMan<SyntheticData> uiStratSynthDisp::getCurrentSyntheticData(
							bool wva ) const
{
    const WeakPtr<SyntheticData>& synthview = wva ? currentwvasynthetic_
						  : currentvdsynthetic_;
    ConstRefMan<SyntheticData> ret = synthview.get();
    return ret;
}


void uiStratSynthDisp::fillPar( IOPar& par, const StratSynth* stratsynth ) const
{
    IOPar stratsynthpar;
    stratsynthpar.set( sKeySnapLevel(), levelsnapselfld_->currentItem());
    int nr_nonproprefsynths = 0;
    for ( const auto* sd : stratsynth->synthetics() )
    {
	if ( sd->isStratProp() )
	    continue;

	nr_nonproprefsynths++;
	const SynthGenParams& sgd = sd->getGenParams();
	IOPar synthpar;
	sgd.fillPar( synthpar );
	sd->fillDispPar( synthpar );
	stratsynthpar.mergeComp( synthpar,
		IOPar::compKey(StratSynth::sKeySyntheticNr(),
				nr_nonproprefsynths-1) );
    }

    savedzoomwr_ = curView( false );
    TypeSet<double> startviewareapts;
    startviewareapts.setSize( 4 );
    startviewareapts[0] = savedzoomwr_.left();
    startviewareapts[1] = savedzoomwr_.top();
    startviewareapts[2] = savedzoomwr_.right();
    startviewareapts[3] = savedzoomwr_.bottom();
    stratsynthpar.set( sKeyViewArea(), startviewareapts );
    stratsynthpar.set( StratSynth::sKeyNrSynthetics(), nr_nonproprefsynths );
    par.removeWithKey( StratSynth::sKeySynthetics() );
    par.mergeComp( stratsynthpar, StratSynth::sKeySynthetics() );
}


void uiStratSynthDisp::fillPar( IOPar& par, bool useed ) const
{
    fillPar( par, useed ? edstratsynth_ : stratsynth_ );
}


void uiStratSynthDisp::fillPar( IOPar& par ) const
{
    fillPar( par, &curSS() );
}


bool uiStratSynthDisp::prepareElasticModel()
{
    if ( !forceupdate_ && !autoupdate_ )
	return false;

    if ( !curSS().hasTaskRunner() )
    {
	if ( !taskrunner_ )
	{
	    auto* uitaskrunner = new uiTaskRunner( this );
	    taskrunner_ = uitaskrunner;
	    mAttachCB( uitaskrunner->objectToBeDeleted(),
		       uiStratSynthDisp::uiTaskRunDeletedCB );
	}

	curSS().setTaskRunner( taskrunner_.ptr() );
    }

    return curSS().createElasticModels();
}


void uiStratSynthDisp::uiTaskRunDeletedCB( CallBacker* cb )
{
    if( !cb || !taskrunner_ )
	return;

    mDynamicCastGet(uiBaseObject*,uitaskrunnerdlg,cb);
    if ( !uitaskrunnerdlg )
	return;

    const_cast<StratSynth&>( normalSS() ).setTaskRunner( nullptr );
    const_cast<StratSynth&>( editSS() ).setTaskRunner( nullptr );
    taskrunner_.release();
}


bool uiStratSynthDisp::usePar( const IOPar& par )
{
    if ( !curSS().hasElasticModels() )
	return false;

    setCurSynthetic( nullptr, true );
    setCurSynthetic( nullptr, false );
    curSS().clearSynthetics();
    deleteAndZeroPtr( d2tmodels_ );
    par.get( sKeyDecimation(), dispeach_ );

    PtrMan<IOPar> stratsynthpar = par.subselect( StratSynth::sKeySynthetics() );
    const bool hasiop = stratsynthpar;
    if ( !stratsynthpar )
    {
	stratsynthpar = new IOPar;
	stratsynthpar->set( StratSynth::sKeyNrSynthetics(), 1 );
	IOPar defiop;
	const SynthGenParams sgp;
	sgp.fillPar( defiop );
	stratsynthpar->mergeComp( defiop,
			   IOPar::compKey(StratSynth::sKeySyntheticNr(),0) );
    }

    const bool res = synthgendlg_
		   ? synthgendlg_->grp()->usePar( *stratsynthpar.ptr() )
		   : curSS().usePar( *stratsynthpar.ptr() );
    if ( res )
    {
	const int nrsynths = curSS().nrSynthetics();
	if ( &curSS() == &editSS() )
	{
	    for ( int idx=0; idx<nrsynths; idx++ )
	    {
		const BufferString synthnm( curSS().getSyntheticName(idx) );
		const SynthFVSpecificDispPars* curdisp =
					    normalSS().dispPars( synthnm );
		SynthFVSpecificDispPars* sddisp =
					    curSS().dispPars( synthnm );
		if ( !curdisp || !sddisp )
		    continue;

		IOPar synthdisppar;
		curdisp->fillPar( synthdisppar );
		sddisp->usePar( synthdisppar );
	    }
	}
	else
	{
	    PtrMan<IOPar> synthpar =
		stratsynthpar->subselect( StratSynth::sKeySyntheticNr() );
	    if ( synthpar )
	    {
		for ( int idx=0; idx<nrsynths; idx++ )
		{
		    ConstPtrMan<IOPar> iop = synthpar->subselect( idx );
		    if ( !iop )
			continue;

		    const BufferString synthnm(
					    curSS().getSyntheticName(idx) );
		    SynthFVSpecificDispPars* sddisp =
					    curSS().dispPars( synthnm );
		    if ( sddisp )
			sddisp->usePar( *iop.ptr() );
		}
	    }
	}
    }
    else
	showInfoMsg( false );

    if ( curSS().nrSynthetics() < 1 )
    {
	displaySynthetic( nullptr );
	displayPostStackSynthetic( nullptr, false );
	return false;
    }

    if ( !hasiop )
	synthsChanged.trigger(); //update synthetic WorkBenchPar

    if ( GetEnvVarYN("DTECT_STRAT_MAKE_PROPERTYTRACES",true) )
	curSS().generateOtherQuantities();

    if ( useed_ && GetEnvVarYN("USE_FR_DIFF",false) )
	setDiffData();

    if ( hasiop )
    {
	int snaplvl = 0;
	stratsynthpar->get( sKeySnapLevel(), snaplvl );
	levelsnapselfld_->setCurrentItem( snaplvl );
	TypeSet<double> startviewareapts;
	if ( stratsynthpar->get(sKeyViewArea(),startviewareapts) &&
	     startviewareapts.size() == 4 )
	{
	    savedzoomwr_.setLeft( startviewareapts[0] );
	    savedzoomwr_.setTop( startviewareapts[1] );
	    savedzoomwr_.setRight( startviewareapts[2] );
	    savedzoomwr_.setBottom( startviewareapts[3] );
	}
    }

    return true;
}


void uiStratSynthDisp::setDiffData()
{
    for ( int idx=0; idx<curSS().nrSynthetics(); idx++ )
    {
	ConstRefMan<SyntheticData> frsdc = curSS().getSyntheticByIdx( idx );
	ConstRefMan<SyntheticData> sd = altSS().getSyntheticByIdx( idx );
	if ( !frsdc || !sd )
	    continue;

	auto* frsd = const_cast<SyntheticData*>( frsdc.ptr() );
	if ( !sd->isPS() )
	{
	    mDynamicCastGet(PostStackSyntheticData*,frpostsd,frsd)
	    mDynamicCastGet(const PostStackSyntheticData*,postsd,sd.ptr())
	    SeisTrcBuf& frsdbuf = frpostsd->postStackPack().trcBuf();
	    const SeisTrcBuf& sdbuf = postsd->postStackPack().trcBuf();
	    for ( int itrc=0; itrc<frsdbuf.size(); itrc++ )
	    {
		SeisTrc* frtrc = frsdbuf.get( itrc );
		const SeisTrc* trc = sdbuf.get( itrc );
		for ( int isamp=0; isamp<frtrc->size(); isamp++ )
		{
		    const float z = trc->samplePos( isamp );
		    const float trcval = trc->getValue( z, 0 );
		    const float frtrcval = frtrc->getValue( z, 0 );
		    frtrc->set( isamp, frtrcval - trcval , 0 );
		}
	    }
	}
	else
	{
	    mDynamicCastGet(PreStackSyntheticData*,frpresd,frsd)
	    mDynamicCastGet(const PreStackSyntheticData*,presd,sd.ptr())
	    PreStack::GatherSetDataPack& frgdp =frpresd->preStackPack();
	    ObjectSet<PreStack::Gather>& frgathers = frgdp.getGathers();
	    StepInterval<float> offrg( frpresd->offsetRange() );
	    offrg.step = frpresd->offsetRangeStep();
	    const PreStack::GatherSetDataPack& gdp = presd->preStackPack();
	    for ( int igather=0; igather<frgathers.size(); igather++ )
	    {
		for ( int offsidx=0; offsidx<offrg.nrSteps(); offsidx++ )
		{
		    SeisTrc* frtrc = frgdp.getTrace( igather, offsidx );
		    const SeisTrc* trc = gdp.getTrace( igather, offsidx );
		    for ( int isamp=0; isamp<frtrc->size(); isamp++ )
		    {
			const float trcval = trc->get( isamp, 0 );
			const float frtrcval = frtrc->get( isamp, 0 );
			frtrc->set( isamp, frtrcval-trcval, 0 );
		    }
		}
	    }
	}
    }
}


uiGroup* uiStratSynthDisp::getDisplayClone( uiParent* p ) const
{
    auto* vwr = new uiFlatViewer( p );
    vwr->rgbCanvas().disableImageSave();
    vwr->setInitialSize( uiSize(800,300) );
    vwr->setStretch( 2, 2 );
    vwr->appearance() = vwr_->appearance();
    vwr->setPack( true, vwr_->packID(true), false );
    vwr->setPack( false, vwr_->packID(false), false );
    return vwr;
}


void uiStratSynthDisp::setDefaultAppearance( FlatView::Appearance& app )
{
    app.setGeoDefaults( true );
    app.setDarkBG( false );
    app.annot_.allowuserchangereversedaxis_ = false;
    app.annot_.title_.setEmpty();
    app.annot_.x1_.showAll( true );
    app.annot_.x2_.showAll( true );
    app.annot_.x1_.annotinint_ = true;
    app.annot_.x2_.name_ = "TWT";
    app.ddpars_.show( true, true );
    app.ddpars_.wva_.allowuserchangedata_ = false;
    app.ddpars_.vd_.allowuserchangedata_ = false;
}


uiSynthSlicePos::uiSynthSlicePos( uiParent* p, const uiString& lbltxt )
    : uiGroup( p, "Slice Pos" )
    , positionChg(this)
{
    label_ = new uiLabel( this, lbltxt );
    sliceposbox_ = new uiSpinBox( this, 0, "Slice position" );
    mAttachCB( sliceposbox_->valueChanging, uiSynthSlicePos::slicePosChg );
    mAttachCB( sliceposbox_->valueChanged, uiSynthSlicePos::slicePosChg );
    sliceposbox_->attach( rightOf, label_ );

    auto* steplabel = new uiLabel( this, uiStrings::sStep() );
    steplabel->attach( rightOf, sliceposbox_ );

    slicestepbox_ = new uiSpinBox( this, 0, "Slice step" );
    slicestepbox_->attach( rightOf, steplabel );

    prevbut_ = new uiToolButton( this, "prevpos", tr("Previous position"),
				mCB(this,uiSynthSlicePos,prevCB) );
    prevbut_->attach( rightOf, slicestepbox_ );
    nextbut_ = new uiToolButton( this, "nextpos", tr("Next position"),
				 mCB(this,uiSynthSlicePos,nextCB) );
    nextbut_->attach( rightOf, prevbut_ );
}


uiSynthSlicePos::~uiSynthSlicePos()
{
    detachAllNotifiers();
}


void uiSynthSlicePos::slicePosChg( CallBacker* )
{
    positionChg.trigger();
}


void uiSynthSlicePos::prevCB( CallBacker* )
{
    uiSpinBox* posbox = sliceposbox_;
    uiSpinBox* stepbox = slicestepbox_;
    posbox->setValue( posbox->getIntValue()-stepbox->getIntValue() );
}


void uiSynthSlicePos::nextCB( CallBacker* )
{
    uiSpinBox* posbox = sliceposbox_;
    uiSpinBox* stepbox = slicestepbox_;
    posbox->setValue( posbox->getIntValue()+stepbox->getIntValue() );
}


void uiSynthSlicePos::setLimitSampling( StepInterval<float> lms )
{
    limitsampling_ = lms;
    sliceposbox_->setInterval( lms.start, lms.stop );
    sliceposbox_->setStep( lms.step );
    slicestepbox_->setValue( lms.step );
    slicestepbox_->setStep( lms.step );
    slicestepbox_->setMinValue( lms.step );
}


void uiSynthSlicePos::setValue( int val ) const
{
    sliceposbox_->setValue( val );
}


int uiSynthSlicePos::getValue() const
{
    return sliceposbox_->getIntValue();
}
