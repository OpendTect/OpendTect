/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiwelltietoseismicdlg.cc,v 1.17 2009-06-17 11:48:13 cvsranojay Exp $";

#include "uiwelltietoseismicdlg.h"
#include "uiwelltiecontrolview.h"
#include "uiwelltieeventstretch.h"
#include "uiwelltielogstretch.h"
#include "uiwelltiestretch.h"
#include "uiwelltieview.h"
#include "uiwelltiewavelet.h"

#include "uibutton.h"
#include "uifiledlg.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "uiseparator.h"
#include "uitoolbar.h"
#include "uiwelldlgs.h"

#include "arrayndimpl.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "ctxtioobj.h"
#include "survinfo.h"
#include "welldata.h"
#include "welltrack.h"
#include "wellextractdata.h"
#include "wellman.h"
#include "wellmarker.h"

#include "welltiedata.h"
#include "welltied2tmodelmanager.h"
#include "welltiepickset.h"
#include "welltiesetup.h"
#include "welltietoseismic.h"
#include "welltieunitfactors.h"


#define mErrRet(msg) \
{ uiMSG().error(msg); return false; }
uiWellTieToSeismicDlg::uiWellTieToSeismicDlg( uiParent* p, 
					      const WellTieSetup& wts,
					      const Attrib::DescSet& ads )
	: uiDialog(p,uiDialog::Setup("Tie Well to seismics dialog", "",
		    			mTODOHelpID).modal(false))
    	, setup_(WellTieSetup(wts))
	, wd_(Well::MGR().get(wts.wellid_))
	, dataplayer_(0)
	, datadrawer_(0)
    	, controlview_(0)
	, params_(0)       		 
	, manip_(0)				 
{
    uiTaskRunner* tr = new uiTaskRunner( p );
    toolbar_ = new uiToolBar( this, "Well Tie Control" );
    vwrgrp_ = new uiGroup( this, "viewers group" );
    
    params_ 	  = new WellTieParams( setup_, wd_, ads );
    dataholder_   = new WellTieDataHolder( params_, wd_, setup_ );
    dataplayer_   = new WellTieToSeismic( dataholder_, ads, tr );
    datadrawer_   = new uiWellTieView( vwrgrp_, dataholder_ );
    logstretcher_ = new uiWellTieLogStretch( this, dataholder_, *datadrawer_ );
    logstretcher_->dispdataChanged.notify(
			mCB(this,uiWellTieToSeismicDlg,dispDataChanged) );

    eventstretcher_ = new uiWellTieEventStretch( this, dataholder_,
	    					 *datadrawer_ );
    eventstretcher_->dispdataChanged.notify(
	    		mCB(this,uiWellTieToSeismicDlg,dispDataChanged) );
    eventstretcher_->readyforwork.notify(
	    		mCB(this,uiWellTieToSeismicDlg,applyReady) );
    eventstretcher_->pickadded.notify(
	    		mCB(this,uiWellTieToSeismicDlg,checkIfPick) );
   
    infodlg_ = new uiWellTieInfoDlg( this, dataholder_, params_ );
    infodlg_->applyPushed.notify(
	    		mCB(this,uiWellTieToSeismicDlg,doWholeWork) );

    setWinTitle( ads );
    drawFields( vwrgrp_ );
    infodlg_->setUserDepths();
    initAll();
    
    finaliseDone.notify( mCB(this,uiWellTieToSeismicDlg,setView) );
   /* 
    BufferString msg( "To correlate seismics to synthetics, please select events 	in the synthetic seismic traces and link them to the seismic traces");
    uiMSG().showMsgNextTime(msg);
    */
}


uiWellTieToSeismicDlg::~uiWellTieToSeismicDlg()
{
    if ( eventstretcher_ )
    {
	eventstretcher_->readyforwork.remove(
				mCB(this,uiWellTieToSeismicDlg,applyReady) );
	eventstretcher_->dispdataChanged.remove(
			    mCB(this,uiWellTieToSeismicDlg,dispDataChanged) );
    }
    if ( logstretcher_ )
	logstretcher_->dispdataChanged.remove(
			    mCB(this,uiWellTieToSeismicDlg,dispDataChanged) );

    if ( datadrawer_ )     delete datadrawer_;
    if ( dataholder_ ) 	   delete dataholder_;
    if ( params_ )	   delete params_;
    if ( logstretcher_ )   delete logstretcher_;
    if ( eventstretcher_ ) delete eventstretcher_;
    if ( dataplayer_ )     delete dataplayer_;
}


void uiWellTieToSeismicDlg::setWinTitle( const Attrib::DescSet& ads )
{
    const Attrib::Desc* ad = ads.getDesc( setup_.attrid_ );
    if ( !ad ) return;

    BufferString attrnm = ad->userRef();

    BufferString wname = "Tie ";
    wname += wd_->name();
    wname += " to ";
    wname += attrnm;   
    
    setCaption( wname );
}


void uiWellTieToSeismicDlg::initAll()
{
    addControl();
    addToolBarTools();
    doWholeWork( 0 );
}


void uiWellTieToSeismicDlg::doWholeWork( CallBacker* )
{
    params_->resetDataParams(0); //TODO put as CB
    dataplayer_->computeAll();
    logstretcher_->resetData();
    eventstretcher_->resetData();
    drawData();
    resetInfoDlg();
}

void uiWellTieToSeismicDlg::resetInfoDlg()
{
    infodlg_->setXCorrel();
    infodlg_->setWvlts();
}

void uiWellTieToSeismicDlg::drawData()
{
    datadrawer_->fullRedraw();
    setView(0);
}


#define mAddButton(pm,func,tip) \
    toolbar_->addButton( pm, mCB(this,uiWellTieToSeismicDlg,func), tip )
void uiWellTieToSeismicDlg::addToolBarTools()
{
    toolbar_->addSeparator();
    mAddButton( "z2t.png", editD2TPushed, "View/Edit Model" );
    mAddButton( "saveflow.png",saveD2TPushed, "Save Model" );
    //toolbar_->addSeparator();
    //mAddButton( "xplot.png", viewDataPushed, "View/Plot Data" );
}    


void uiWellTieToSeismicDlg::addControl()
{
    ObjectSet<uiFlatViewer> viewer;
    for (int vwridx=0; vwridx<datadrawer_->viewerSize(); vwridx++)
	viewer += datadrawer_->getViewer( vwridx );

    controlview_ = new uiWellTieControlView( this, toolbar_, viewer );
    controlview_->setPickSetMGR( dataholder_->pickmgr_ );
}


void uiWellTieToSeismicDlg::drawFields( uiGroup* vwrgrp_ )
{
    uiGroup* csgrp = new uiGroup( this, "check shot group" );
    csgrp->attach( ensureBelow, vwrgrp_ );
    csgrp->attach( leftBorder );
    createCSFields( csgrp );

    uiGroup* taskgrp = new uiGroup( this, "task group" );
    taskgrp->attach( ensureBelow, vwrgrp_ );
    taskgrp->attach( rightBorder );
    createTaskFields( taskgrp );

    uiGroup* informgrp = new uiGroup( this, "Indicator Group" );
    informgrp->attach( ensureBelow, csgrp );
    informgrp->attach( hCentered );
    infobut_ = new uiPushButton( informgrp, "&Display additional information",
	               mCB(this,uiWellTieToSeismicDlg,infoPushed), true );
}


//TODO some of them will have to be placed in toolbar
void uiWellTieToSeismicDlg::createTaskFields( uiGroup* taskgrp )
{
    applybut_ = new uiPushButton( taskgrp, "&Apply Changes",
	   mCB(this,uiWellTieToSeismicDlg,applyPushed), true );
    applybut_->setSensitive( false );

    undobut_ = new uiPushButton( taskgrp, "&Undo",
	   mCB(this,uiWellTieToSeismicDlg,undoPushed), true );
    undobut_->attach( ensureRightOf, applybut_ );
    undobut_->setSensitive( false );
    
    clearallpicksbut_ = new uiPushButton( taskgrp, "&Clear picks",
	   mCB(this,uiWellTieToSeismicDlg,clearAllPicksPushed), true );
    clearallpicksbut_->setSensitive( false );
    clearallpicksbut_->attach( ensureRightOf, undobut_ );
}


void uiWellTieToSeismicDlg::createCSFields( uiGroup* csgrp )
{
    cscorrfld_ = new uiCheckBox( csgrp, "use checkshot corrections" );
    cscorrfld_->setChecked(params_->iscscorr_);
    cscorrfld_->activated.notify(mCB(this,uiWellTieToSeismicDlg,checkShotChg));

    csdispfld_ = new uiCheckBox( csgrp, "display checkshot related curve" );
    csdispfld_->setChecked(params_->iscsdisp_);
    csdispfld_->activated.notify(mCB(this,uiWellTieToSeismicDlg,checkShotDisp));
    csdispfld_->attach( alignedBelow, cscorrfld_ );
    
    setPrefWidthInChar( 60 );
    updateButtons();	    
}


void uiWellTieToSeismicDlg::applyReady( CallBacker* )
{
    if ( manip_ )
	manip_ = 0;
    else
	manip_ = 1;
    applybut_->setSensitive( manip_ ? true : false );
}


void uiWellTieToSeismicDlg::setView( CallBacker* )
{
    if ( controlview_ )
	controlview_->setView();
}


void uiWellTieToSeismicDlg::viewDataPushed( CallBacker* )
{
    //TODO dps and uidps class to view data
}


void uiWellTieToSeismicDlg::wvltChg( CallBacker* cb  )
{
    doWholeWork( cb );
}


void uiWellTieToSeismicDlg::updateButtons()
{
    if ( !params_->iscsavailable_ )
    {
	cscorrfld_->display(false);
	csdispfld_->display(false);
    }
}


void uiWellTieToSeismicDlg::checkShotChg( CallBacker* )
{
    params_->iscscorr_ = cscorrfld_->isChecked();
    if ( params_->iscscorr_ )
	params_->currvellognm_ = setup_.corrvellognm_;
    else
	params_->currvellognm_ = setup_.vellognm_;
    datadrawer_->drawVelLog();
    datadrawer_->drawWellMarkers();
    datadrawer_->drawCShot();
    setView(0);
    applybut_->setSensitive(true);
}


void uiWellTieToSeismicDlg::checkShotDisp( CallBacker* )
{
    params_->iscsdisp_ = csdispfld_->isChecked();
    datadrawer_->drawCShot();
    setView(0);
}


void uiWellTieToSeismicDlg::dispDataChanged( CallBacker* )
{
    dataplayer_->setd2TModelFromData();
    applybut_->setSensitive( true );
}


void uiWellTieToSeismicDlg::infoPushed( CallBacker* )
{
    infodlg_->go();
}


bool uiWellTieToSeismicDlg::editD2TPushed( CallBacker* )
{
    uiD2TModelDlg d2tmdlg( this, *wd_, false );

    if ( d2tmdlg.go() )
    {
	applybut_->setSensitive(true);
	undobut_->setSensitive( true );
	return true;
    }
    return false;
}


bool uiWellTieToSeismicDlg::saveD2TPushed( CallBacker* )
{
    uiFileDialog* uifiledlg =
	new uiFileDialog( this, false,"","", "Save depth/time table" );
    uifiledlg->setDirectory(
             IOObjContext::getDataDirName(IOObjContext::WllInf) );
    
    if ( uifiledlg->go() );
    {
	if ( !dataplayer_->saveD2TModel(uifiledlg->fileName()) )
	    return true;
    }
    delete uifiledlg;
   
    return true;
}


void uiWellTieToSeismicDlg::applyPushed( CallBacker* cb )
{
    eventstretcher_->doWork(0);
    //dataholder_->pickmgr_->clearAllPicks();
    doWholeWork( cb );
    applybut_->setSensitive( false );
    undobut_->setSensitive( true );
    manip_ = 0;
}


void uiWellTieToSeismicDlg::clearPickPushed( CallBacker* )
{
    dataholder_->pickmgr_->clearLastPicks();
    drawData();
    checkIfPick(0);
}


void uiWellTieToSeismicDlg::clearAllPicksPushed( CallBacker* )
{
    dataholder_->pickmgr_->clearAllPicks();
    drawData();
    checkIfPick(0);
}


void uiWellTieToSeismicDlg::checkIfPick( CallBacker* )
{
    bool ispick = dataholder_->pickmgr_->checkIfPick();
    //clearpickbut_->setSensitive( ispick );
    clearallpicksbut_->setSensitive( ispick );
    if ( !ispick )
	applybut_->setSensitive( false );
}


bool uiWellTieToSeismicDlg::undoPushed( CallBacker* cb )
{
    if ( !dataplayer_->undoD2TModel() )
    	mErrRet( "Cannot go back to previous model" );
    
    doWholeWork( cb );
    undobut_->setSensitive( false );
    applybut_->setSensitive( false );

    return true;	    
}


bool uiWellTieToSeismicDlg::rejectOK( CallBacker* )
{
    dataplayer_->cancelD2TModel();
    return true;
}


bool uiWellTieToSeismicDlg::acceptOK( CallBacker* )
{
    if ( !dataplayer_->commitD2TModel() )
	mErrRet("Cannot write new depth/time model")
    else
    {
	BufferString msg("This will overwrite your depth/time model, do you want to continue?");
	return uiMSG().askOverwrite(msg);
    }
}




uiWellTieInfoDlg::uiWellTieInfoDlg( uiParent* p, 
				    WellTieDataHolder* dh,
				    WellTieParams* pms )
	: uiDialog(p,uiDialog::Setup("Cross-checking parameters", "",
				     mTODOHelpID).modal(false))
	, dataholder_(dh)
	, wd_(dh->wd())		 
	, params_(pms) 
      	, crosscorr_(0)	       
	, wvltdraw_(0)
	, applyPushed(this)
{
    setCtrlStyle( LeaveOnly );
    
    uiGroup* panelsgrp = new uiGroup( this, "wavelet group" );
    uiGroup* wvltgrp = new uiGroup( panelsgrp, "wavelet group" );
    wvltdraw_ = new uiWellTieWaveletView( wvltgrp, dataholder_ );
    //wvltdraw_->wvltChanged.notify( mCB(this,uiWellTieToSeismicDlg,wvltChg) );

    uiGroup* corrgrp = new uiGroup( panelsgrp, "CrossCorrelation group" );
    corrgrp->attach( rightOf, wvltgrp );
    crosscorr_ = new uiWellTieCorrView( corrgrp, dataholder_ );

    uiGroup* markergrp =  new uiGroup( this, "User Position Group" );
    markergrp->attach( centeredAbove, panelsgrp );
    markernames_.add( Well::TrackSampler::sKeyDataStart() );
    for ( int idx=0; idx<Well::MGR().wells()[0]->markers().size(); idx++ )
	markernames_.add( Well::MGR().wells()[0]->markers()[idx]->name() );
    markernames_.add( Well::TrackSampler::sKeyDataEnd() );
    StringListInpSpec slis( markernames_ );

    topmrkfld_ = new uiGenInput( markergrp, "Compute data between",
	    			 slis.setName("Top Marker") );
    topmrkfld_->setValue( (int)0 );
    topmrkfld_->setElemSzPol( uiObject::Medium );
    topmrkfld_->valuechanged.notify(
			mCB(this, uiWellTieInfoDlg, userDepthsChanged));
    
    botmrkfld_ = new uiGenInput( markergrp, "", slis.setName("Bottom Marker") );
    botmrkfld_->attach( rightOf, topmrkfld_ );
    botmrkfld_->setValue( markernames_.size()-1 );
    botmrkfld_->setElemSzPol( uiObject::Medium );
    botmrkfld_->valuechanged.notify(
			mCB(this, uiWellTieInfoDlg, userDepthsChanged));
    
    applymrkbut_ = new uiPushButton( markergrp, "&Apply",
	       mCB(this,uiWellTieInfoDlg,applyMarkerPushed), true );
    applymrkbut_->setSensitive( false );
    applymrkbut_->attach( rightOf, botmrkfld_ );
}


uiWellTieInfoDlg::~uiWellTieInfoDlg()
{
    //if ( wvltdraw_  )
	//wvltdraw_->wvltChanged.remove(mCB(this,uiWellTieToSeismicDlg,wvltChg));
    //if ( crosscorr_ )      delete crosscorr_;
}


void uiWellTieInfoDlg::applyMarkerPushed( CallBacker* )
{
    if ( !setUserDepths() ) return;
    applymrkbut_->setSensitive( false );
    applyPushed.trigger();
}


bool uiWellTieInfoDlg::setUserDepths()
{
    const bool zinft = SI().depthsInFeetByDefault();
    const int topmarker = markernames_.indexOf( topmrkfld_->text() );
    const int bottommarker = markernames_.indexOf( botmrkfld_->text() );
    float startdah = 0;
    float stopdah = 0;
    if ( topmarker == 0 )
	startdah = wd_->track().dah(0);
    else
	startdah = wd_->markers()[ topmarker-1 ]->dah();

    if ( markernames_.size()-1 != bottommarker )
	stopdah = wd_->markers()[ bottommarker-1 ]->dah();
    else
	stopdah = wd_->track().dah( wd_->track().size()-1 );

    if ( startdah >= stopdah )
	mErrRet("Please choose the Markers correctly");
    
    params_->corrstartdah_ = startdah;
    params_->corrstopdah_  = stopdah;

    return true;
}


void uiWellTieInfoDlg::userDepthsChanged( CallBacker* )
{
    applymrkbut_->setSensitive(true);
}


void uiWellTieInfoDlg::setXCorrel()
{
    crosscorr_->setCrossCorrelation();
}


void uiWellTieInfoDlg::setWvlts()
{
    wvltdraw_->initWavelets();
}
