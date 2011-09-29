/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiwelltietoseismicdlg.cc,v 1.95 2011-09-29 12:29:17 cvsbruno Exp $";

#include "uiwelltietoseismicdlg.h"
#include "uiwelltiecontrolview.h"
#include "uiwelltieeventstretch.h"
#include "uiwelltieview.h"
#include "uiwelltiewavelet.h"
#include "uiwelltiesavedatadlg.h"

#include "uitoolbutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uistatusbar.h"
#include "uitoolbar.h"
#include "uitaskrunner.h"
#include "uiwelldlgs.h"
#include "uiwelllogdisplay.h"

#include "arrayndimpl.h"
#include "arrayndutils.h"
#include "seistrc.h"
#include "wavelet.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "welllog.h"
#include "welllogset.h"
#include "welltrack.h"
#include "wellextractdata.h"
#include "wellmarker.h"
#include "welltiedata.h"
#include "welltiepickset.h"
#include "welltiesetup.h"

namespace WellTie
{

static const char*  errdmsg = "unable to handle data, please check your input ";
static const char*  helpid = "107.4.1";
static const char*  eventtypes[] = { "None","Extrema","Maxima",
				     "Minima","Zero-crossings",0 };

#define mErrRet(msg) { uiMSG().error(msg); return false; }
#define mGetWD(act) const Well::Data* wd = server_.wd(); if ( !wd ) act;

uiTieWin::uiTieWin( uiParent* p, const WellTie::Setup& wts ) 
	: uiFlatViewMainWin(p,uiFlatViewMainWin::Setup("")
			    .withhanddrag(true)
			    .deleteonclose(false))
    	, setup_(*new WellTie::Setup(wts))
	, server_(*new Server(setup_))
	, stretcher_(*new EventStretch(server_.pickMgr(),server_.d2TModelMgr()))
	, controlview_(0)
	, infodlg_(0)
	, params_(server_.dispParams())
{
    drawer_ = new uiTieView( this, &viewer(), server_.data() );
    drawer_->infoMsgChanged.notify( mCB(this,uiTieWin,dispInfoMsg) );
    server_.pickMgr().pickadded.notify( mCB(this,uiTieWin,checkIfPick) );
    server_.setTaskRunner( new uiTaskRunner( p ) );
    
    mGetWD(return) 
    BufferString title( "Tie ");
    title += wd->name(); title += " to "; title += wts.seisnm_;
    setCaption( title );

    initAll();
}


uiTieWin::~uiTieWin()
{
    delete &stretcher_;
    delete infodlg_;
    delete &setup_;
    delete drawer_;
    delete &server_;
}


void uiTieWin::initAll()
{
    drawFields();
    addControls();
    doWork( 0 );
    show();
    dispPropChg( 0 );
}


void uiTieWin::fillPar( IOPar& par ) const
{
    server_.dispParams().fillPar( par );
    controlview_->fillPar( par );
    if ( infodlg_ )
	infodlg_->fillPar( par );
}


void uiTieWin::usePar( const IOPar& par )
{
    server_.dispParams().usePar( par );
    controlview_->usePar( par );
    if ( infodlg_ )
	infodlg_->usePar( par );
    par_ = par;
}


void uiTieWin::displayUserMsg( CallBacker* )
{
    BufferString msg = "To correlate synthetic to seismic, "; 
    msg += "choose your tracking mode, "; 
    msg += "pick one or more synthetic events "; 
    msg += "and link them with the seismic events. "; 
    msg += "Each synthetic event must be coupled with a seismic event. "; 
    msg += "Once you are satisfied with the picking, push the ";
    msg += "'Apply Changes' button to compute a new depth/time model ";
    msg += "and to re-extract the data.\n"; 
    msg += "Repeat the picking operation if needed.\n";
    msg += "To cross-check your operation, press the 'Display additional ";
    msg += "information' button.\n\n";
    msg += "Press 'OK/Save' to store your new depth/time model.";

    uiMSG().message( msg );
}


void uiTieWin::doWork( CallBacker* cb )
{
    drawer_->enableCtrlNotifiers( false );
    if ( !server_.computeAll() )
	{ uiMSG().error( server_.errMSG() ); return; }
    getDispParams();
    reDrawAll(0);
    drawer_->enableCtrlNotifiers( true );
}


void uiTieWin::reDrawSeisViewer( CallBacker* )
{
    drawer_->redrawViewer();
}


void uiTieWin::reDrawSeisViewerAnnot( CallBacker* )
{
    drawer_->redrawViewerAnnots();
}


void uiTieWin::reDrawAll( CallBacker* )
{
    drawer_->fullRedraw();
    if ( infodlg_ )
	infodlg_->drawData();
}


#define mAddButton(pm,func,tip) \
    toolbar_->addButton( pm, tip, mCB(this,uiTieWin,func) )
void uiTieWin::addToolBarTools()
{
    toolbar_ = new uiToolBar( this, "Well Tie Control", uiToolBar::Right ); 
    mAddButton( "z2t.png", editD2TPushed, "View/Edit Model" );
    mAddButton( "z2t.png", editD2TDriftPushed, "Drift curve" );
    mAddButton( "save.png", saveDataPushed, "Save Data" );
}    


void uiTieWin::addControls()
{
    addToolBarTools();
    controlview_ = new WellTie::uiControlView(this,toolbar_,&viewer(),server_);
    controlview_->redrawNeeded.notify( mCB(this,uiTieWin,reDrawAll) );
    controlview_->redrawAnnotNeeded.notify( 
	    			mCB(this,uiTieWin,reDrawSeisViewerAnnot) );
}


void uiTieWin::drawFields()
{
    uiGroup* vwrtaskgrp = new uiGroup( this, "task group" );
    vwrtaskgrp->attach( alignedBelow, viewer() );
    vwrtaskgrp->attach( rightBorder );
    createViewerTaskFields( vwrtaskgrp );

    uiGroup* disppropgrp = new uiGroup( this, "Display Properties group" );
    disppropgrp->attach( ensureLeftOf, vwrtaskgrp );
    disppropgrp->attach( ensureBelow, viewer() );
    disppropgrp->attach( ensureBelow, drawer_->logDisps()[0] );
    disppropgrp->attach( ensureBelow, drawer_->logDisps()[1] );
    disppropgrp->attach( leftBorder );
    createDispPropFields( disppropgrp );

    uiSeparator* horSepar = new uiSeparator( this );
    horSepar->attach( stretchedBelow, disppropgrp );
    horSepar->attach( ensureBelow, vwrtaskgrp );

    uiPushButton* okbut = new uiPushButton( this, "&Ok/Save",
	      		mCB(this,uiTieWin,acceptOK), true );
    okbut->attach( leftBorder, 80 );
    okbut->attach( ensureBelow, horSepar );

    uiPushButton* infobut = new uiPushButton( this, "Info",
	      		mCB(this,uiTieWin,displayUserMsg), false );
    infobut->attach( hCentered );
    infobut->attach( ensureBelow, horSepar );
    uiToolButton* helpbut = new uiToolButton( this, "contexthelp.png", "Help",
			mCB(this,uiTieWin,provideWinHelp) );
    helpbut->setPrefWidthInChar( 5 );
    helpbut->attach( rightOf, infobut );
    helpbut->attach( ensureBelow, horSepar );
    uiPushButton* cancelbut = new uiPushButton( this, "&Cancel",
	      		mCB(this,uiTieWin,rejectOK), true );
    cancelbut->attach( rightBorder );
    cancelbut->attach( ensureBelow, horSepar );
}


void uiTieWin::provideWinHelp( CallBacker* )
{
    provideHelp( helpid );
}


void uiTieWin::createViewerTaskFields( uiGroup* taskgrp )
{
    eventtypefld_ = new uiLabeledComboBox( taskgrp, "Track" );
    for ( int idx=0; eventtypes[idx]; idx++)
	eventtypefld_->box()->addItem( eventtypes[idx] );
    eventtypefld_->box()->selectionChanged.
	notify(mCB(this,uiTieWin,eventTypeChg));

    applybut_ = new uiPushButton( taskgrp, "&Apply Changes",
	   mCB(this,uiTieWin,applyPushed), true );
    applybut_->setSensitive( false );
    applybut_->attach( rightOf, eventtypefld_ );

    undobut_ = new uiPushButton( taskgrp, "&Undo",
	   mCB(this,uiTieWin,undoPushed), true );
    undobut_->attach( rightOf, applybut_ );
    undobut_->setSensitive( false );
    
    clearpicksbut_ = new uiPushButton( taskgrp, "&Clear picks",
	   mCB(this,uiTieWin,clearPicks), true );
    clearpicksbut_->setSensitive( false );
    clearpicksbut_->attach( rightOf, undobut_ );
    
    clearlastpicksbut_ = new uiPushButton( taskgrp, "&Clear last pick",
	   mCB(this,uiTieWin,clearLastPick), true );
    clearlastpicksbut_->setSensitive( false );
    clearlastpicksbut_->attach( rightOf, clearpicksbut_ );

    infobut_ = new uiPushButton( taskgrp, "Display additional information",
	               mCB(this,uiTieWin,infoPushed), false );
    infobut_->attach( ensureBelow, applybut_ );
    infobut_->attach( hCentered );
    
    matchhormrksbut_ = new uiPushButton( taskgrp,"Match markers and horizons",
	               mCB(this,uiTieWin,matchHorMrks), true );
    matchhormrksbut_->attach( rightOf, infobut_ );
}


void uiTieWin::createDispPropFields( uiGroup* dispgrp )
{
    mGetWD(return);
    dispgrp->setHSpacing( 50 );
    cscorrfld_ = new uiCheckBox( dispgrp, "use checkshot corrections" );
    cscorrfld_->display( wd->haveCheckShotModel() && !setup_.useexistingd2tm_);

    csdispfld_ = new uiCheckBox( dispgrp, "display checkshot related curve" );
    csdispfld_->display( wd->haveCheckShotModel() && !setup_.useexistingd2tm_);

    zinftfld_ = new uiCheckBox( dispgrp, "Z in feet" );
    zinftfld_ ->attach( rightOf, csdispfld_);

    zintimefld_ = new uiCheckBox( dispgrp, "Z in time" );
    zintimefld_ ->attach( alignedAbove, zinftfld_ );
    
    markerfld_ = new uiCheckBox( dispgrp, "Display Markers" );
    markerfld_->attach( rightOf, zintimefld_ );
    markerfld_->display( wd->haveMarkers() );

    putDispParams();

    const CallBack pccb( mCB(this,uiTieWin,dispPropChg) );
    cscorrfld_->activated.notify(mCB(this,uiTieWin,csCorrChanged));
    cscorrfld_->activated.notify( pccb );
    csdispfld_->activated.notify( pccb );
    markerfld_->activated.notify( pccb );
    zinftfld_->activated.notify( pccb );
    zintimefld_->activated.notify( pccb );

    zinftfld_->setChecked( SI().depthsInFeetByDefault() );
}


void uiTieWin::getDispParams()
{
    params_.iscscorr_ = cscorrfld_->isChecked();
    params_.iscsdisp_ = csdispfld_->isChecked();
    params_.iszinft_ = zinftfld_->isChecked();
    params_.iszintime_ = zintimefld_->isChecked();
    params_.ismarkerdisp_ = markerfld_->isChecked();
}


void uiTieWin::putDispParams()
{
    csdispfld_->setChecked( params_.iscsdisp_ );
    cscorrfld_->setChecked( params_.iscscorr_ );
    markerfld_->setChecked( params_.ismarkerdisp_ );
    zinftfld_->setChecked( params_.iszinft_ );
    zintimefld_->setChecked( params_.iszintime_ );
}


void uiTieWin::dispPropChg( CallBacker* )
{
    getDispParams();
    zinftfld_->display( !params_.iszintime_ );
    reDrawAll(0);
}


void uiTieWin::csCorrChanged( CallBacker* cb )
{
    getDispParams();

    server_.computeD2TModel();

    doWork( cb );
}


void uiTieWin::infoPushed( CallBacker* )
{
    if ( !infodlg_ )
    {
	infodlg_ = new uiInfoDlg( this, server_ );
	infodlg_->redrawNeeded.notify( mCB(this,uiTieWin,reDrawSeisViewer) );
	infodlg_->usePar( par_ );
    }
    infodlg_->show();
}


void uiTieWin::editD2TPushed( CallBacker* cb )
{
    Well::Data* wd = server_.wd(); 
    if ( !wd || !wd->haveD2TModel() ) return;
    
    uiD2TModelDlg d2tmdlg( this, *wd, false );
    if ( d2tmdlg.go() )
	doWork( cb );
}


void uiTieWin::editD2TDriftPushed( CallBacker* cb )
{
}



bool uiTieWin::saveDataPushed( CallBacker* cb )
{
    uiSaveDataDlg dlg( this, server_.data(), server_.dataWriter() );
    return dlg.go();
}


void uiTieWin::eventTypeChg( CallBacker* )
{
    server_.pickMgr().setEventType(eventtypefld_->box()->currentItem());
    controlview_->setEditOn( true );
}


void uiTieWin::applyPushed( CallBacker* cb )
{
    mGetWD();
    stretcher_.setD2TModel( wd->d2TModel() );
    stretcher_.doWork( cb );
    doWork( cb );
    clearPicks( cb );
    if ( infodlg_ )
	infodlg_->propChanged(0);
    applybut_->setSensitive( false );
    undobut_->setSensitive( true );
}


void uiTieWin::clearPicks( CallBacker* cb )
{
    server_.pickMgr().clearAllPicks();
    checkIfPick( cb );
    reDrawSeisViewerAnnot(0);
}


void uiTieWin::clearLastPick( CallBacker* cb )
{
    server_.pickMgr().clearLastPicks();
    checkIfPick( cb );
    reDrawSeisViewerAnnot(0);
}


void uiTieWin::checkIfPick( CallBacker* )
{
    const bool ispick = server_.pickMgr().isPick();
    const bool issamesz = server_.pickMgr().isSynthSeisSameSize();
    clearpicksbut_->setSensitive( ispick );
    clearlastpicksbut_->setSensitive( ispick );
    applybut_->setSensitive( ispick && issamesz );
}


bool uiTieWin::undoPushed( CallBacker* cb )
{
    if ( !server_.undoD2TModel() )
    	mErrRet( "Cannot go back to previous model" );
    doWork( cb );
    clearPicks( cb );

    if ( infodlg_ )
	infodlg_->propChanged(0);
    
    undobut_->setSensitive( false );
    applybut_->setSensitive( false );
    return true;	    
}


bool uiTieWin::matchHorMrks( CallBacker* )
{
    PickSetMgr& pmgr = server_.pickMgr();
    mGetWD(return false)
    if ( !wd || !wd->markers().size() ) 
	mErrRet( "No Well marker found" )

    BufferString msg("No horizon loaded, do you want to load some ?");
    const Data& data = server_.data();
    if ( !data.horizons_.size() )
    {
	if ( !uiMSG().askGoOn( msg ) )
	    return false;
	controlview_->loadHorizons(0);
    }
    pmgr.clearAllPicks();
    uiDialog matchdlg( this, uiDialog::Setup("Settings","",mNoHelpID) );
    uiGenInput* matchinpfld = new uiGenInput( &matchdlg, "Match same", 
				BoolInpSpec(true,"Name","Regional marker") ); 
    matchdlg.go();
    TypeSet<HorizonMgr::PosCouple> pcs;
    server_.horizonMgr().matchHorWithMarkers( pcs, matchinpfld->getBoolValue());
    if ( pcs.isEmpty() )
	mErrRet( "No match between markers and horizons" )
    for ( int idx=0; idx<pcs.size(); idx ++ )
    {
	pmgr.addPick( pcs[idx].z1_, true );
	pmgr.addPick( pcs[idx].z2_, false );
    }
    drawer_->drawUserPicks();
    return true;
}


bool uiTieWin::rejectOK( CallBacker* )
{
    server_.cancelD2TModel();
    close();
    return true;
}


bool uiTieWin::acceptOK( CallBacker* )
{
    BufferString msg("This will overwrite your depth/time model, do you want to continue?");
    if ( uiMSG().askOverwrite(msg) )
    {
       if ( !server_.commitD2TModel() )
	    mErrRet("Cannot write new depth/time model")
	close();
    }
    return false;
}


void uiTieWin::dispInfoMsg( CallBacker* cb )
{
    mCBCapsuleUnpack(BufferString,mesg,cb);
    statusBar()->message( mesg.buf() );
}


static const char* sKeyInfoIsInitWvltActive = "Is init wavelet active";
static const char* sKeyInfoSelBoxIndex = "Selected index";
static const char* sKeyInfoSelZrange = "Selected Z Range";
static const char* sKeyStartMrkrName = "Start Marker Name";
static const char* sKeyStopMrkrName = "Stop Marker Name";

uiInfoDlg::uiInfoDlg( uiParent* p, Server& server )
	: uiDialog(p,uiDialog::Setup("Cross-checking parameters", "",
				     "107.4.2").modal(false))
	, server_(server)
	, selidx_(0)			 
      	, crosscorr_(0)	       
	, wvltdraw_(0)
	, redrawNeeded(this)
	, data_(server_.data())
{
    setCtrlStyle( LeaveOnly );
    
    uiGroup* viewersgrp = new uiGroup( this, "Viewers group" );
    uiGroup* wvltgrp = new uiGroup( viewersgrp, "wavelet group" );
    uiGroup* corrgrp = new uiGroup( viewersgrp, "CrossCorrelation group" );

    ObjectSet<Wavelet> wvlts; 
    wvlts += &data_.initwvlt_; 
    wvlts += &data_.estimatedwvlt_;
    wvltdraw_ = new WellTie::uiWaveletView( wvltgrp, wvlts );
    wvltdraw_->activeWvltChged.notify(mCB(this,WellTie::uiInfoDlg,wvltChanged));
    wvltdraw_->setActiveWavelet( data_.isinitwvltactive_ );
    estwvltlengthfld_ = new uiGenInput(wvltgrp,"Estimated wavelet length (ms)");
    estwvltlengthfld_ ->attach( centeredBelow, wvltdraw_ );
    estwvltlengthfld_->valuechanged.notify( mCB(this,uiInfoDlg,propChanged) );

    uiSeparator* verSepar = new uiSeparator( viewersgrp,"Verical", false );
    verSepar->attach( rightTo, wvltgrp );

    corrgrp->attach( rightOf, verSepar );
    crosscorr_ = new uiCrossCorrView( corrgrp, data_ );

    uiSeparator* horSepar = new uiSeparator( this );
    horSepar->attach( stretchedAbove, viewersgrp );
    
    uiGroup* markergrp =  new uiGroup( this, "User Z Range Group" );
    markergrp->attach( centeredAbove, horSepar );

    const char* choice[] = { "Markers", "Times", "Depths", 0 };
    choicefld_ = new uiGenInput( markergrp, "Compute Data between", 
	    				StringListInpSpec(choice) );
    choicefld_->valuechanged.notify( mCB(this,uiInfoDlg,propChanged) );
    
    markernames_.add( Well::TrackSampler::sKeyDataStart() );
    mGetWD(return)
    if ( wd && wd->haveMarkers() )
    {
	for ( int idx=0; idx<wd->markers().size(); idx++)
	    markernames_.add( wd->markers()[idx]->name() );
    }
    markernames_.add( Well::TrackSampler::sKeyDataEnd() );
    StringListInpSpec slis( markernames_ );
    const char* markernms[] = { "Top Marker", "Bottom Marker", 0 };
    const char* units[] = { "", "seconds", "meters", 0 };

    for ( int idx=0; choice[idx]; idx++ )
    {
	if ( !idx )
	{
	    zrangeflds_ += new uiGenInput( markergrp, "", 
				slis.setName(markernms[0]),
				slis.setName(markernms[1]) ); 
	    zrangeflds_[idx]->setValue( markernames_.size()-1, 1 );
	}
	else
	    zrangeflds_ += new uiGenInput( markergrp, "",
					    FloatInpIntervalSpec() );
	zrangeflds_[idx]->valuechanged.notify(mCB(this,uiInfoDlg,propChanged));
	zrangeflds_[idx]->attach( rightOf, choicefld_ );
	zlabelflds_ += new uiLabel( markergrp, units[idx] );
	zlabelflds_[idx]->attach( rightOf, zrangeflds_[idx] );
    }
    putToScreen();
    finaliseDone.notify( mCB(this,uiInfoDlg,propChanged) );
}


uiInfoDlg::~uiInfoDlg()
{
    delete crosscorr_;
    delete wvltdraw_;
}


void uiInfoDlg::putToScreen()
{
    const Wavelet& wvlt = data_.isinitwvltactive_ ? data_.initwvlt_
						 : data_.estimatedwvlt_;
    wvltdraw_->setActiveWavelet( data_.isinitwvltactive_ );
    estwvltlengthfld_->setValue( wvlt.samplePositions().width()*1000 );
    zrangeflds_[selidx_]->setValue( zrg_ );
    if ( !selidx_ )
    {
	zrangeflds_[0]->setText( startmrknm_.buf(), 0 );
	zrangeflds_[0]->setText( stopmrknm_.buf(), 1 );
    }
}


bool uiInfoDlg::getMarkerDepths( Interval<float>& zrg )
{
    mGetWD(return false)
    zrg.start = wd->d2TModel()->getDah( 0 );
    zrg.stop = wd->track().dah( wd->track().size()-1 );

    const Interval<int> mintv = zrangeflds_[0]->getIInterval();
    const bool zinft = SI().depthsInFeetByDefault();
    const Well::Marker* topmarkr = 
			wd->markers().getByName( zrangeflds_[0]->text(0) );
    const Well::Marker* botmarkr = 
			wd->markers().getByName( zrangeflds_[0]->text(1) );

    if ( mintv.start == mintv.stop ) { topmarkr = 0; botmarkr = 0; }
    if ( topmarkr ) zrg.start = topmarkr->dah();
    if ( botmarkr ) zrg.stop = botmarkr->dah();

    zrg.sort();
    return true;
}


void uiInfoDlg::fillPar( IOPar& par ) const
{
    par.setYN( sKeyInfoIsInitWvltActive, data_.isinitwvltactive_ );
    par.set( sKeyInfoSelBoxIndex, selidx_ );
    par.set( sKeyInfoSelZrange, zrg_ );
    par.set( sKeyStartMrkrName, zrangeflds_[0]->text(0) );
    par.set( sKeyStopMrkrName, zrangeflds_[0]->text(1) );
}


void uiInfoDlg::usePar( const IOPar& par ) 
{
    bool isinitwvltactive;
    par.getYN( sKeyInfoIsInitWvltActive, isinitwvltactive );
    par.get( sKeyInfoSelBoxIndex, selidx_ );
    par.get( sKeyInfoSelZrange, zrg_ );
    par.get( sKeyStartMrkrName, startmrknm_ );
    par.get( sKeyStopMrkrName, stopmrknm_ );

    putToScreen();
    propChanged(0);
}


#define d2T( zval, time ) time ? d2t->getTime( zval ) : d2t->getDah( zval ); 
#define d2TI( zrg, time )\
    { zrg.start = d2T( zrg.start, time ); zrg.stop = d2T( zrg.stop, time ) }
void uiInfoDlg::propChanged( CallBacker* )
{
    mGetWD(return)
    selidx_ = choicefld_->getIntValue();
    zrg_ = zrangeflds_[selidx_]->getFInterval();
    if ( !selidx_ ) getMarkerDepths( zrg_ );

    for ( int idx=0; idx<zrangeflds_.size(); idx++ )
    {
	zrangeflds_[idx]->display( idx == selidx_ );
	zlabelflds_[idx]->display( idx == selidx_ );
    }
    const Well::D2TModel* d2t = wd->d2TModel();
    if ( !selidx_ )
	zrangeflds_[2]->setValue( zrg_ );
    else if ( selidx_ == 1 )
    	{ d2TI( zrg_, false ); zrangeflds_[2]->setValue( zrg_ ); }
    if ( !selidx_ || selidx_ == 2 )
    	{ d2TI( zrg_, true ); zrangeflds_[1]->setValue( zrg_ );  }

    const double wvltlgth = (double)estwvltlengthfld_->getIntValue()/1000;
    if ( wvltlgth >= zrg_.width() )
    {
	uiMSG().error("the wavelet must be shorter than the computation time");
	return;
    }
    estwvltsz_ =  mNINT( wvltlgth/SI().zStep() ); 
    wvltChanged(0);
}


#define mLag 200
#define mDelAndReturn() { delete [] seisarr;  delete [] syntharr; return; }
void uiInfoDlg::computeData()
{
    zrg_.limitTo( data_.timeintv_ );
    const float step = data_.timeintv_.step;
    const int nrsamps = (int)( zrg_.width()/step );

    mDeclareAndTryAlloc( float*, seisarr, float[nrsamps] );
    mDeclareAndTryAlloc( float*, corrarr, float[nrsamps] );
    mDeclareAndTryAlloc( float*, syntharr, float[nrsamps] );

#define mGetIdx data_.timeintv_.getIndex( zrg_.atIndex( idx, step ) ) 
    for ( int idx=0; idx<nrsamps; idx++ )
    {
	syntharr[idx] = data_.seistrc_.get( mGetIdx, 0 );
	seisarr[idx] = data_.synthtrc_.get( mGetIdx, 0 );
    }
    GeoCalculator gc;
    double coeff = gc.crossCorr( seisarr, syntharr, corrarr, nrsamps );
    const float normalfactor = coeff / corrarr[nrsamps/2];
    crosscorr_->set( corrarr, nrsamps, mLag, coeff );
    delete [] corrarr; 

    if ( data_.isinitwvltactive_ )
    {
	int wvltsz = estwvltsz_;
	wvltsz += wvltsz%2 ? 0 : 1;
	data_.estimatedwvlt_.reSize( wvltsz );
	if ( data_.timeintv_.nrSteps() < wvltsz )
	{ 
	    uiMSG().error( "Seismic trace shorter than wavelet" ); 
	    mDelAndReturn()
	}

	const Well::Log* log = data_.logset_.getLog( data_.reflectivity() );
	if ( !log )
	{ 
	    uiMSG().error( "No reflectivity to estimate wavelet" ); 
	    mDelAndReturn(); 
	}

	mDeclareAndTryAlloc( float*, refarr, float[nrsamps] );
	mDeclareAndTryAlloc( float*, wvltarr, float[nrsamps] );
	mDeclareAndTryAlloc( float*, wvltshiftedarr, float[wvltsz] );

	for ( int idx=0; idx<nrsamps; idx++ )
	    refarr[idx] = log->valArr()[mGetIdx];

	gc.deconvolve( seisarr, refarr, wvltarr, nrsamps );
	for ( int idx=0; idx<wvltsz; idx++ )
	    wvltshiftedarr[idx] = wvltarr[(nrsamps-wvltsz)/2 + idx];

	Array1DImpl<float> wvltvals( wvltsz );
	memcpy( wvltvals.getData(), wvltshiftedarr, wvltsz*sizeof(float) );
	ArrayNDWindow window( Array1DInfoImpl(wvltsz), false, "CosTaper", .05 );
	window.apply( &wvltvals );
	memcpy( wvltshiftedarr, wvltvals.getData(), wvltsz*sizeof(float) );

	server_.setEstimatedWvlt( wvltshiftedarr, wvltsz );
	delete [] wvltarr; 	delete [] wvltshiftedarr;\
    }
    mDelAndReturn()
}


void uiInfoDlg::drawData()
{
    wvltdraw_->redrawWavelets();
    crosscorr_->draw();
}


void uiInfoDlg::wvltChanged( CallBacker* cb )
{
    if ( cb )
    {
	mCBCapsuleUnpack(bool,isinitwvlatactive,cb);
	server_.setInitWvltActive( isinitwvlatactive );
    }
    computeData();
    server_.computeSynthetics(); 
    drawData();
    redrawNeeded.trigger();
}

}; //namespace WellTie
