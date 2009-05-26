/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiwelltietoseismicdlg.cc,v 1.9 2009-05-26 07:06:53 cvsbruno Exp $";

#include "uiwelltietoseismicdlg.h"
#include "uiwelltiecontrolview.h"
#include "uiwelltieview.h"
#include "uiwelltiestretch.h"
#include "uiwelltiewavelet.h"
#include "uiwelltieeventstretch.h"
#include "uiwelltielogstretch.h"

#include "uigroup.h"
#include "uibutton.h"
#include "uidatapointset.h"
#include "uifiledlg.h"
#include "uimsg.h"
#include "uiwelldlgs.h"
#include "uigeninput.h"
#include "uitoolbar.h"
#include "uiseparator.h"
#include "uitaskrunner.h"

#include "arrayndimpl.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "ctxtioobj.h"
#include "datapointset.h"
#include "survinfo.h"

#include "welldata.h"
#include "welltrack.h"
#include "wellextractdata.h"
#include "wellimpasc.h"
#include "welllogset.h"
#include "welllog.h"
#include "wellman.h"
#include "wellmarker.h"

#include "welltiecshot.h"
#include "welltiedata.h"
#include "welltiesetup.h"
#include "welltieunitfactors.h"
#include "welltietoseismic.h"
#include "welltiepickset.h"


#define mErrRet(msg) \
{ uiMSG().error(msg); return false; }
uiWellTieToSeismicDlg::uiWellTieToSeismicDlg( uiParent* p, 
					      const WellTieSetup& wts,
					      const Attrib::DescSet& ads, 
					      bool isdelclose)
	: uiDialog(p,uiDialog::Setup("Tie Well to seismics dialog", "",
		    			mTODOHelpID).modal(false))
    	, setup_(WellTieSetup(wts))
	, wd_(Well::MGR().get(wts.wellid_))
	, dataplayer_(0)
	, wvltdraw_(0)
	, datadrawer_(0)
    	, controlview_(0)
	, params_(0)       		 
	, manip_(0)				 
{
    uiTaskRunner* tr = new uiTaskRunner( p );
    toolbar_ = new uiToolBar( this, "Well Tie Control" );
    vwrgrp_ = new uiGroup( this, "viewers group" );

    picksetmgr_   = new WellTiePickSetMGR();
    params_ 	  = new WellTieParams( setup_, wd_, ads );
    datamgr_      = new WellTieDataMGR( params_ );
    dataplayer_   = new WellTieToSeismic( wd_, params_, ads, *datamgr_, tr );
    datadrawer_   = new uiWellTieView( vwrgrp_, *datamgr_, wd_, params_ );
    logstretcher_ = new uiWellTieLogStretch( this, params_, wd_, *datamgr_,
					     *datadrawer_, *picksetmgr_ );
    logstretcher_->dispdataChanged.notify(
	    		mCB(this,uiWellTieToSeismicDlg,dispDataChanged) );
    eventstretcher_ = new uiWellTieEventStretch( this, params_, wd_, *datamgr_,
						 *datadrawer_, *picksetmgr_ );
    eventstretcher_->dispdataChanged.notify(
	    		mCB(this,uiWellTieToSeismicDlg,dispDataChanged) );
    eventstretcher_->readyforwork.notify(
	    		mCB(this,uiWellTieToSeismicDlg,applyReady) );
    
    setWinTitle( ads );
    drawFields( vwrgrp_ );
    initAll();
    
    finaliseDone.notify( mCB(this,uiWellTieToSeismicDlg,setView) );
}


uiWellTieToSeismicDlg::~uiWellTieToSeismicDlg()
{
    if ( wvltdraw_  )
	wvltdraw_->wvltChanged.remove(mCB(this,uiWellTieToSeismicDlg,wvltChg));
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
    if ( params_ )	   delete params_;
    if ( datamgr_ ) 	   delete datamgr_;
    if ( wd_ ) 		   delete wd_;	 
    if ( logstretcher_ )   delete logstretcher_;
    if ( eventstretcher_ ) delete eventstretcher_;
    if ( picksetmgr_ ) 	   delete picksetmgr_;	
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
    doWholeWork();
}


void uiWellTieToSeismicDlg::doWholeWork()
{
    dataplayer_->computeAll();
    logstretcher_->resetData();
    eventstretcher_->resetData();
    drawData();
}


void uiWellTieToSeismicDlg::doLogWork()
{
    wvltdraw_->initWavelets( dataplayer_->estimateWavelet() );
    datadrawer_->drawDenLog();
    datadrawer_->drawVelLog();
    datadrawer_->drawSynthetics();
}


void uiWellTieToSeismicDlg::drawData()
{
    wvltdraw_->initWavelets( dataplayer_->estimateWavelet() );
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
    controlview_->setPickSetMGR( picksetmgr_ );
}


void uiWellTieToSeismicDlg::drawFields( uiGroup* vwrgrp_ )
{	
    uiGroup* taskgrp = new uiGroup( this, "task group" );
    taskgrp->attach( alignedBelow, vwrgrp_ );
    createTaskFields( taskgrp );

    uiSeparator* spl = new uiSeparator( this, "sep", false, true );
    spl->attach( alignedBelow, taskgrp ) ;
    
    uiGroup* wvltgrp = new uiGroup( this, "wavelet group" );
    wvltgrp->attach( alignedBelow, spl );
    wvltgrp->attach( ensureBelow, spl );
    wvltdraw_ = new uiWellTieWavelet( wvltgrp, setup_ );
    wvltdraw_->wvltChanged.notify( mCB(this,uiWellTieToSeismicDlg,wvltChg) );

    uiGroup* markergrp =  new uiGroup( this, "User Position Group" );
    markergrp->attach( centeredAbove, vwrgrp_ );
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
			mCB(this, uiWellTieToSeismicDlg, userDepthsChanged));
    

    botmrkfld_ = new uiGenInput( markergrp, "", slis.setName("Bottom Marker") );
    botmrkfld_->attach( rightOf, topmrkfld_ );
    botmrkfld_->setValue( markernames_.size()-1 );
    botmrkfld_->setElemSzPol( uiObject::Medium );
    botmrkfld_->valuechanged.notify(
			mCB(this, uiWellTieToSeismicDlg, userDepthsChanged));
    
    applymrkbut_ = new uiPushButton( markergrp, "Apply",
	       mCB(this,uiWellTieToSeismicDlg,applyMarkerPushed),false );
    applymrkbut_->setSensitive( false );
    applymrkbut_->attach( rightOf, botmrkfld_ );

   /* 
    corrcoefffld_ = new uiGenInput( markergrp, "Correlation coefficient", 
	    				FloatInpSpec(0));
    corrcoefffld_->attach( alignedBelow, wvltgrp );
    corrcoefffld_->attach( ensureBelow, wvltgrp );*/
}


//TODO some of them will have to be placed in toolbar
void uiWellTieToSeismicDlg::createTaskFields( uiGroup* taskgrp )
{
    applybut_ = new uiPushButton( taskgrp, "Apply Changes",
       mCB(this,uiWellTieToSeismicDlg,applyPushed),false );
    applybut_->setSensitive( false );

    undobut_ = new uiPushButton( taskgrp, "Undo",
	   mCB(this,uiWellTieToSeismicDlg,undoPushed), false );
    undobut_->attach( ensureRightOf, applybut_ );
    undobut_->setSensitive( false );

    cscorrfld_ = new uiCheckBox( taskgrp, "use checkshot corrections" );
    cscorrfld_->attach( rightOf, undobut_ );
    cscorrfld_->setChecked(params_->iscscorr_);
    cscorrfld_->activated.notify(mCB(this,uiWellTieToSeismicDlg,checkShotChg));

    csdispfld_ = new uiCheckBox( taskgrp, "display checkshot related curve" );
    csdispfld_->setChecked(params_->iscsdisp_);
    csdispfld_->activated.notify(mCB(this,uiWellTieToSeismicDlg,checkShotDisp));
    csdispfld_->attach( rightOf, cscorrfld_ );
    
    setPrefWidthInChar( 60 );
    updateButtons();	    
}


bool uiWellTieToSeismicDlg::setUserDepths()
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

    if ( startdah > stopdah )
	mErrRet("Please choose the Markers correctly");
    
    params_->startdah_ = startdah;
    params_->stopdah_ = stopdah;

    return true;
}


void uiWellTieToSeismicDlg::userDepthsChanged( CallBacker* )
{
    applymrkbut_->setSensitive(true);
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


void uiWellTieToSeismicDlg::wvltChg( CallBacker* )
{
    doLogWork();
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


bool uiWellTieToSeismicDlg::editD2TPushed( CallBacker* )
{
    uiD2TModelDlg d2tmdlg( this, *wd_, false );

    if ( d2tmdlg.go() )
    {
	applybut_->setSensitive(true);
	undobut_->setSensitive( true );
	return true;
    }
    else
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


void uiWellTieToSeismicDlg::applyPushed( CallBacker* )
{
    eventstretcher_->doWork(0);
    picksetmgr_->clearAllPicks();
    setUserDepths();
    doWholeWork();
    applybut_->setSensitive( false );
    undobut_->setSensitive( true );
    manip_ = 0;
}


void uiWellTieToSeismicDlg::applyMarkerPushed( CallBacker* )
{
    if ( !setUserDepths() ) return;
    doWholeWork();
    applymrkbut_->setSensitive( false );
}


bool uiWellTieToSeismicDlg::undoPushed( CallBacker* )
{
    if ( !dataplayer_->undoD2TModel() )
    	mErrRet( "Cannot go back to previous model" );
    
    doWholeWork();
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
	BufferString msg ("This will overwrite your depth/time model, do you want to continue?");
	return uiMSG().askOverwrite(msg);
    }
}



