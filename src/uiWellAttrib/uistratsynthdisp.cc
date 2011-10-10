/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratsynthdisp.cc,v 1.65 2011-10-10 10:14:30 cvsbruno Exp $";

#include "uistratsynthdisp.h"
#include "uiseiswvltsel.h"
#include "uisynthtorealscale.h"
#include "uicombobox.h"
#include "uiflatviewer.h"
#include "uiflatviewmainwin.h"
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

mDefineInstanceCreatedNotifierAccess(uiStratSynthDisp)


uiStratSynthDisp::uiStratSynthDisp( uiParent* p, const Strat::LayerModel& lm )
    : uiGroup(p,"LayerModel synthetics display")
    , d2tmodels_(0)	     
    , stratsynth_(*new StratSynth(lm))
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
    topgrp_ = new uiGroup( this, "Top group" );
    topgrp_->setFrame( true );
    topgrp_->setStretch( 2, 0 );

    uiToolButton* layertb = new uiToolButton( topgrp_, "defraytraceprops.png", 
				    "Specify synthetic layers properties", 
				    mCB(this,uiStratSynthDisp,layerPropsPush) );

    uiToolButton* rttb = new uiToolButton( topgrp_, "raytrace.png", 
				    "Specify ray tracer parameters", 
				    mCB(this,uiStratSynthDisp,rayTrcParPush) );
    rttb->attach( rightOf, layertb );

    uiSeparator* pp2wvltsep = new uiSeparator( topgrp_, "Prop2Wvlt Sep", false);
    pp2wvltsep->attach( rightTo, rttb );

    wvltfld_ = new uiSeisWaveletSel( topgrp_ );
    wvltfld_->newSelection.notify( mCB(this,uiStratSynthDisp,wvltChg) );
    wvltfld_->setFrame( false );
    wvltfld_->attach( rightOf, pp2wvltsep );

    scalebut_ = new uiPushButton( topgrp_, "Scale", false );
    scalebut_->activated.notify( mCB(this,uiStratSynthDisp,scalePush) );
    scalebut_->attach( rightOf, wvltfld_ );

    uiSeparator* wvlt2raysep = new uiSeparator(topgrp_, "Prop2Wvlt Sep", false);
    wvlt2raysep->attach( stretchedRightTo, scalebut_ );

    uiPushButton* createssynthbut = 
		new uiPushButton( topgrp_, "Add new synthetics", false );
    createssynthbut->activated.notify(mCB(this,uiStratSynthDisp,addSynth2List));
    createssynthbut->attach( rightOf, wvlt2raysep );

    modelgrp_ = new uiGroup( this, "Model group" );
    modelgrp_->attach( ensureBelow, topgrp_ );
    modelgrp_->setFrame( true );
    modelgrp_->setStretch( 2, 0 );

    modellist_ = new uiLabeledComboBox( modelgrp_, "View ", "" );
    modellist_->box()->selectionChanged.notify(
	    				mCB(this,uiStratSynthDisp,dataSetSel) );
    modellist_->setStretch( 0, 0 );

    offsetposfld_ = new uiSynthSlicePos( modelgrp_, "Offset" );
    offsetposfld_->setSensitive( false );
    offsetposfld_->positionChg.notify( 
	    		mCB(this,uiStratSynthDisp,offsetPosChged) );
    offsetposfld_->attach( rightOf, modellist_ );
    offsetposfld_->attach( hCentered );

    uiToolButton* pstb = new uiToolButton( modelgrp_, "nonmocorr64.png", 
				"View Offset Direction", 
				mCB(this,uiStratSynthDisp,viewPreStackPush) );
    pstb->attach( rightOf, offsetposfld_);

    cleanSynthetics();

    vwr_ = new uiFlatViewer( this );
    vwr_->setInitialSize( uiSize(600,250) ); //TODO get hor sz from laymod disp
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

    mTriggerInstanceCreatedNotifier();
}


uiStratSynthDisp::~uiStratSynthDisp()
{
    delete &stratsynth_;
}


const Strat::LayerModel& uiStratSynthDisp::layerModel() const
{
    return stratsynth_.layerModel();
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

    lasttool_ = tb;
}


void uiStratSynthDisp::cleanSynthetics()
{
    modellist_->box()->setEmpty();
    modellist_->box()->addItem( "Free view" );
    modellist_->setSensitive( false );
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
    {
	MultiID mid( dlg.selWvltID() );
	if ( mid.isEmpty() )
	    pErrMsg( "Huh" );
	else
	    wvltfld_->setInput( mid );
	vwr_->handleChange( FlatView::Viewer::All );
    }
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
    doModelChange();
}


void uiStratSynthDisp::displaySynthetic( const SyntheticData* sd )
{
    displayPostStackSynthetic( sd );
    displayPreStackSynthetic( sd );
}


void uiStratSynthDisp::displayPostStackSynthetic( const SyntheticData* sd )
{
    vwr_->clearAllPacks(); vwr_->setNoViewDone();
    vwr_->control()->zoomMgr().toStart();
    deepErase( vwr_->appearance().annot_.auxdata_ );

    if ( !sd ) return;
    const DataPack* dp = sd->getPack( false );
    if ( !dp ) return;

    d2tmodels_ = &sd->d2tmodels_;
    for ( int idx=0; idx<d2tmodels_->size(); idx++ )
    {
	int maxaimodelsz =  0;
	if ( (*d2tmodels_)[idx]->size() > maxaimodelsz )
	    { maxaimodelsz = (*d2tmodels_)[idx]->size(); longestaimdl_ = idx; }
    }

    vwr_->setPack( true, dp->id(), false );
    vwr_->setPack( false, dp->id(), false );
}


void uiStratSynthDisp::displayPreStackSynthetic( const SyntheticData* sd )
{
    const int midx = prestackwin_ ? modelposfld_->getValue() : -1;
    CBCapsule<int> caps( midx, this );
    modSelChanged.trigger( &caps );

    if ( !prestackwin_ ) return;

    uiFlatViewer& vwr = prestackwin_->viewer();
    vwr.clearAllPacks(); vwr.setNoViewDone();
    vwr.control()->zoomMgr().toStart();

    if ( !sd ) return;
    mDynamicCastGet(const PreStack::GatherSetDataPack*,gsetdp,sd->getPack(true))
    if ( !gsetdp ) return;

    PreStack::Gather* gdp = new PreStack::Gather( *gsetdp->getGathers()[midx] );
    DPM(DataPackMgr::FlatID()).add( gdp );

    vwr.appearance() = vwr_->appearance();
    vwr.appearance().annot_.auxdata_.erase();
    vwr.setPack( false, gdp->id(), false, false ); 
    vwr.setPack( true, gdp->id(), false , false); 
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
	fvsu.withwva( true ).withthumbnail( false ).withcoltabed( false )
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

    const int seldataidx = modellist_->box()->currentItem(); 
    topgrp_->setSensitive( seldataidx == 0 );

    currentsynthetic_ = stratsynth_.getSynthetic( seldataidx );

    if ( currentsynthetic_ )
    {
	offsetposfld_->setLimitSampling(currentsynthetic_->raypars_.offsetrg_);
	currentsynthetic_->setPostStack( 0 );
    }
    offsetposfld_->setSensitive( currentsynthetic_ );

    displaySynthetic( currentsynthetic_ );

    if ( stratsynth_.errMsg() )
	mErrRet( stratsynth_.errMsg(), return )
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
    for ( int idx=0; idx<modellist_->box()->size(); idx++ )
	synthnms.add( modellist_->box()->textOfItem( idx ) );

    uiAddNewSynthDlg dlg( this, wvltfld_->getName(), synthnms );
    if ( !dlg.go() )
	return;

    const char* nm = dlg.getSynthName();
    currentsynthetic_->setName( nm );
    stratsynth_.addSynthetics( currentsynthetic_ );
    
    modellist_->box()->addItem( nm );
    modellist_->setSensitive( stratsynth_.synthetics().size() > 1 );
}


void uiStratSynthDisp::rayTrcParPush( CallBacker* )
{
    if ( !raytrcpardlg_ )
	raytrcpardlg_ = new uiRayTrcParamsDlg( this, stratsynth_.rayPars() );

    raytrcpardlg_->go();
    raytrcpardlg_->button( uiDialog::OK )->activated.notify(
			mCB(this,uiStratSynthDisp,rayTrcParChged) );
}



void uiStratSynthDisp::offsetPosChged( CallBacker* )
{
    if ( currentsynthetic_ )
    {
	const RayParams& rp = currentsynthetic_->raypars_;
	const int offidx = rp.offsetrg_.getIndex( offsetposfld_->getValue() );
	currentsynthetic_->setPostStack( offidx ); 
    }
    displayPostStackSynthetic( currentsynthetic_ );
}


void uiStratSynthDisp::modelPosChged( CallBacker* )
{
    displayPreStackSynthetic( currentsynthetic_ );
}


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


const MultiID& uiStratSynthDisp::waveletID() const
{
    return wvltfld_->getID();
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

    prevbut_ = new uiToolButton( this, "prevpos.png", "Previous position",
				mCB(this,uiSynthSlicePos,prevCB) );
    prevbut_->attach( rightOf, slicestepbox_ );
    nextbut_ = new uiToolButton( this, "nextpos.png", "Next position",
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



uiRayTrcParamsDlg::uiRayTrcParamsDlg( uiParent* p, RayParams& rp ) 
    : uiDialog(p,uiDialog::Setup(
		"Specify ray tracer parameters","",mTODOHelpID).modal(false))
    , raypars_(rp)
{
    setCtrlStyle( DoAndStay );

    nmobox_ = new uiCheckBox( this, "NMO corrections" );
    nmobox_->setChecked( raypars_.usenmotimes_ );
    nmobox_->attach( hCentered );

    uiRayTracer1D::Setup rsu( &raypars_.setup_ );
    rsu.dooffsets_ = true;

    raytrace1dgrp_ = uiVrmsRayTracer1D::create( this, rsu );
    raytrace1dgrp_->attach( alignedBelow, nmobox_ );
}


bool uiRayTrcParamsDlg::acceptOK( CallBacker* )
{

    raypars_.usenmotimes_ = nmobox_->isChecked();

    return false;
}


