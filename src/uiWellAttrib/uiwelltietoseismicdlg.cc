/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisaveimagedlg.h"
#include "uiwelltietoseismicdlg.h"
#include "uiwelltiecontrolview.h"
#include "uiwelltieeventstretch.h"
#include "uiwelltieview.h"
#include "uiwelltiewavelet.h"
#include "uiwelltiesavedatadlg.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseiswvltsel.h"
#include "uiseparator.h"
#include "uistatusbar.h"
#include "uistrings.h"
#include "uitoolbar.h"
#include "uiwelldlgs.h"

#include "seiscommon.h"
#include "seistrc.h"
#include "stratsynthgenparams.h"
#include "unitofmeasure.h"
#include "wavelet.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "welltrack.h"
#include "wellextractdata.h"
#include "wellman.h"
#include "wellmarker.h"
#include "welltiedata.h"
#include "welltiepickset.h"
#include "welltiesetup.h"
#include "od_helpids.h"


#define mErrRetYN(msg) { uiMSG().error(msg); return false; }
#define mErrRet(msg) { uiMSG().error(msg); return; }
#define mGetWD(act) const Well::Data* wd = server_.wd(); if ( !wd ) act;

const WellTie::Setup& WellTie::uiTieWin::welltieSetup() const
{
    return server_.data().setup();
}


WellTie::uiTieWin::uiTieWin( uiParent* p, Server& wts )
    : uiFlatViewMainWin(p,
			uiFlatViewMainWin::Setup(uiString::emptyString())
			.deleteonclose(false))
    , server_(wts)
    , stretcher_(*new EventStretch(server_.pickMgr(),server_.d2TModelMgr()))
    , params_(server_.dispParams())
{
    drawer_ = new uiTieView( this, &viewer(), server_.data() );
    mAttachCB( drawer_->infoMsgChanged, uiTieWin::dispInfoMsg );
    mAttachCB( server_.pickMgr().pickadded, uiTieWin::checkIfPick );

    mGetWD(return)
    uiString title = tr("Tie %1 to %2").arg(toUiString(wd->name()))
				       .arg(toUiString(welltieSetup().seisnm_));
    setCaption( title );

    initAll();
}


WellTie::uiTieWin::~uiTieWin()
{
    detachAllNotifiers();
    cleanUp(nullptr);
    delete &stretcher_;
    delete infodlg_;
    delete drawer_;
    delete &server_;
}


void WellTie::uiTieWin::initAll()
{
    drawFields();
    addControls();
    doWork( nullptr );
    show();
}


void WellTie::uiTieWin::fillPar( IOPar& par ) const
{
    server_.dispParams().fillPar( par );
    controlview_->fillPar( par );
    if ( infodlg_ )
	infodlg_->fillPar( par );
}


void WellTie::uiTieWin::usePar( const IOPar& par )
{
    server_.dispParams().usePar( par );
    controlview_->usePar( par );
    if ( infodlg_ )
	infodlg_->usePar( par );
    par_ = par;
}


void WellTie::uiTieWin::displayUserMsg( CallBacker* )
{
    uiString msg = tr("To correlate synthetic to seismic, "
		      "choose your tracking mode, "
		      "pick one or more synthetic events "
		      "and link them with the seismic events. "
		      "Each synthetic event must be coupled with a seismic "
		      "event. Once you are satisfied with the picking, push "
		      "the 'Apply Changes' button to compute a new depth/time "
		      "model and to re-extract the data.\n"
		      "Repeat the picking operation if needed.\n"
		      "To cross-check your operation, press the 'Quality "
		      "Control' button.\n\n"
		      "Press 'OK/Save' to store your new depth/time model.");

    uiMSG().message( msg );
}


void WellTie::uiTieWin::doWork( CallBacker* )
{
    drawer_->enableCtrlNotifiers( false );
    const Wavelet& wvlt = infodlg_ ? infodlg_->getWavelet()
				   : server_.data().initwvlt_;
    if ( !server_.computeSynthetics(wvlt) )
	{ uiMSG().error( server_.errMsg() ); }

    if ( server_.doSeismic() ) //needs to be redone also when new d2t
	if ( !server_.extractSeismics() )
	    { uiMSG().error( server_.errMsg() ); }

    if ( server_.warnMsg().isSet() )
	uiMSG().warning( server_.warnMsg() );

    reDrawAll( nullptr );
    drawer_->enableCtrlNotifiers( true );
}


void WellTie::uiTieWin::reDrawSeisViewer( CallBacker* )
{
    drawer_->redrawViewer();
}


void WellTie::uiTieWin::reDrawAuxDatas( CallBacker* )
{
    drawer_->redrawLogsAuxDatas();
    drawer_->redrawViewerAuxDatas();
}


void WellTie::uiTieWin::reDrawAll( CallBacker* )
{
    drawer_->fullRedraw();
    if ( infodlg_ )
	infodlg_->drawData();
}


#define mAddButton(pm,func,tip) \
    toolbar_->addButton( pm, tip, mCB(this,uiTieWin,func) )
void WellTie::uiTieWin::addToolBarTools()
{
    toolbar_ = new uiToolBar( this, tr("Well Tie Control"), uiToolBar::Right );
    mAddButton( "z2t", editD2TPushed, tr("View/Edit Model") );
    mAddButton( "save", saveDataPushed, tr("Save Data") );
    mAddButton( "snapshot", snapshotCB, uiStrings::sTakeSnapshot() );
}


void WellTie::uiTieWin::addControls()
{
    addToolBarTools();
    controlview_ = new WellTie::uiControlView(this,toolbar_,&viewer(),server_);
    controlview_->redrawNeeded.notify( mCB(this,uiTieWin,reDrawAll) );
    controlview_->redrawAnnotNeeded.notify( mCB(this,uiTieWin,reDrawAuxDatas) );
}


void WellTie::uiTieWin::snapshotCB( CallBacker* )
{
    uiSaveWinImageDlg snapshotdlg( this );
    snapshotdlg.go();
}


void WellTie::uiTieWin::drawFields()
{
    uiSeparator* sep1 = new uiSeparator( this );
    sep1->attach( stretchedBelow, viewer() );
    sep1->attach( ensureBelow, drawer_->displayGroup() );

    uiGroup* vwrtaskgrp = new uiGroup( this, "task group" );
    vwrtaskgrp->attach( ensureBelow, sep1 );
    vwrtaskgrp->attach( alignedBelow, viewer() );
    vwrtaskgrp->attach( rightBorder );
    createViewerTaskFields( vwrtaskgrp );

    polarityfld_ = new uiGenInput( this, uiStrings::sPolarity(),
				   BoolInpSpec(true, Seis::sSEGPositive(),
						     Seis::sSEGNegative()) );
    polarityfld_->attach( leftOf, vwrtaskgrp );
    polarityfld_->attach( ensureBelow, sep1 );
    polarityfld_->valueChanged.notify( mCB(this, uiTieWin, polarityChanged) );

    wvltfld_ = new uiSeisWaveletSel( this, "Wavelet", false, false );
    wvltfld_->setInput( server_.data().setup().sgp_.getWaveletID() );
    wvltfld_->newSelection.notify( mCB(this,uiTieWin,wvltSelCB) );
    wvltfld_->attach( leftOf, polarityfld_ );
    wvltfld_->attach( ensureBelow, sep1 );

    auto* sep2 = new uiSeparator( this );
    sep2->attach( stretchedBelow, wvltfld_ );
    sep2->attach( ensureBelow, vwrtaskgrp );

    auto* okbut = new uiPushButton( this, tr("OK/Save"),
				    mCB(this,uiTieWin,okPushCB), true );
    okbut->attach( leftBorder, 80 );
    okbut->attach( ensureBelow, sep2 );

    uiPushButton* infobut = new uiPushButton( this, uiStrings::sInfo(),
			mCB(this,uiTieWin,displayUserMsg), false );
    infobut->attach( hCentered );
    infobut->attach( ensureBelow, sep2 );
    uiPushButton* helpbut = new uiPushButton( this, uiStrings::sHelp(),
			mCB(this,uiTieWin,provideWinHelp), true );
    helpbut->attach( rightOf, infobut );
    helpbut->attach( ensureBelow, sep2 );
    uiPushButton* cancelbut = new uiPushButton( this, uiStrings::sCancel(),
			mCB(this,uiTieWin,cancelPushCB), true );
    cancelbut->attach( rightBorder );
    cancelbut->attach( ensureBelow, sep2 );
}


void WellTie::uiTieWin::provideWinHelp( CallBacker* )
{
    HelpProvider::provideHelp( HelpKey(mODHelpKey(mWellTieTieWinHelpID) ) );
}


void WellTie::uiTieWin::createViewerTaskFields( uiGroup* taskgrp )
{
    eventtypefld_ = new uiLabeledComboBox( taskgrp, uiStrings::sTrack() );
    BufferStringSet eventtypes;
    server_.pickMgr().getEventTypes( eventtypes );
    for ( int idx=0; idx<eventtypes.size(); idx++)
	eventtypefld_->box()->addItem( toUiString(eventtypes[idx]->buf()) );

    eventtypefld_->box()->setCurrentItem( server_.pickMgr().getEventType() );
    eventtypefld_->box()->selectionChanged.notify(
				mCB(this,uiTieWin,eventTypeChg) );

    IntInpSpec iis( 5 );
    iis.setLimits( StepInterval<int>(1,99,2) );
    nrtrcsfld_ = new uiGenInput( taskgrp, tr("Nr Traces"), iis );
    nrtrcsfld_->valueChanging.notify( mCB(this,uiTieWin,nrtrcsCB) );
    nrtrcsfld_->attach( alignedBelow, eventtypefld_ );

    applybut_ = new uiPushButton( taskgrp, tr("Apply Changes"),
	   mCB(this,uiTieWin,applyPushed), true );
    applybut_->setSensitive( false );
    applybut_->attach( rightOf, eventtypefld_ );

    undobut_ = new uiPushButton( taskgrp, uiStrings::sUndo(),
				   mCB(this,uiTieWin,undoPushed), true );
    undobut_->attach( rightOf, applybut_ );
    undobut_->setSensitive( false );

    clearpicksbut_ = new uiPushButton( taskgrp, tr("Clear picks"),
	   mCB(this,uiTieWin,clearPicks), true );
    clearpicksbut_->setSensitive( false );
    clearpicksbut_->attach( rightOf, undobut_ );

    clearlastpicksbut_ = new uiPushButton( taskgrp, tr("Clear last pick"),
	   mCB(this,uiTieWin,clearLastPick), true );
    clearlastpicksbut_->setSensitive( false );
    clearlastpicksbut_->attach( rightOf, clearpicksbut_ );

    matchhormrksbut_ = new uiPushButton( taskgrp,
					 tr("Match markers and horizons"),
		       mCB(this,uiTieWin,matchHorMrks), true );
    matchhormrksbut_->attach( rightBorder );

    infobut_ = new uiPushButton( taskgrp, tr("Quality Control"),
	               mCB(this,uiTieWin,infoPushed), false );
    infobut_->attach( ensureBelow, applybut_ );
    infobut_->attach( leftOf, matchhormrksbut_ );
}


void WellTie::uiTieWin::infoPushed( CallBacker* )
{
    if ( !infodlg_ )
    {
	if ( !server_.hasSynthetic() || !server_.hasSeismic() )
	{
	    uiString errmsg = tr("No %1 extracted.\nCannot go further")
			    .arg(server_.hasSeismic() ? tr("synthetic data")
						      : tr("seismic data"));
	    uiMSG().error( errmsg );
	    return;
	}

	infodlg_ = new uiInfoDlg( this, server_ );
	infodlg_->redrawNeeded.notify( mCB(this,uiTieWin,reDrawSeisViewer) );
	infodlg_->usePar( par_ );
    }

    infodlg_->show();
}


void WellTie::uiTieWin::editD2TPushed( CallBacker* cb )
{
    Well::Data* wd = server_.wd();
    if ( !wd || !wd->haveD2TModel() ) return;

    uiD2TModelDlg d2tmdlg( this, *wd, false );
    if ( d2tmdlg.go() )
    {
	server_.updateExtractionRange();
	doWork( cb );
    }
}


void WellTie::uiTieWin::saveDataPushed( CallBacker* )
{
    uiSaveDataDlg dlg( this, server_ );
    dlg.go();
    wvltfld_->rebuildList();
}


void WellTie::uiTieWin::eventTypeChg( CallBacker* )
{
    server_.pickMgr().setEventType( eventtypefld_->box()->text() );
    controlview_->setEditMode( true );
}


void WellTie::uiTieWin::applyPushed( CallBacker* cb )
{
    mGetWD(return);
    stretcher_.setD2TModel( wd->d2TModel() );
    stretcher_.setTrack( &wd->track() );
    stretcher_.doWork( cb );
    server_.updateExtractionRange();
    if ( infodlg_ )
	infodlg_->dtmodelChanged( nullptr );

    doWork( cb );
    clearPicks( cb );

    applybut_->setSensitive( false );
    undobut_->setSensitive( true );
}


void WellTie::uiTieWin::clearPicks( CallBacker* cb )
{
    server_.pickMgr().clearAllPicks();
    checkIfPick( cb );
    drawer_->redrawViewerAuxDatas();
}


void WellTie::uiTieWin::clearLastPick( CallBacker* cb )
{
    server_.pickMgr().clearLastPicks();
    checkIfPick( cb );
    drawer_->redrawViewerAuxDatas();
}


void WellTie::uiTieWin::checkIfPick( CallBacker* )
{
    const bool ispick = server_.pickMgr().isPick();
    const bool issamesz = server_.pickMgr().isSynthSeisSameSize();
    clearpicksbut_->setSensitive( ispick );
    clearlastpicksbut_->setSensitive( ispick );
    applybut_->setSensitive( ispick && issamesz );
}


void WellTie::uiTieWin::undoPushed( CallBacker* cb )
{
    if ( !server_.d2TModelMgr().undo() )
	mErrRet( tr("Cannot go back to previous model") );

    server_.updateExtractionRange();
    doWork( cb );
    clearPicks( cb );

    if ( infodlg_ )
	infodlg_->dtmodelChanged( nullptr );

    undobut_->setSensitive( false );
    applybut_->setSensitive( false );
}


void WellTie::uiTieWin::matchHorMrks( CallBacker* )
{
    PickSetMgr& pmgr = server_.pickMgr();
    mGetWD(return)
    if ( !wd || !wd->markers().size() )
	mErrRet( tr("No Well marker found") )

    uiString msg = tr("No horizon loaded, do you want to load some ?");
    const Data& data = server_.data();
    if ( !data.horizons_.size() )
    {
	if ( !uiMSG().askGoOn( msg ) )
	    return;
	controlview_->loadHorizons( nullptr );
    }
    pmgr.clearAllPicks();
    uiDialog matchdlg( this, uiDialog::Setup(uiStrings::sSettings(),mNoDlgTitle,
                                             mNoHelpKey) );
    uiGenInput* matchinpfld = new uiGenInput( &matchdlg, tr("Match same"),
				BoolInpSpec(true,uiStrings::sName(),
                                            tr("Regional marker")) );
    matchdlg.go();
    TypeSet<HorizonMgr::PosCouple> pcs;
    server_.horizonMgr().matchHorWithMarkers( pcs, matchinpfld->getBoolValue());
    if ( pcs.isEmpty() )
	mErrRet( tr("No match between markers and horizons") )
    for ( int idx=0; idx<pcs.size(); idx ++ )
    {
	pmgr.addPick( pcs[idx].z1_, true );
	pmgr.addPick( pcs[idx].z2_, false );
    }
    drawer_->drawUserPicks();
}


void WellTie::uiTieWin::nrtrcsCB( CallBacker* )
{
    const int nrtrcs = nrtrcsfld_->getIntValue();
    drawer_->setNrTrcs( nrtrcs );
}


void WellTie::uiTieWin::wvltSelCB( CallBacker* )
{
    const PtrMan<Wavelet> selwvlt = wvltfld_->getWavelet();
    if ( !selwvlt ) return;

    if ( !server_.setNewWavelet(wvltfld_->getID()) )
	mErrRet( server_.errMsg() )

    drawer_->redrawViewer();

    if ( infodlg_ )
	infodlg_->updateInitialWavelet();
}


void WellTie::uiTieWin::cancelPushCB( CallBacker* )
{
    drawer_->enableCtrlNotifiers( false );
    close();
}


void WellTie::uiTieWin::cleanUp( CallBacker* )
{
    server_.d2TModelMgr().cancel();
    if ( Well::MGR().isLoaded( server_.wellID() ) )
	Well::MGR().reload( server_.wellID() );
    return;
}


void WellTie::uiTieWin::okPushCB( CallBacker* )
{
    uiString errmsg = tr("This will overwrite your depth/time model, "
			 "do you want to continue?");
    if ( uiMSG().askOverwrite(errmsg) )
    {
	drawer_->enableCtrlNotifiers( false );
	close();
	if ( !server_.d2TModelMgr().commitToWD() )
	    mErrRet(tr("Cannot write new depth/time model"))

	if ( Well::MGR().isLoaded( server_.wellID() ) )
	    Well::MGR().reload( server_.wellID() );
    }
}


void WellTie::uiTieWin::dispInfoMsg( CallBacker* cb )
{
    mCBCapsuleUnpack(BufferString,mesg,cb);
    statusBar()->message( mToUiStringTodo(mesg.buf()) );
}


void WellTie::uiTieWin::polarityChanged( CallBacker* )
{
    const bool isSEGPositive = polarityfld_->getBoolValue();
    drawer_->setSEGPositivePolarity( isSEGPositive );
}


static const char* sKeyInfoIsInitWvltActive = "Is init wavelet active";
static const char* sKeyInfoSelBoxIndex = "Selected index";
static const char* sKeyInfoSelZrange = "Selected Z Range";
static const char* sKeyStartMrkrName = "Start Marker Name";
static const char* sKeyStopMrkrName = "Stop Marker Name";


#define	mMarkerFldIdx	0
#define	mTwtFldIdx  1
#define	mDahFldIdx  2
#define mMinWvltLength	20

WellTie::uiInfoDlg::uiInfoDlg( uiParent* p, Server& server )
	: uiDialog(p,uiDialog::Setup(tr("Cross-checking parameters"),
				     uiString::emptyString(),
				     mODHelpKey(mWellTieInfoDlgHelpID) )
				.modal(false))
    , server_(server)
    , redrawNeeded(this)
    , data_(server_.data())
{
    setCtrlStyle( CloseOnly );

    auto* viewersgrp = new uiGroup( this, "Viewers group" );
    auto* wvltgrp = new uiGroup( viewersgrp, "wavelet group" );
    auto* corrgrp = new uiGroup( viewersgrp, "CrossCorrelation group" );

    ObjectSet<Wavelet> wvlts;
    wvlts += &data_.initwvlt_;
    wvlts += &data_.estimatedwvlt_;
    wvltdraw_ = new WellTie::uiWaveletView( wvltgrp, wvlts );
    mAttachCB( wvltdraw_->activeWvltChgd, uiInfoDlg::wvltChanged );
    wvltdraw_->setActiveWavelet( true );

    wvltscaler_ = new uiLabel( wvltgrp, uiStrings::sEmptyString() );
    wvltscaler_->attach( leftAlignedBelow, wvltdraw_ );
    const int initwvltsz = data_.initwvlt_.size() - 1;
    const int maxwvltsz = mNINT32( server_.data().getTraceRange().width() *
				   SI().zDomain().userFactor() );
    estwvltlengthfld_ = new uiGenInput(wvltgrp,
				       tr("Deterministic wavelet length (ms)"),
	IntInpSpec(initwvltsz,mMinWvltLength,maxwvltsz) );
    estwvltlengthfld_->attach( leftAlignedBelow, wvltscaler_ );
    estwvltlengthfld_->setElemSzPol( uiObject::Small );
    mAttachCB( estwvltlengthfld_->valueChanged,uiInfoDlg::needNewEstimatedWvlt);

    uiSeparator* verSepar = new uiSeparator( viewersgrp, "Vert sep",
					     OD::Vertical );
    verSepar->attach( rightTo, wvltgrp );

    corrgrp->attach( rightOf, verSepar );
    crosscorr_ = new uiCrossCorrView( corrgrp, data_ );

    uiSeparator* horSepar = new uiSeparator( this );
    horSepar->attach( stretchedAbove, viewersgrp );

    uiGroup* markergrp =  new uiGroup( this, "User Z Range Group" );
    markergrp->attach( centeredAbove, viewersgrp );
    horSepar->attach( ensureBelow, markergrp );

    const char* choice[] = { "Markers", "Times", "Depths (MD)", nullptr };
    choicefld_ = new uiGenInput( markergrp, tr("Compute Data between"),
					StringListInpSpec(choice) );
    mAttachCB( choicefld_->valueChanged, uiInfoDlg::zrgChanged );

    markernames_.add( Well::ExtractParams::sKeyDataStart() );
    mGetWD(return)
    if ( wd && wd->haveMarkers() )
    {
	for ( int idx=0; idx<wd->markers().size(); idx++)
	    markernames_.add( wd->markers()[idx]->name() );
    }

    markernames_.add( Well::ExtractParams::sKeyDataEnd() );
    StringListInpSpec slis( markernames_ );
    const char* markernms[] = { "Top Marker", "Bottom Marker", 0 };

    zrginft_ = SI().depthsInFeet();
    const uiString units[] = { uiString::emptyString(),
		UnitOfMeasure::zUnitAnnot(true,true,false),
		UnitOfMeasure::surveyDefDepthUnitAnnot(false,false) };

    zrangeflds_ += new uiGenInput( markergrp, uiString::emptyString(),
				   slis.setName(markernms[0]),
				   slis.setName(markernms[1]) );
    zrangeflds_[mMarkerFldIdx]->setValue( markernames_.size()-1, 1 );

    const int maxtwtval = mNINT32( server_.data().getTraceRange().stop *
				   SI().zDomain().userFactor() );
    zrangeflds_ += new uiGenInput( markergrp, uiString::emptyString(),
	    IntInpIntervalSpec().setLimits(StepInterval<int>(0,maxtwtval,1)));

    const float maxdah = wd->track().dahRange().stop;
    zrangeflds_ += new uiGenInput( markergrp, uiString::emptyString(),
	    FloatInpIntervalSpec().setLimits(Interval<float>(0,maxdah)));
    zrangeflds_[mDahFldIdx]->setNrDecimals(2,0);
    zrangeflds_[mDahFldIdx]->setNrDecimals(2,1);

    for ( int idx=0; choice[idx]; idx++ )
    {
	mAttachCB( zrangeflds_[idx]->valueChanged, uiInfoDlg::zrgChanged );
	zrangeflds_[idx]->attach( rightOf, choicefld_ );
	zlabelflds_ += new uiLabel( markergrp, units[idx] );
	zlabelflds_[idx]->attach( rightOf, zrangeflds_[idx] );
    }
    setPrefWidth( 400 );
    setPrefHeight( 200 );
}


WellTie::uiInfoDlg::~uiInfoDlg()
{
    detachAllNotifiers();
    delete crosscorr_;
    delete wvltdraw_;
}


void WellTie::uiInfoDlg::updateInitialWavelet()
{
    computeNewWavelet();
    server_.computeCrossCorrelation();
    drawData();
}


void WellTie::uiInfoDlg::fillPar( IOPar& par ) const
{
    par.setYN( sKeyInfoIsInitWvltActive, isInitWvltActive() );
    par.set( sKeyInfoSelBoxIndex, selidx_ );
    if ( selidx_ == mMarkerFldIdx )
    {
	par.set( sKeyStartMrkrName, zrangeflds_[mMarkerFldIdx]->text(0) );
	par.set( sKeyStopMrkrName, zrangeflds_[mMarkerFldIdx]->text(1) );
    }
    else
    {
	const uiGenInput* selectedChoice = zrangeflds_[selidx_];
	if ( selectedChoice )
	{
	    Interval<float> range = selectedChoice->getFInterval();
	    const float scalefact = selidx_ == mTwtFldIdx
				  ? 1.f / SI().zDomain().userFactor()
				  : zrginft_ ? mFromFeetFactorF : 1.f;
	    range.scale( scalefact );
	    par.set( sKeyInfoSelZrange, range );
	}
    }
}


void WellTie::uiInfoDlg::usePar( const IOPar& par )
{
    bool isinitwvltactive;
    par.getYN( sKeyInfoIsInitWvltActive, isinitwvltactive );
    par.get( sKeyInfoSelBoxIndex, selidx_ );
    if ( selidx_ == mMarkerFldIdx )
    {
	par.get( sKeyStartMrkrName, startmrknm_ );
	par.get( sKeyStopMrkrName, stopmrknm_ );
    }
    else
	par.get( sKeyInfoSelZrange, zrg_ );

    if ( wvltdraw_ && isinitwvltactive != isInitWvltActive() )
	wvltdraw_->setActiveWavelet( isinitwvltactive );

    putToScreen();
    zrgChanged( nullptr );
}


void WellTie::uiInfoDlg::putToScreen()
{
    if ( !choicefld_ )
	return;

    choicefld_->setValue( selidx_ );
    if ( !zrangeflds_[selidx_] )
	return;

    const int lastmarkeridx = markernames_.size()-1;
    if ( selidx_ == mMarkerFldIdx )
    {
	mGetWD(return)
	if ( !wd || !wd->markers().size() )
	    mErrRet( tr("No Well marker found") )

	const Well::Marker* topmarkr = wd->markers().getByName( startmrknm_ );
	const Well::Marker* basemarkr = wd->markers().getByName( stopmrknm_ );
	if ( !topmarkr || !basemarkr )
	{
	    if ( !topmarkr )
	    {
		if ( !startmrknm_.startsWith(
				  Well::ZRangeSelector::sKeyDataStart()) )
		    uiMSG().warning(
			   tr("Top marker from setup could not be retrieved."));

		zrangeflds_[selidx_]->setValue( 0, 0 );
	    }

	    if ( !basemarkr )
	    {
		if ( !stopmrknm_.startsWith(
				  Well::ZRangeSelector::sKeyDataEnd()) )
		    uiMSG().warning(
			  tr("Base marker from setup could not be retrieved."));

		zrangeflds_[selidx_]->setValue( lastmarkeridx, 1 );
	    }
	}
	else if( topmarkr->dah() >= basemarkr->dah() )
	{
	    uiMSG().warning(
		    tr("Inconsistency between top/base marker from setup."));
	    zrangeflds_[selidx_]->setValue( 0, 0 );
	    zrangeflds_[selidx_]->setValue( lastmarkeridx, 1 );
	}
	else
	{
	    zrangeflds_[selidx_]->setText( startmrknm_.buf(), 0 );
	    zrangeflds_[selidx_]->setText( stopmrknm_.buf(), 1 );
	}
    }
    else
    {
	Interval<float> zrg = zrg_;
	const float scalefact = selidx_ == mTwtFldIdx
			      ? SI().zDomain().userFactor()
			      : zrginft_ ? mToFeetFactorF : 1.f;
	zrg.scale( scalefact );
	if ( zrg.width(false) < (float)mMinWvltLength )
	{
	    uiMSG().warning(
		    tr("Invalid correlation gate from setup. Resetting."));
	    selidx_ = mMarkerFldIdx;
	    choicefld_->setValue( selidx_ );
	    zrangeflds_[selidx_]->setValue( 0, 0 );
	    zrangeflds_[selidx_]->setValue( lastmarkeridx, 1 );
	    return;
	}

	zrangeflds_[selidx_]->setValue( zrg );
    }
}


void WellTie::uiInfoDlg::dtmodelChanged( CallBacker* )
{
    needNewEstimatedWvlt( nullptr );
    if ( !isInitWvltActive() )
	if ( !server_.updateSynthetics(getWavelet()) )
	    mErrRet( server_.errMsg() )

    synthChanged( nullptr );
}


void WellTie::uiInfoDlg::wvltChanged( CallBacker* )
{
    if ( wvltdraw_ )
	wvltdraw_->redrawWavelets();

    if( !server_.updateSynthetics(getWavelet()) )
	mErrRet( server_.errMsg() )

    synthChanged( nullptr );
}


void WellTie::uiInfoDlg::needNewEstimatedWvlt( CallBacker* )
{
    if ( !computeNewWavelet() )
	return;

    wvltChanged( nullptr );
}


void WellTie::uiInfoDlg::synthChanged( CallBacker* )
{
    redrawNeeded.trigger();
    if ( !server_.computeCrossCorrelation() )
	mErrRet( server_.errMsg() )

    crossCorrelationChanged( nullptr );
}


void WellTie::uiInfoDlg::zrgChanged( CallBacker* )
{
    if ( !updateZrg() )
	return;

    zrg_.limitTo( data_.getTraceRange() );
    if ( zrg_.isRev() )
	mErrRet( tr("Top marker must be above base marker.") )
    server_.setCrossCorrZrg( zrg_ );
    needNewEstimatedWvlt( nullptr );

    if ( !server_.computeCrossCorrelation() )
	mErrRet( server_.errMsg() )

    crossCorrelationChanged( nullptr );
}


void WellTie::uiInfoDlg::crossCorrelationChanged( CallBacker* )
{
    if ( crosscorr_ )
    {
	crosscorr_->set( data_.correl_ );
	crosscorr_->draw();
    }

    const float scaler = data_.correl_.scaler_;
    if ( wvltscaler_ && !mIsUdf(scaler) )
    {
	if ( !mIsUdf(scaler) )
	{
	    uiString scalerfld = tr("Synthetic to seismic scaler: %1")
			       .arg(scaler);
	    wvltscaler_->setText( scalerfld );
	}
	else
	    wvltscaler_->clear();
    }
}


#define md2TI( inzrg, ouzrg, outistime )\
    { ouzrg.start = md2T( inzrg.start, outistime ); \
      ouzrg.stop = md2T( inzrg.stop, outistime ) }
#define md2T( zval, outistime )\
    outistime ? d2t->getTime( zval, wd->track() ) \
	      : d2t->getDah( zval, wd->track() );


bool WellTie::uiInfoDlg::updateZrg()
{
    if ( zrangeflds_.isEmpty() || !choicefld_ )
	return false;

    NotifyStopper ns0 = NotifyStopper( zrangeflds_[0]->valueChanged );
    NotifyStopper ns1 = NotifyStopper( zrangeflds_[1]->valueChanged );
    NotifyStopper ns2 = NotifyStopper( zrangeflds_[2]->valueChanged );

    mGetWD(return false)
    const Well::D2TModel* d2t = wd->d2TModel();
    selidx_ = choicefld_->getIntValue();
    Interval<float> dahrg( mUdf(float), mUdf(float) );
    Interval<float> timerg( mUdf(float), mUdf(float) );
    const uiGenInput* selectedchoice = zrangeflds_[selidx_];
    if ( !selectedchoice )
	return false;

    if ( selidx_ == mTwtFldIdx )
    {
	timerg = selectedchoice->getFInterval();
	timerg.scale( 1.f / SI().zDomain().userFactor() );
	md2TI( timerg, dahrg, false )
    }
    else
    {
	if ( selidx_ == mMarkerFldIdx )
	{
	    if( !getMarkerDepths(dahrg) )
		return false;
	}
	else if ( selidx_ == mDahFldIdx )
	{
	    dahrg = selectedchoice->getFInterval();
	    if ( dahrg.isRev() )
		mErrRetYN( tr("Top marker must be above base marker.") )

	    if ( zrginft_ )
		dahrg.scale( mFromFeetFactorF );
	}

	if ( !wd->track().dahRange().includes(dahrg) )
	    mErrRetYN( tr("Selected interval is larger than the track."))

	md2TI( dahrg, timerg, true )
    }

    if ( timerg.isUdf() || dahrg.isUdf() )
	mErrRetYN( tr("Selected interval is incorrect.") )

    const StepInterval<float> reflrg = data_.getReflRange();
    timerg.start = mMAX( timerg.start, reflrg.start );
    timerg.stop = mMIN( timerg.stop, reflrg.stop );
    const float reflstep = reflrg.step;
    timerg.start = (float) mNINT32( timerg.start / reflstep ) * reflstep;
    timerg.stop = (float) mNINT32( timerg.stop / reflstep ) * reflstep;
    if ( timerg.width() < (float)mMinWvltLength / SI().zDomain().userFactor() )
    {
	uiString errmsg = tr("The selected interval length must be "
			     "at least %1ms").arg(mMinWvltLength);
	mErrRetYN( errmsg )
    }

    zrg_ = timerg;
    for ( int idx=0; idx<zrangeflds_.size(); idx++ )
    {
	if ( zrangeflds_[idx] )
	    zrangeflds_[idx]->display( idx == selidx_ );

	if ( zlabelflds_[idx] )
	    zlabelflds_[idx]->display( idx == selidx_ );
    }

    if ( zrangeflds_[mTwtFldIdx] )
    {
	const float zfact = mCast( float, SI().zDomain().userFactor() );
	Interval<int> timergms( mCast( int, timerg.start * zfact ),
				mCast( int, timerg.stop * zfact ) );
	zrangeflds_[mTwtFldIdx]->setValue( timergms );
    }

    if( zrangeflds_[mDahFldIdx] )
    {
	if ( zrginft_ )
	    dahrg.scale( mToFeetFactorF );

	zrangeflds_[mDahFldIdx]->setValue( dahrg );
    }

    return true;
}


bool WellTie::uiInfoDlg::getMarkerDepths( Interval<float>& zrg )
{
    mGetWD(return false)
    const Interval<int> mintv = zrangeflds_[mMarkerFldIdx]->getIInterval();
    if ( mintv.start >= mintv.stop )
	mErrRetYN( tr("Top marker must be above base marker.") )

    const Well::Marker* topmarkr =
		wd->markers().getByName( zrangeflds_[mMarkerFldIdx]->text(0) );
    if ( !topmarkr && mintv.start )
    {
	zrangeflds_[mMarkerFldIdx]->setValue(0,0);
	uiMSG().warning( tr("Cannot not find the top marker in the well.") );
    }

    const Well::Marker* botmarkr =
		wd->markers().getByName( zrangeflds_[mMarkerFldIdx]->text(1) );
    const int lastmarkeridx = markernames_.size()-1;
    if ( !botmarkr && mintv.stop != lastmarkeridx )
    {
	zrangeflds_[mMarkerFldIdx]->setValue( lastmarkeridx, 1 );
	uiMSG().warning( tr("Cannot not find the base marker in the well.") );
    }

    zrg.start = topmarkr ? topmarkr->dah() : data_.getDahRange().start;
    zrg.stop = botmarkr ? botmarkr->dah() : data_.getDahRange().stop;

    return true;
}


bool WellTie::uiInfoDlg::computeNewWavelet()
{
    if ( !estwvltlengthfld_ )
	return false;

    const int reqwvltlgthsz = estwvltlengthfld_->getIntValue();
    if ( reqwvltlgthsz < mMinWvltLength )
    {
	uiString errmsg = tr("The wavelet length must be at least %1ms")
			.arg(mMinWvltLength);
	mErrRetYN( errmsg )
    }

    const int zrgsz = mCast( int, zrg_.width(false) *
				  SI().zDomain().userFactor() ) + 1;
    const int wvltlgth = zrgsz < reqwvltlgthsz ? zrgsz : reqwvltlgthsz;
    if ( !server_.computeEstimatedWavelet(wvltlgth) )
	mErrRetYN( server_.errMsg() )

    return true;
}


void WellTie::uiInfoDlg::drawData()
{
    crossCorrelationChanged( nullptr );
    if ( wvltdraw_ )
	wvltdraw_->redrawWavelets();
}


const Wavelet& WellTie::uiInfoDlg::getWavelet() const
{
    return isInitWvltActive() ? data_.initwvlt_
			      : data_.estimatedwvlt_;
}


bool WellTie::uiInfoDlg::isInitWvltActive() const
{
    if ( !wvltdraw_ )
	return true;

    return wvltdraw_->isInitialWvltActive();
}
