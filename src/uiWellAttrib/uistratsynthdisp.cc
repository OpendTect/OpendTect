/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratsynthdisp.cc,v 1.53 2011-08-11 14:06:38 cvsbruno Exp $";

#include "uistratsynthdisp.h"
#include "uistratsynthdisp2crossplot.h"
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
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "velocitycalc.h"
#include "wavelet.h"


static const int cMarkerSize = 6;

Notifier<uiStratSynthDisp>& uiStratSynthDisp::fieldsCreated()
{
    static Notifier<uiStratSynthDisp> FieldsCreated(0);
    return FieldsCreated;
}


uiStratSynthDisp::uiStratSynthDisp( uiParent* p, const Strat::LayerModel& lm )
    : uiGroup(p,"LayerModel synthetics display")
    , tmpsynthetic_(0)   
    , wvlt_(0)
    , lm_(lm)
    , d2tmodels_(0)	     
    , stratsynth_(*new StratSynth(lm_))
    , raypars_(*new RayParams)				    
    , wvltChanged(this)
    , zoomChanged(this)
    , layerPropSelNeeded(this)
    , longestaimdl_(0)
    , lasttool_(0)
    , raytrcpardlg_(0)
{
    topgrp_ = new uiGroup( this, "Top group" );
    topgrp_->setFrame( true );
    topgrp_->setStretch( 2, 0 );

    uiToolButton* layertb = new uiToolButton( topgrp_, "defraytraceprops.png", 
				    "Specify synthetic layers properties", 
				    mCB(this,uiStratSynthDisp,layerPropsPush) );

    uiSeparator* pp2wvltsep = new uiSeparator( topgrp_, "Prop2Wvlt Sep", false);
    pp2wvltsep->attach( stretchedRightTo, layertb );

    wvltfld_ = new uiSeisWaveletSel( topgrp_ );
    wvltfld_->newSelection.notify( mCB(this,uiStratSynthDisp,wvltChg) );
    wvltfld_->setFrame( false );
    wvltfld_->attach( rightOf, pp2wvltsep );

    scalebut_ = new uiPushButton( topgrp_, "Scale", false );
    scalebut_->activated.notify( mCB(this,uiStratSynthDisp,scalePush) );
    scalebut_->attach( rightOf, wvltfld_ );

    uiSeparator* wvlt2raysep = new uiSeparator(topgrp_, "Prop2Wvlt Sep", false);
    wvlt2raysep->attach( stretchedRightTo, scalebut_ );

    uiToolButton* rttb = new uiToolButton( topgrp_, "raytrace.png", 
				    "Specify ray tracer parameters", 
				    mCB(this,uiStratSynthDisp,rayTrcParPush) );
    rttb->attach( rightOf, wvlt2raysep );

    posfld_ = new uiOffsetSlicePos( topgrp_ );
    posfld_->setLabels( "Model", "Offset", "Z" );
    posfld_->attachGrp()->attach( rightOf, rttb );
    posfld_->attachGrp()->setSensitive( false );
    posfld_->setCubeSampling( raypars_.cs_ );
    posfld_->positionChg.notify( mCB(this,uiStratSynthDisp,rayTrcPosChged) );

    modelgrp_ = new uiGroup( this, "Model group" );
    modelgrp_->attach( ensureBelow, topgrp_ );
    modelgrp_->setFrame( true );
    modelgrp_->setStretch( 2, 0 );

    modellist_ = new uiLabeledComboBox( modelgrp_, "View ", "" );
    modellist_->attach( hCentered );
    modellist_->box()->selectionChanged.notify(
	    				mCB(this,uiStratSynthDisp,dataSetSel) );
    cleanSynthetics();

    uiPushButton* createssynthbut 
		= new uiPushButton( modelgrp_, "Create synthetics", false );
    createssynthbut->activated.notify(mCB(this,uiStratSynthDisp,addSynth2List));
    modellist_->attach( ensureRightOf, createssynthbut );

    vwr_ = new uiFlatViewer( this );
    vwr_->setInitialSize( uiSize(500,250) ); //TODO get hor sz from laymod disp
    vwr_->setStretch( 2, 2 );
    vwr_->attach( ensureBelow, modelgrp_ );
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
}


uiStratSynthDisp::~uiStratSynthDisp()
{
    delete &raypars_;
    delete wvlt_; wvlt_ = 0;
    delete &stratsynth_;
    deepErase( synthetics_ );
    delete tmpsynthetic_;
}


void uiStratSynthDisp::layerPropsPush( CallBacker* )
{
    layerPropSelNeeded.trigger();
}


void uiStratSynthDisp::addTool( const uiToolButtonSetup& bsu )
{
    uiToolButton* tb = new uiToolButton( modelgrp_, bsu );
    if ( lasttool_ )
	tb->attach( leftOf, lasttool_ );
    else
	tb->attach( rightBorder );

    modellist_->attach( ensureLeftOf, tb );
    lasttool_ = tb;
    tb->setSensitive( false );
}


void uiStratSynthDisp::cleanSynthetics()
{
    deepErase( synthetics_ );
    modellist_->box()->setEmpty();
    modellist_->box()->addItem( "Free view" );
    modellist_->setSensitive( false );
    if ( lasttool_ )
	lasttool_->setSensitive( false );
}


void uiStratSynthDisp::setDispMrkrs( const char* lnm,
				     const TypeSet<float>& zvals, Color col )
{
    levelname_ = lnm;
    FlatView::Annotation& ann = vwr_->appearance().annot_;
    deepErase( ann.auxdata_ );

    if ( d2tmodels_ && !d2tmodels_->isEmpty() && !zvals.isEmpty() )
    {
	SeisTrcBuf& tbuf = const_cast<SeisTrcBuf&>( curTrcBuf() );
	FlatView::Annotation::AuxData* auxd =
			new FlatView::Annotation::AuxData("Level markers");
	auxd->linestyle_.type_ = LineStyle::None;
	for ( int imdl=0; imdl<d2tmodels_->size(); imdl++ )
	{
	    float tval = zvals[ imdl>=zvals.size() ? zvals.size()-1 :imdl ];
	    if ( !mIsUdf(tval) )
	    {
		tval = (*d2tmodels_)[imdl]->getTime( tval );
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
    doModelChange();
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


#define mErrRet(s,act) { if ( s ) uiMSG().error(s); act; }

void uiStratSynthDisp::modelChanged()
{
    cleanSynthetics();

    NotifyStopper ns( posfld_->positionChg );
    CubeSampling cs( raypars_.cs_ ); 
    HorSampling& hs = cs.hrg;
    hs.setInlRange( Interval<int>(1,lm_.size()) );
    hs.setCrlRange( Interval<int>(0,0) );
    raypars_.cs_ = cs;
    posfld_->setCubeSampling( raypars_.cs_ );
    hs.setCrlRange( Interval<int>(0,uiRayTracer1D::sKeyStdMaxOffset()) );
    posfld_->setLimitSampling( cs );
    posfld_->attachGrp()->setSensitive( true );

    if ( raytrcpardlg_ )
	raytrcpardlg_->setLimitSampling( raypars_.cs_ );

    doModelChange();
}


void uiStratSynthDisp::displaySynthetics( const SyntheticData* sd )
{
    vwr_->clearAllPacks(); vwr_->setNoViewDone();
    vwr_->control()->zoomMgr().toStart();
    deepErase( vwr_->appearance().annot_.auxdata_ );

    if ( !sd ) return;

    DataPack::FullID dpid = sd->packid_;
    DataPackMgr::ID pmgrid = DataPackMgr::getID( dpid );
    DataPack* dp = DPM(pmgrid).obtain( DataPack::getID(dpid) );
    mDynamicCastGet(PreStack::GatherSetDataPack*,gsetdp,dp)
    if ( gsetdp ) 
    {
	FlatDataPack* gdp = new PreStack::Gather(*gsetdp->getGathers()[0]);
	dp = gdp; 
	DPM(DataPackMgr::FlatID()).add( dp );
    }
    if ( !dp ) return;

    d2tmodels_ = &sd->d2tmodels_;
    for ( int idx=0; idx<d2tmodels_->size(); idx++ )
    {
	int maxaimodelsz =  0;
	if ( (*d2tmodels_)[idx]->size() > maxaimodelsz )
	    { maxaimodelsz = (*d2tmodels_)[idx]->size(); longestaimdl_ = idx; }
    }

    deepErase( vwr_->appearance().annot_.auxdata_ );
    vwr_->setPack( true, dp->id(), false );
    vwr_->setPack( false, dp->id(), false );
}


void uiStratSynthDisp::doModelChange()
{
    MouseCursorChanger mcs( MouseCursor::Busy );
    delete wvlt_; wvlt_ = wvltfld_->getWavelet();
    delete tmpsynthetic_; tmpsynthetic_ = 0; 

    stratsynth_.setWavelet( *wvlt_ );
    d2tmodels_ = 0;
    BufferString errmsg;

    const SyntheticData* sd = 0;
    const int seldataidx = modellist_->box()->currentItem(); 
    topgrp_->setSensitive( seldataidx == 0 );
    if ( seldataidx > 0 )
    {
	 sd = synthetics_[seldataidx-1];
    }
    else
    {
	sd = stratsynth_.generate( raypars_, false, &errmsg );
	tmpsynthetic_ = sd;
    }

    displaySynthetics( sd );
    if ( !errmsg.isEmpty() )
	mErrRet( errmsg.buf(), return )
}


void uiStratSynthDisp::addSynth2List( CallBacker* )
{
    uiStratSynthDisp2Crossplot dlg( this, raypars_, getLimitSampling() ); 
    if ( dlg.go() )
    {
	if ( modellist_->box()->isPresent( dlg.rayParam().synthname_ ) )
	    mErrRet( "Name is already present, please specify another name", 
		    return );

	const SyntheticData* sd = 
	    		stratsynth_.generate( dlg.rayParam(),dlg.isPS() );
	if ( sd )
	{
	    synthetics_ += sd; 
	    modellist_->box()->addItem( sd->name() );
	}
    }
    modellist_->setSensitive( !synthetics_.isEmpty() );
    lasttool_->setSensitive( !synthetics_.isEmpty() );
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


const CubeSampling& uiStratSynthDisp::getLimitSampling() const
{
    return posfld_->getLimitSampling();
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


void uiStratSynthDisp::dataSetSel( CallBacker* )
{
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

    raytrcpargrp_ = new uiRayTrcParamsGrp( this, uiRayTrcParamsGrp::Setup(rp) );
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



uiRayTrcParamsGrp::uiRayTrcParamsGrp( uiParent* p, const Setup& su )
    : uiGroup(p,"Ray paramrs group" )
    , raypars_(su.raypars_)
    , isoffsetdir_(su.offsetdir_)		  
    , previsoffsetdir_(su.offsetdir_)
{
    nmobox_ = new uiCheckBox( this, "NMO corrections" );
    nmobox_->setChecked( raypars_.usenmotimes_ );
    nmobox_->attach( hCentered );

    stackfld_ = new uiGenInput( this, "",
			    BoolInpSpec(true, "Stack", "Zero Offset" ) );
    stackfld_->setValue( raypars_.dostack_ );
    stackfld_->valuechanged.notify( mCB(this,uiRayTrcParamsGrp,updateCB) );
    stackfld_->attach( hCentered );

    uiRayTracer1D::Setup rsu(0);
    rsu.dooffsets_ = true; 
    raytrace1dgrp_ = new uiRayTracer1D( this, rsu );
    raytrace1dgrp_->attach( ensureBelow, stackfld_ );

    updateCB( 0 );
}


void uiRayTrcParamsGrp::setLimitSampling( const CubeSampling& cs )
{
    limitcs_ = cs;
    updateCB(0);
}


void uiRayTrcParamsGrp::updateCB( CallBacker* )
{
    if ( isoffsetdir_ != previsoffsetdir_ )
    {	
	NotifyStopper( stackfld_->valuechanged );
	stackfld_->setValue( false );
    }
    raypars_.cs_ = limitcs_;
    nmobox_->display( isoffsetdir_ );
    stackfld_->display( !isoffsetdir_ );
    const bool isstacked = stackfld_->getBoolValue();
    raytrace1dgrp_->displayOffsetFlds( isoffsetdir_ || isstacked );

    if ( isoffsetdir_ || isstacked )
    {
	StepInterval<float> offsetrg;
	raytrace1dgrp_->fill( raypars_.setup_ );
	raytrace1dgrp_->getOffsets( offsetrg );
	raypars_.cs_.hrg.setCrlRange( 
		Interval<int>( (int)offsetrg.start, (int)offsetrg.stop ) );
	raypars_.cs_.hrg.step.crl = (int)offsetrg.step;
	if ( !isstacked )
	    raypars_.cs_.hrg.setInlRange( Interval<int>(1,1) ); //model idx to 1
    }
    if ( !isoffsetdir_ )
    {
	raypars_.cs_.hrg.step.inl = 1;
	raypars_.cs_.hrg.setCrlRange( Interval<int>( 0, 0 ) ); //offset to 0
    }
    raypars_.usenmotimes_ = !isoffsetdir_ ? true : nmobox_->isChecked();
    raypars_.dostack_ = isstacked;

    previsoffsetdir_ = isoffsetdir_;
}
