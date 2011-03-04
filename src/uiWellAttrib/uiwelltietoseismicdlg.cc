/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiwelltietoseismicdlg.cc,v 1.81 2011-03-04 14:13:51 cvsbruno Exp $";

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
#include "uiwelldlgs.h"
#include "uiwelllogdisplay.h"

#include "seistrc.h"
#include "wavelet.h"
#include "welldata.h"
#include "welld2tmodel.h"
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
    	, setup_(wts)
	, server_(*new Server(setup_))
	, stretcher_(*new EventStretch(server_.pickMgr())) 
    	, controlview_(0)
	, infodlg_(0)
	, params_(server_.dispParams())
{
    stretcher_.timeChanged.notify(mCB(this,uiTieWin,timeChanged));
    drawer_ = new uiTieView( this, &viewer(), server_.data() );
    drawer_->infoMsgChanged.notify( mCB(this,uiTieWin,dispInfoMsg) );
    server_.pickMgr().pickadded.notify( mCB(this,uiTieWin,checkIfPick) );
    
    mGetWD(return) 
    BufferString title( "Tie ");
    title += wd->name(); title += " to "; title += wts.seisnm_;
    setCaption( title );

    initAll();
}


uiTieWin::~uiTieWin()
{
    stretcher_.timeChanged.remove(mCB(this,uiTieWin,timeChanged));
    delete &stretcher_;
    delete &setup_;
    delete drawer_;
    delete &server_;
}


void uiTieWin::initAll()
{
    drawFields();
    addControl();
    doWork( 0 );
    show();
    dispPropChg( 0 );
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
    server_.computeAll();
    getDispParams();
    drawData();
    drawer_->enableCtrlNotifiers( true );
}


void uiTieWin::reDrawSeisViewer( CallBacker* )
{
    drawer_->redrawViewer();
}


void uiTieWin::drawData()
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
    mAddButton( "save.png", saveDataPushed, "Save Data" );
}    


void uiTieWin::addControl()
{
    addToolBarTools();
    controlview_ = new WellTie::uiControlView(this,toolbar_,&viewer(),server_);
    controlview_->redrawNeeded.notify( mCB(this,uiTieWin,reDrawSeisViewer) );
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
    
    markerfld_ = new uiCheckBox( dispgrp, "display Markers" );
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
    drawData();
}


void uiTieWin::timeChanged( CallBacker* )
{
    const Array1DImpl<float>* timearr = stretcher_.timeArr();
    if ( timearr )
    {
	server_.replaceTime( *timearr );
	applybut_->setSensitive( true );
    }
}


void uiTieWin::csCorrChanged( CallBacker* cb )
{
    mGetWD(return);
    getDispParams();
    const bool iscscorr = params_.iscscorr_;
    server_.setVelLogName( iscscorr );

    if ( !iscscorr )
	server_.undoD2TModel();

    doWork( cb );
}


void uiTieWin::infoPushed( CallBacker* )
{
    if ( !infodlg_ )
    {
	infodlg_ = new uiInfoDlg( this, server_ );
	infodlg_->redrawNeeded.notify( mCB(this,uiTieWin,reDrawSeisViewer) );
    }
    infodlg_->show();
}


void uiTieWin::editD2TPushed( CallBacker* cb )
{
    mGetWD(return);
    Well::Data newwd( *wd );
    uiD2TModelDlg d2tmdlg( this, newwd, false );
    if ( d2tmdlg.go() )
    {
	server_.resetD2TModel( newwd.d2TModel() );
	doWork( cb );
    }
}


bool uiTieWin::saveDataPushed( CallBacker* cb )
{
    uiSaveDataDlg dlg( this, server_.data() );
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
    infodlg_->propChanged(0);
    applybut_->setSensitive( false );
    undobut_->setSensitive( true );
}


void uiTieWin::clearPicks( CallBacker* cb )
{
    server_.pickMgr().clearAllPicks();
    drawer_->drawUserPicks();
    checkIfPick( cb );
    reDrawSeisViewer(0);
}


void uiTieWin::clearLastPick( CallBacker* cb )
{
    server_.pickMgr().clearLastPicks();
    drawer_->drawUserPicks();
    checkIfPick( cb );
    reDrawSeisViewer(0);
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
    clearPicks( cb );
    doWork( cb );
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




uiInfoDlg::uiInfoDlg( uiParent* p, Server& server )
	: uiDialog(p,uiDialog::Setup("Cross-checking parameters", "",
				     "107.4.2").modal(false))
	, server_(server)
      	, crosscorr_(0)	       
	, wvltdraw_(0)
	, redrawNeeded(this)
{
    setCtrlStyle( LeaveOnly );
    
    uiGroup* viewersgrp = new uiGroup( this, "Viewers group" );
    uiGroup* wvltgrp = new uiGroup( viewersgrp, "wavelet group" );
    uiGroup* corrgrp = new uiGroup( viewersgrp, "CrossCorrelation group" );

    const Data& data = server_.data();
    ObjectSet<Wavelet> wvlts; 
    wvlts += &data.initwvlt_; 
    wvlts += &data.estimatedwvlt_;
    wvltdraw_ = new WellTie::uiWaveletView( wvltgrp, wvlts );
    wvltdraw_->activeWvltChged.notify(mCB(this,WellTie::uiInfoDlg,wvltChanged));
    estwvltlengthfld_ = new uiGenInput(wvltgrp,"Estimated wavelet length (ms)");
    estwvltlengthfld_ ->attach( centeredBelow, wvltdraw_ );
    estwvltlengthfld_->valuechanged.notify( mCB(this,uiInfoDlg,propChanged) );
    estwvltlengthfld_->setValue( wvlts[0]->samplePositions().width()*1000 );

    uiSeparator* verSepar = new uiSeparator( viewersgrp,"Verical", false );
    verSepar->attach( rightTo, wvltgrp );

    corrgrp->attach( rightOf, verSepar );
    crosscorr_ = new uiCrossCorrView( corrgrp, server.data() );

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
    finaliseDone.notify( mCB(this,uiInfoDlg,propChanged) );
}


uiInfoDlg::~uiInfoDlg()
{
    delete wvltdraw_;
    delete crosscorr_;
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

#define d2T( zval, time ) time ? d2t->getTime( zval ) : d2t->getDah( zval ); 
#define d2TI( zrg, time )\
    { zrg.start = d2T( zrg.start, time ); zrg.stop = d2T( zrg.stop, time ) }
void uiInfoDlg::propChanged( CallBacker* )
{
    mGetWD(return)
    const int selidx = choicefld_->getIntValue();
    zrg_ = zrangeflds_[selidx]->getFInterval();
    if ( !selidx ) getMarkerDepths( zrg_ );

    for ( int idx=0; idx<zrangeflds_.size(); idx++ )
    {
	zrangeflds_[idx]->display( idx == selidx );
	zlabelflds_[idx]->display( idx == selidx );
    }
    const Well::D2TModel* d2t = wd->d2TModel();
    if ( !selidx )
	zrangeflds_[2]->setValue( zrg_ );
    else if ( selidx == 1 )
    	{ d2TI( zrg_, false ); zrangeflds_[2]->setValue( zrg_ ); }
    if ( !selidx || selidx == 2 )
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


#define mLag 0.4
void uiInfoDlg::computeData()
{
    return;
    const Data& data = server_.data();
    const int sz = data.seistrc_.size();
    const int nrsamps = (int) ( mLag / data.timeintv_.step );
    float* seisarr = new float( nrsamps );
    float* syntharr = new float( nrsamps );
    float* corrarr = new float( nrsamps );
    float* wvltarr = new float( estwvltsz_ );
    float* wvltshiftedarr = new float( estwvltsz_ );
    for ( int idx=0; idx<nrsamps; idx++ )
    {
	syntharr[idx] = data.seistrc_.get( idx + ( sz-nrsamps )/2, 0 );
	seisarr[idx] = data.synthtrc_.get( idx + ( sz-nrsamps )/2, 0 );
    }
    GeoCalculator gc;
    double coeff = gc.crossCorr( seisarr, syntharr, corrarr, nrsamps );
    const float normalfactor = coeff / corrarr[nrsamps/2];
    crosscorr_->set( corrarr, nrsamps, mLag, coeff );

    gc.deconvolve( seisarr, syntharr, wvltarr, estwvltsz_ );
    for ( int idx=0; idx<estwvltsz_; idx++ )
	wvltshiftedarr[idx] = wvltarr[nrsamps/2 + idx - estwvltsz_/2];
    server_.setEstimatedWvlt( wvltshiftedarr, estwvltsz_ );

    delete [] syntharr; delete [] corrarr; 
    delete [] wvltarr; 	delete [] wvltshiftedarr;
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
