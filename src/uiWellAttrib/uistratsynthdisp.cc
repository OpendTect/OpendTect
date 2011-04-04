/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratsynthdisp.cc,v 1.30 2011-04-04 15:14:52 cvsbruno Exp $";

#include "uistratsynthdisp.h"
#include "uiseiswvltsel.h"
#include "uisynthtorealscale.h"
#include "uicombobox.h"
#include "uiflatviewer.h"
#include "uiflatviewstdcontrol.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uiflatviewslicepos.h"
#include "uispinbox.h"
#include "uitaskrunner.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"

#include "flatposdata.h"
#include "flatviewzoommgr.h"
#include "ioman.h"
#include "ptrman.h"
#include "raytrace1d.h"
#include "survinfo.h"
#include "synthseis.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "velocitycalc.h"
#include "wavelet.h"


static const int cMarkerSize = 6;

#define mMAXOffsetIdx 50
Notifier<uiStratSynthDisp>& uiStratSynthDisp::fieldsCreated()
{
    static Notifier<uiStratSynthDisp> FieldsCreated(0);
    return FieldsCreated;
}


uiStratSynthDisp::uiStratSynthDisp( uiParent* p, const Strat::LayerModel& lm )
    : uiGroup(p,"LayerModel synthetics display")
    , wvlt_(0)
    , lm_(lm)
    , wvltChanged(this)
    , zoomChanged(this)
    , longestaimdl_(0)
    , lasttool_(0)
    , raytrcpardlg_(0)
{
    topgrp_ = new uiGroup( this, "Top group" );
    topgrp_->setFrame( true );
    topgrp_->setStretch( 2, 0 );
    wvltfld_ = new uiSeisWaveletSel( topgrp_ );
    wvltfld_->newSelection.notify( mCB(this,uiStratSynthDisp,wvltChg) );
    wvltfld_->setFrame( false );

    scalebut_ = new uiPushButton( topgrp_, "Scale", false );
    scalebut_->activated.notify( mCB(this,uiStratSynthDisp,scalePush) );
    scalebut_->attach( rightOf, wvltfld_ );

    posfld_ = new uiOffsetSlicePos( topgrp_ );
    posfld_->setLabels( "Model", "Offset", "Z" );
    posfld_->attachGrp()->attach( rightOf, scalebut_ );
    raypars_.cs_.hrg.set( Interval<int>(1,25), Interval<int>(1,mMAXOffsetIdx) );
    posfld_->setLimitSampling( raypars_.cs_ );
    raypars_.cs_.hrg.setCrlRange( Interval<int>(1,1) );
    posfld_->setCubeSampling( raypars_.cs_ );
    posfld_->positionChg.notify( mCB(this,uiStratSynthDisp,rayTrcParChged) );

    vwr_ = new uiFlatViewer( this );
    vwr_->setInitialSize( uiSize(500,250) ); //TODO get hor sz from laymod disp
    vwr_->setStretch( 2, 2 );
    vwr_->attach( ensureBelow, topgrp_ );
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
    fvsu.withwva( true ).withthumbnail( false ).withcoltabed( false )
	.tba( (int)uiToolBar::Right );
    uiFlatViewStdControl* ctrl = new uiFlatViewStdControl( *vwr_, fvsu );
    ctrl->zoomChanged.notify( mCB(this,uiStratSynthDisp,zoomChg) );

    uiToolBar* tb = ctrl->toolBar();
    CallBack cb( mCB(this,uiStratSynthDisp,rayTrcParPush) );
    tb->addButton( new uiToolButton( tb,"raytrace.png", 
				"Specify ray tracer parameters", cb ) );
}


uiStratSynthDisp::~uiStratSynthDisp()
{
    delete wvlt_;
    deepErase( d2tmodels_ );
}


void uiStratSynthDisp::addTool( const uiToolButtonSetup& bsu )
{
    uiToolButton* tb = new uiToolButton( topgrp_, bsu );
    if ( lasttool_ )
	tb->attach( leftOf, lasttool_ );
    else
	tb->attach( rightBorder );

    tb->attach( ensureRightOf, posfld_->attachGrp() );
    lasttool_ = tb;
}


void uiStratSynthDisp::setDispMrkrs( const char* lnm,
				     const TypeSet<float>& zvals, Color col )
{
    levelname_ = lnm;
    FlatView::Annotation& ann = vwr_->appearance().annot_;
    deepErase( ann.auxdata_ );

    if ( !d2tmodels_.isEmpty() && !zvals.isEmpty() )
    {
	SeisTrcBuf& tbuf = const_cast<SeisTrcBuf&>( curTrcBuf() );
	FlatView::Annotation::AuxData* auxd =
			new FlatView::Annotation::AuxData("Level markers");
	auxd->linestyle_.type_ = LineStyle::None;
	for ( int imdl=0; imdl<d2tmodels_.size(); imdl++ )
	{
	    float tval = zvals[ imdl>=zvals.size() ? zvals.size()-1 :imdl ];
	    if ( !mIsUdf(tval) )
	    {
		tval = d2tmodels_[imdl]->getTime( tval );
		if ( imdl < tbuf.size() )
		    tbuf.get(imdl)->info().pick = tval;

		auxd->markerstyles_ += MarkerStyle2D( MarkerStyle2D::Target,
						      cMarkerSize, col );
		auxd->poly_ += FlatView::Point( imdl+1, tval );
	    }
	}
	if ( auxd->isEmpty() )
	    delete auxd;
	else
	    ann.auxdata_ += auxd;
    }

    vwr_->handleChange( FlatView::Viewer::Annot, true );
}


void uiStratSynthDisp::wvltChg( CallBacker* )
{
    modelChanged();
    wvltChanged.trigger();
}


void uiStratSynthDisp::scalePush( CallBacker* )
{
    SeisTrcBuf& tbuf = const_cast<SeisTrcBuf&>( curTrcBuf() );
    if ( tbuf.isEmpty() ) return;

    bool is2d = SI().has2D();
    if ( is2d && SI().has3D() )
    {
	int res = uiMSG().question( "Type of seismic data to use", "2D", "3D",
					"Cancel", "Specify geometry" );
	if ( res < 0 ) return;
	is2d = res == 1;
    }

    uiSynthToRealScale dlg( this, is2d, tbuf, wvltfld_->getID(), levelname_ );
    if ( dlg.go() )
	vwr_->handleChange( FlatView::Viewer::All );
}


void uiStratSynthDisp::zoomChg( CallBacker* )
{
    zoomChanged.trigger();
}


const uiWorldRect& uiStratSynthDisp::curView( bool indpth ) const
{
    static uiWorldRect wr; wr = vwr_->curView();
    if ( indpth && !d2tmodels_.isEmpty() )
    {
	int mdlidx = longestaimdl_;
	if ( mdlidx >= d2tmodels_.size() )
	    mdlidx = d2tmodels_.size()-1;

	const TimeDepthModel& d2t = *d2tmodels_[mdlidx];
	wr.setTop( d2t.getDepth( (float)wr.top() ) );
	wr.setBottom( d2t.getDepth( (float)wr.bottom() ) );
    }
    return wr;
}


const FlatDataPack* uiStratSynthDisp::dataPack() const
{
    const FlatDataPack* dp = vwr_->pack( true );
    return dp ? dp : vwr_->pack( false );
}


const SeisTrcBuf& uiStratSynthDisp::curTrcBuf() const
{
    const FlatDataPack* dp = dataPack();
    mDynamicCastGet(const SeisTrcBufDataPack*,tbdp,dp)
    if ( !tbdp )
    {
	static SeisTrcBuf emptybuf( false );
	return emptybuf;
    }
    return tbdp->trcBuf();
}


DataPack::FullID uiStratSynthDisp::packID() const
{
    const FlatDataPack* dp = dataPack();
    if ( !dp ) return DataPack::cNoID();
    return DataPack::FullID( DataPackMgr::FlatID(), dp->id() );
}


int uiStratSynthDisp::getVelIdx( bool& isvel ) const
{
    //TODO this requires a lot of work. Can be auto-detected form property
    // StdType but sometimes user has many velocity providers:
    // - Many versions (different measurements, sources, etc)
    // - Sonic vs velocity
    isvel = true; return 1; // This is what the simple generator generates
}


int uiStratSynthDisp::getDenIdx( bool& isden ) const
{
    //TODO support:
    // - density itself
    // - den = ai / vel
    isden = true; return 2; // This is what the simple generator generates
}


#define mErrRet(s) { if ( s ) uiMSG().error(s); return; }

void uiStratSynthDisp::modelChanged()
{
    HorSampling hs; 
    hs.set( Interval<int>(1,lm_.size()), Interval<int>(1,mMAXOffsetIdx) );
    raypars_.cs_.hrg.limitTo( hs );

    NotifyStopper ns( posfld_->positionChg );
    posfld_->setLimitSampling( raypars_.cs_ );

    if ( raytrcpardlg_ )
	raytrcpardlg_->setLimitSampling( raypars_.cs_ );

    doModelChange();
}


void uiStratSynthDisp::doModelChange()
{
    MouseCursorChanger mcs( MouseCursor::Busy );
    vwr_->clearAllPacks(); vwr_->setNoViewDone();
    vwr_->control()->zoomMgr().toStart();
    deepErase( vwr_->appearance().annot_.auxdata_ );

    deepErase( d2tmodels_ );
    delete wvlt_;
    wvlt_ = wvltfld_->getWavelet();
    if ( !wvlt_ )
    {
	const char* nm = wvltfld_->getName();
	if ( nm && *nm )
	    mErrRet("Cannot read chosen wavelet")
	else
	    mErrRet(0)
    }

    bool isvel; const int velidx = getVelIdx( isvel );
    bool isden; const int denidx = getDenIdx( isden );
    longestaimdl_ = 0; int maxaimdlsz = 0;
    ObjectSet<RayTracer1D> raytracers;
    if ( lm_.isEmpty() ) 
	mErrRet( 0 )

    Seis::ODRaySynthGenerator synthgen;
    synthgen.setRayParams( raypars_ );
    synthgen.setWavelet( wvlt_, OD::UsePtr );
    const int nraimdls = lm_.size();

    for ( int iseq=0; iseq<nraimdls; iseq++ )
    {
	const Strat::LayerSequence& seq = lm_.sequence( iseq );
	AIModel aimod; seq.getAIModel( aimod, velidx, denidx, isvel, isden );
	if ( aimod.size() > maxaimdlsz )
	    { maxaimdlsz = aimod.size(); longestaimdl_ = iseq; }
	synthgen.addModel( aimod );
    }
    uiTaskRunner tr( this );
    if ( !synthgen.doWork( tr ) )
	mErrRet( synthgen.errMsg() );

    SeisTrcBuf* tbuf = new SeisTrcBuf( true );
    const int crlstep = SI().crlStep();
    const BinID bid0( SI().inlRange(false).stop + SI().inlStep(),
	    	      SI().crlRange(false).stop + crlstep );

    ObjectSet<const SeisTrc> trcs; synthgen.getTrcs( trcs );
    if ( trcs.isEmpty() )
	mErrRet(0)

    for ( int imdl=0; imdl<trcs.size(); imdl++ )
    {
	SeisTrc* newtrc = new SeisTrc( *trcs[imdl] );
	const int trcnr = imdl + 1;
	newtrc->info().nr = trcnr;
	newtrc->info().binid = BinID( bid0.inl, bid0.crl + imdl * crlstep );
	newtrc->info().coord = SI().transform( newtrc->info().binid );
	tbuf->add( newtrc );
	TimeDepthModel* d2tm = new TimeDepthModel;
	//synthgen.getTWT( imdl, 0, *d2tm );
	d2tmodels_ += d2tm;
    }

    deepErase( vwr_->appearance().annot_.auxdata_ );
    SeisTrcBufDataPack* dp = new SeisTrcBufDataPack( tbuf, Seis::Line,
	    			SeisTrcInfo::TrcNr, "Seismic" );
    dp->setName( "Synthetics" );
    dp->posData().setRange( true, StepInterval<double>(1,nraimdls,1) );
    const SeisTrc& trc0 = *tbuf->get(0);
    StepInterval<double> zrg( trc0.info().sampling.start,
			      trc0.info().sampling.atIndex(trc0.size()-1),
			      trc0.info().sampling.step );
    dp->posData().setRange( false, zrg );
    DPM(DataPackMgr::FlatID()).add( dp );
    vwr_->setPack( true, dp->id(), false );
    vwr_->setPack( false, dp->id(), false );
}


void uiStratSynthDisp::rayTrcParPush( CallBacker* )
{
    if ( !raytrcpardlg_ )
	raytrcpardlg_ = new uiRayTrcParamsDlg( this, raypars_ );
    raytrcpardlg_->setLimitSampling( posfld_->getLimitSampling() );
    raytrcpardlg_->go();
    raytrcpardlg_->parChged.notify( mCB(this,uiStratSynthDisp,rayTrcParChged) );
}


void uiStratSynthDisp::rayTrcParChged( CallBacker* )
{
    doModelChange();
}


uiRayTrcParamsDlg::uiRayTrcParamsDlg( uiParent* p, 
				Seis::RaySynthGenerator::RayParams& rp ) 
    : uiDialog(p,uiDialog::Setup(
		"Specify ray tracer parameters","",mTODOHelpID).modal(false))
    , parChged( this )
    , raypars_(rp)
{
    setCtrlStyle( LeaveOnly );

    uiGroup* posgrp = new uiGroup( this, "Position group" );

    static const char* dir[] = { "Offset", "Angle", "Model", 0 };
    uiLabeledComboBox* lblb = new uiLabeledComboBox( posgrp, "Direction" );
    directionfld_ = lblb->box();
    directionfld_->addItems( dir );
    directionfld_->selectionChanged.notify( mCB(this,uiRayTrcParamsDlg,dirChg) );

    offsetfld_ = new uiGenInput( posgrp, "Offset range (start/stop)",
	    				FloatInpIntervalSpec() );
    offsetfld_->attach( alignedBelow, lblb );
    offsetfld_->setValue( Interval<float>( 0, 
				mMAXOffsetIdx*rp.cs_.hrg.crlRange().step ) );
    offsetfld_->setElemSzPol( uiObject::Small );
    offsetstepfld_ = new uiGenInput( posgrp, "step" );
    offsetstepfld_->setValue( rp.cs_.hrg.crlRange().step );
    offsetstepfld_->attach( rightOf, offsetfld_ );
    offsetstepfld_->setElemSzPol( uiObject::Small );

    uiSeparator* sp = new uiSeparator( this, "Offset/Setup sep" );
    sp->attach( stretchedBelow, posgrp );

    sourcerecfld_ = new uiGenInput( this, "Source / Receiver depth",
	    			FloatInpIntervalSpec() );
    sourcerecfld_->setValue(Interval<float>(rp.sourcedpt_,rp.receivdpt_));
    sourcerecfld_->attach( centeredBelow, posgrp );
    sourcerecfld_->attach( ensureBelow, sp );

    vp2vsfld_ = new uiGenInput( this, "Vp, Vs factors (a/b)", 
	    			FloatInpIntervalSpec() );
    vp2vsfld_->setValue( Interval<float>( rp.setup_.pvel2svelafac_,
					  rp.setup_.pvel2svelbfac_) );
    vp2vsfld_->attach( alignedBelow, sourcerecfld_ );

    CallBack parcb( mCB(this,uiRayTrcParamsDlg,parChg) );

    offsetfld_->valuechanged.notify( parcb );
    offsetstepfld_->valuechanged.notify( parcb );
    sourcerecfld_->valuechanged.notify( parcb );
    vp2vsfld_->valuechanged.notify( parcb );
}


void uiRayTrcParamsDlg::setLimitSampling( const CubeSampling& cs )
{
    limitcs_ = cs;
    dirChg(0);
}


void uiRayTrcParamsDlg::dirChg( CallBacker* )
{
    CubeSampling cs = limitcs_;
    const int idx = directionfld_->currentItem();
    const char* lbl = idx == 0 ? "Offset" : ( idx == 1 ? "Angle" : "Model" );
    if ( idx < 2 )
    {
	cs.hrg.setCrlRange( Interval<int>( 1, 1 ) );
    }
    else
    {
	cs.hrg.setInlRange( Interval<int>( 1, 1 ) );
    }
    raypars_.cs_ = cs;
    parChg( 0 );
}


void uiRayTrcParamsDlg::parChg( CallBacker* )
{
    raypars_.offsetsampling_.start = offsetfld_->getFInterval().start;
    raypars_.offsetsampling_.step = offsetstepfld_->getfValue();
    raypars_.setup_.sourcedepth_ = sourcerecfld_->getFInterval().start;
    raypars_.setup_.receiverdepth_ = sourcerecfld_->getFInterval().stop;
    raypars_.setup_.pvel2svelafac_ = vp2vsfld_->getFInterval().start;
    raypars_.setup_.pvel2svelbfac_ = vp2vsfld_->getFInterval().stop;
    parChged.trigger();
}



uiOffsetSlicePos::uiOffsetSlicePos( uiParent* p )
    : uiSlicePos2DView( p )
{
    mDynamicCastGet(uiMainWin*,mw,p)
    if ( mw )
	mw->removeToolBar( toolbar_ );
    else
    {
	mDynamicCastGet(uiMainWin*,mw,p->mainwin())
	if ( mw )
	    mw->removeToolBar( toolbar_ );
    }

    attachgrp_ = new uiGroup( p, "Attach group" );

    label_ = new uiLabel( attachgrp_, "Crl" );
    sliceposbox_ = new uiSpinBox( attachgrp_, 0, "Slice position" );
    sliceposbox_->valueChanging.notify( mCB(this,uiOffsetSlicePos,slicePosChg));
    sliceposbox_->attach( rightOf, label_ );

    uiLabel* steplabel = new uiLabel( attachgrp_, "Step" );
    steplabel->attach( rightOf, sliceposbox_ );

    slicestepbox_ = new uiSpinBox( attachgrp_, 0, "Slice step" );
    slicestepbox_->valueChanging.notify(mCB(this,uiOffsetSlicePos,sliceStepChg));
    slicestepbox_->attach( rightOf, steplabel );

    prevbut_ = new uiToolButton( attachgrp_, "prevpos.png", "Previous position",
				mCB(this,uiOffsetSlicePos,prevCB) );
    prevbut_->attach( rightOf, slicestepbox_ );
    nextbut_ = new uiToolButton( attachgrp_, "nextpos.png", "Next position",
				 mCB(this,uiOffsetSlicePos,nextCB) );
    nextbut_->attach( rightOf, prevbut_ );
}

