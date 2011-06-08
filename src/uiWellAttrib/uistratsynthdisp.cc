/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratsynthdisp.cc,v 1.40 2011-06-08 07:22:25 cvsbruno Exp $";

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
#include "prestackgather.h"
#include "survinfo.h"
#include "synthseis.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "velocitycalc.h"
#include "wavelet.h"


static const int cMarkerSize = 6;

#define mStdOffset 3000
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
    posfld_->attachGrp()->setSensitive( false );
    posfld_->setCubeSampling( raypars_.cs_ );
    posfld_->positionChg.notify( mCB(this,uiStratSynthDisp,rayTrcPosChged) );

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


#define mErrRet(s,act) { if ( s ) uiMSG().error(s); act; }

void uiStratSynthDisp::modelChanged()
{
    NotifyStopper ns( posfld_->positionChg );
    CubeSampling cs( raypars_.cs_ ); 
    HorSampling& hs = cs.hrg;
    hs.setInlRange( Interval<int>(1,lm_.size()) );
    hs.setCrlRange( Interval<int>(0,0) );
    raypars_.cs_ = cs;
    posfld_->setCubeSampling( raypars_.cs_ );
    hs.setCrlRange( Interval<int>(0,mStdOffset) );
    posfld_->setLimitSampling( cs );
    posfld_->attachGrp()->setSensitive( true );

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
	    mErrRet("Cannot read chosen wavelet",return;)
	else
	    mErrRet(0,return)
    }
    DataPack* dp = genNewDataPack( raypars_, d2tmodels_, false );
    if ( !dp ) return;

    int maxaimodelsz =  0;
    for ( int idx=0; idx<d2tmodels_.size(); idx++ )
    {
	if ( d2tmodels_[idx]->size() > maxaimodelsz )
	    { maxaimodelsz = d2tmodels_[idx]->size(); longestaimdl_ = idx; }
    }

    deepErase( vwr_->appearance().annot_.auxdata_ );
    DPM(DataPackMgr::FlatID()).add( dp );
    vwr_->setPack( true, dp->id(), false );
    vwr_->setPack( false, dp->id(), false );
}


DataPack* uiStratSynthDisp::genNewDataPack( const RayParams& raypars,
					ObjectSet<const TimeDepthModel>& d2ts, 
					bool isgather ) const
{
    const CubeSampling& cs = raypars.cs_;
    TypeSet<float> offsets;
    for ( int idx=0; idx<cs.nrCrl(); idx++ )
	offsets += cs.hrg.crlRange().atIndex(idx);

    Seis::ODRaySynthGenerator synthgen;
    synthgen.setRayParams( offsets, raypars.setup_, raypars.usenmotimes_ );
    synthgen.setWavelet( wvlt_, OD::UsePtr );
    const int nraimdls = cs.nrInl();

    bool isvel; const int velidx = getVelIdx( isvel );
    bool isden; const int denidx = getDenIdx( isden );
    if ( lm_.isEmpty() ) 
	mErrRet( 0, return 0; )

    for ( int iseq=0; iseq<cs.nrInl(); iseq++ )
    {
	int seqidx = cs.hrg.inlRange().atIndex(iseq)-1;
	const Strat::LayerSequence& seq = lm_.sequence( seqidx );
	AIModel aimod; seq.getAIModel( aimod, velidx, denidx, isvel, isden );
	synthgen.addModel( aimod );
    }
    TaskRunner tr;
    if ( !tr.execute( synthgen ) )
	mErrRet( synthgen.errMsg(), return 0 );

    SeisTrcBuf* tbuf = new SeisTrcBuf( true );
    const int crlstep = SI().crlStep();
    const BinID bid0( SI().inlRange(false).start + SI().inlStep(),
	    	      SI().crlRange(false).start + crlstep );

    ObjectSet<PreStack::Gather> gathers;
    ObjectSet<const SeisTrc> trcs;
    for ( int imdl=0; imdl<nraimdls; imdl++ )
    {
	Seis::RaySynthGenerator::RayModel& rm = 
	    const_cast<Seis::RaySynthGenerator::RayModel&>( 
						    synthgen.result( imdl ) );
	trcs.erase(); 
	if ( raypars.dostack_ )
	    trcs += rm.stackedTrc();
	else
	    rm.getTraces( trcs, true );

	if ( trcs.isEmpty() )
	    continue;

	for ( int idx=0; idx<trcs.size(); idx++ )
	{
	    SeisTrc* trc = const_cast<SeisTrc*>( trcs[idx] );
	    const int trcnr = imdl + 1;
	    trc->info().nr = trcnr;
	    trc->info().binid = BinID( bid0.inl, bid0.crl + imdl * crlstep );
	    trc->info().coord = SI().transform( trc->info().binid );
	    tbuf->add( trc );
	}
	if ( isgather )
	{
	    PreStack::Gather* gather = new PreStack::Gather();
	    if ( !gather->readFrom( *tbuf, 0 ) )
		continue;
	    gathers += gather;
	}
	rm.getD2T( d2ts, true );
    }
    if ( ( isgather && gathers.isEmpty() ) || ( !isgather && tbuf->isEmpty() ) )
	mErrRet("No seismic traces genereated ", return 0)

    DataPack* dp =0;
   
    if ( isgather ) 
    {
	delete tbuf;
	PreStack::GatherSetDataPack* pdp = new PreStack::GatherSetDataPack(
							"GatherSet", gathers );
	dp = dynamic_cast<DataPack*>(pdp);
    }
    else
    {
	SeisTrcBufDataPack* tdp = new SeisTrcBufDataPack( 
			    tbuf, Seis::Line, SeisTrcInfo::TrcNr, "Seismic" ) ;
	const SeisTrc& trc0 = *tbuf->get(0);
	StepInterval<double> zrg(  trc0.info().sampling.start,
				   trc0.info().sampling.atIndex(trc0.size()-1),
				   trc0.info().sampling.step );
	tdp->posData().setRange( true, StepInterval<double>(1,tbuf->size(),1) );
	tdp->posData().setRange( false, zrg );

	dp = dynamic_cast<DataPack*>(tdp);
    }
    dp->setName( "Synthetics" );
    return dp;
}


void uiStratSynthDisp::rayTrcParPush( CallBacker* )
{
    if ( !raytrcpardlg_ )
	raytrcpardlg_ = new uiRayTrcParamsDlg( this, raypars_ );
    raytrcpardlg_->setLimitSampling( posfld_->getLimitSampling() );
    raytrcpardlg_->go();
    raytrcpardlg_->button( uiDialog::OK )->activated.notify(
			mCB(this,uiStratSynthDisp,rayTrcParChged) );
}


void uiStratSynthDisp::rayTrcPosChged( CallBacker* )
{
    raypars_.cs_ = posfld_->getCubeSampling();
    doModelChange();
}


void uiStratSynthDisp::rayTrcParChged( CallBacker* )
{
    NotifyStopper ns( posfld_->positionChg );
    posfld_->setCubeSampling( raypars_.cs_ );
    doModelChange();
}


uiRayTrcParamsDlg::uiRayTrcParamsDlg( uiParent* p, RayParams& rp ) 
    : uiDialog(p,uiDialog::Setup(
		"Specify ray tracer parameters","",mTODOHelpID).modal(false))
{
    setCtrlStyle( DoAndStay );

    static const char* dir[] = { "Model", "Offset", 0 };
    uiLabeledComboBox* lblb = new uiLabeledComboBox( this, "Direction" );
    directionfld_ = lblb->box();
    directionfld_->addItems( dir );
    lblb->attach( hCentered );
    CallBack dircb( mCB(this,uiRayTrcParamsDlg,dirChg ) );
    directionfld_->selectionChanged.notify( dircb );

    raytrcpargrp_ = new uiRayTrcParamsGrp( this, rp );
    raytrcpargrp_->attach( ensureBelow, lblb );
}


void uiRayTrcParamsDlg::setLimitSampling( const CubeSampling& cs )
{
    raytrcpargrp_->setLimitSampling( cs );
    dirChg(0);
}


void uiRayTrcParamsDlg::dirChg( CallBacker* )
{
    const int idx = directionfld_->currentItem();
    raytrcpargrp_->setOffSetDirection( idx > 0 );
}


bool uiRayTrcParamsDlg::acceptOK( CallBacker* )
{
    raytrcpargrp_->doUpdate();
    return false;
}


uiOffsetSlicePos::uiOffsetSlicePos( uiParent* p )
    : uiSlicePos2DView( p )
{
    mDynamicCastGet(uiMainWin*,mw,p)
    if ( mw )
	mw->removeToolBar( toolbar_ );
    else
    {
	mDynamicCastGet(uiMainWin*,pmw,p->mainwin())
	if ( pmw )
	    pmw->removeToolBar( toolbar_ );
    }

    attachgrp_ = new uiGroup( p, "Attach group" );

    label_ = new uiLabel( attachgrp_, "Crl" );
    sliceposbox_ = new uiSpinBox( attachgrp_, 0, "Slice position" );
    sliceposbox_->valueChanging.notify( mCB(this,uiOffsetSlicePos,slicePosChg));
    sliceposbox_->valueChanged.notify( mCB(this,uiOffsetSlicePos,slicePosChg));
    sliceposbox_->attach( rightOf, label_ );

    uiLabel* steplabel = new uiLabel( attachgrp_, "Step" );
    steplabel->attach( rightOf, sliceposbox_ );

    slicestepbox_ = new uiSpinBox( attachgrp_, 0, "Slice step" );
    slicestepbox_->valueChanging.notify(
	    			mCB(this,uiOffsetSlicePos,sliceStepChg) );
    slicestepbox_->attach( rightOf, steplabel );

    prevbut_ = new uiToolButton( attachgrp_, "prevpos.png", "Previous position",
				mCB(this,uiOffsetSlicePos,prevCB) );
    prevbut_->attach( rightOf, slicestepbox_ );
    nextbut_ = new uiToolButton( attachgrp_, "nextpos.png", "Next position",
				 mCB(this,uiOffsetSlicePos,nextCB) );
    nextbut_->attach( rightOf, prevbut_ );
}



uiRayTrcParamsGrp::uiRayTrcParamsGrp( uiParent* p, RayParams& rp ) 
    : uiGroup(p,"Ray paramrs group" )
    , raypars_(rp)
    , isoffsetdir_(true)		  
    , previsoffsetdir_(true)		  
{
    offsetfld_ = new uiGenInput( this, "Offset range(m) (start/stop)",
	    				IntInpIntervalSpec() );
    offsetfld_->setValue( Interval<int>( 0, mStdOffset ) );
    offsetfld_->setElemSzPol( uiObject::Small );
    offsetstepfld_ = new uiGenInput( this, "step" );
    offsetstepfld_->setValue( rp.cs_.hrg.crlRange().step );
    offsetstepfld_->attach( rightOf, offsetfld_ );
    offsetstepfld_->setElemSzPol( uiObject::Small );

    nmobox_ = new uiCheckBox( this, "NMO corrections" );
    nmobox_->attach( alignedBelow, offsetfld_ );
    nmobox_->setChecked( raypars_.usenmotimes_ );

    stackbox_ = new uiCheckBox( this, "Stack" );
    stackbox_->setChecked( raypars_.dostack_ );
    stackbox_->attach( alignedBelow, offsetfld_ );
    stackbox_->activated.notify( mCB(this,uiRayTrcParamsGrp,updateCB) );

    uiSeparator* sp = new uiSeparator( this, "Offset/Setup sep" );
    sp->attach( stretchedBelow, nmobox_ );

    sourcerecfld_ = new uiGenInput( this, "Source / Receiver depth",
	    			FloatInpIntervalSpec() );
    sourcerecfld_->setValue(Interval<float>(rp.setup_.sourcedepth_,
					    rp.setup_.receiverdepth_));
    sourcerecfld_->attach( centeredBelow, nmobox_ );
    sourcerecfld_->attach( ensureBelow, sp );

    vp2vsfld_ = new uiGenInput( this, "Vp, Vs factors (a/b)", 
	    			FloatInpIntervalSpec() );
    vp2vsfld_->setValue( Interval<float>( rp.setup_.pvel2svelafac_,
					  rp.setup_.pvel2svelbfac_) );
    vp2vsfld_->attach( alignedBelow, sourcerecfld_ );
    updateCB( 0 );
}


void uiRayTrcParamsGrp::setLimitSampling( const CubeSampling& cs )
{
    limitcs_ = cs;
}


void uiRayTrcParamsGrp::updateCB( CallBacker* )
{
    if ( isoffsetdir_ != previsoffsetdir_ )
    {	
	NotifyStopper( stackbox_->activated );
	stackbox_->setChecked( false );
    }

    raypars_.cs_ = limitcs_;
    nmobox_->display( isoffsetdir_ );
    stackbox_->display( !isoffsetdir_ );
    offsetfld_->display( isoffsetdir_ || stackbox_->isChecked() );
    offsetstepfld_->display( isoffsetdir_ || stackbox_->isChecked() );

    if ( isoffsetdir_ || stackbox_->isChecked())
    {
	raypars_.cs_.hrg.setCrlRange( offsetfld_->getIInterval() );
	raypars_.cs_.hrg.step.crl = (int)offsetstepfld_->getfValue();
	if ( !stackbox_->isChecked() )
	    raypars_.cs_.hrg.setInlRange( Interval<int>(1,1) ); //model idx to 1
    }
    if ( !isoffsetdir_ )
    {
	raypars_.cs_.hrg.step.inl = 1;
	raypars_.cs_.hrg.setCrlRange( Interval<int>( 0, 0 ) ); //offset to 0
    }

    raypars_.setup_.sourcedepth_ = sourcerecfld_->getFInterval().start;
    raypars_.setup_.receiverdepth_ = sourcerecfld_->getFInterval().stop;
    raypars_.setup_.pvel2svelafac_ = vp2vsfld_->getFInterval().start;
    raypars_.setup_.pvel2svelbfac_ = vp2vsfld_->getFInterval().stop;
    raypars_.usenmotimes_ = !isoffsetdir_ ? true : nmobox_->isChecked();
    raypars_.dostack_ = stackbox_->isChecked();

    previsoffsetdir_ = isoffsetdir_;
}
