/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiwelltietoseismicdlg.cc,v 1.18 2009-06-18 07:41:52 cvsbruno Exp $";

#include "uiwelltietoseismicdlg.h"
#include "uiwelltiecontrolview.h"
#include "uiwelltieeventstretch.h"
#include "uiwelltiestretch.h"
#include "uiwelltieview.h"
#include "uiwelltiewavelet.h"

#include "uibutton.h"
#include "uifiledlg.h"
#include "uiflatviewer.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "uiseparator.h"
#include "uitoolbar.h"
#include "uiwelldlgs.h"
#include "uiwelllogdisplay.h"

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
	: uiFlatViewMainWin(p,uiFlatViewMainWin::Setup("")
						.menubar(true)
						.deleteonclose(false))
    	, setup_(WellTieSetup(wts))
	, wd_(Well::MGR().get(wts.wellid_))
	, dataplayer_(0)
	, datadrawer_(0)
    	, controlview_(0)
	, params_(0)       		 
	, manip_(0)				 
{
    setTitle( ads );
   
    uiTaskRunner* tr = new uiTaskRunner( p );
    toolbar_ = new uiToolBar( this, "Well Tie Control" );
    
    for ( int idx=0; idx<2; idx++ )
    {
	uiWellLogDisplay::Setup lsu; lsu.border(5);
	logsdisp_ += new uiWellLogDisplay( this, lsu );
    }
    
    params_ 	    = new WellTieParams( setup_, wd_, ads );
    dataholder_     = new WellTieDataHolder( params_, wd_, setup_ );
    dataplayer_     = new WellTieToSeismic( dataholder_, ads, tr );
    datadrawer_     = new uiWellTieView( this, &viewer(), 
	    				 dataholder_, &logsdisp_ );
    infodlg_ 	    = new uiWellTieInfoDlg( this, dataholder_ );
    eventstretcher_ = new uiWellTieEventStretch( this, dataholder_,
	    					 *datadrawer_ );

    infodlg_->applyPushed.notify(
	    		mCB(this,uiWellTieToSeismicDlg,doWork) );
    eventstretcher_->dispdataChanged.notify(
	    		mCB(this,uiWellTieToSeismicDlg,dispDataChanged) );
    eventstretcher_->readyforwork.notify(
	    		mCB(this,uiWellTieToSeismicDlg,applyReady) );
    eventstretcher_->pickadded.notify(
	    		mCB(this,uiWellTieToSeismicDlg,checkIfPick) );
   
    initAll();
}


uiWellTieToSeismicDlg::~uiWellTieToSeismicDlg()
{
    if ( eventstretcher_ )
    {
	eventstretcher_->readyforwork.remove(
			    mCB(this,uiWellTieToSeismicDlg,applyReady) );
	eventstretcher_->dispdataChanged.remove(
			    mCB(this,uiWellTieToSeismicDlg,dispDataChanged) );
	eventstretcher_->pickadded.remove(
			    mCB(this,uiWellTieToSeismicDlg,checkIfPick) );
    }

    if ( datadrawer_ )     delete datadrawer_;
    if ( dataholder_ ) 	   delete dataholder_;
    if ( eventstretcher_ ) delete eventstretcher_;
    if ( dataplayer_ )     delete dataplayer_;
    if ( infodlg_ )  	   delete infodlg_; 
}


void uiWellTieToSeismicDlg::setTitle( const Attrib::DescSet& ads )
{
    const Attrib::Desc* ad = ads.getDesc( setup_.attrid_ );
    if ( !ad ) return;

    BufferString attrnm = ad->userRef();

    BufferString wname = "Tie ";
    wname += wd_->name();
    wname += " to ";
    wname += attrnm;   
    
    setWinTitle( wname );
}


void uiWellTieToSeismicDlg::initAll()
{
    drawFields();
    addControl();
    addToolBarTools();
    infodlg_->setUserDepths();
    putDispParams();
    doWork( 0 );
}


void uiWellTieToSeismicDlg::doWork( CallBacker* )
{
    dataplayer_->computeAll();
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
}


#define mAddButton(pm,func,tip) \
    toolbar_->addButton( pm, mCB(this,uiWellTieToSeismicDlg,func), tip )
void uiWellTieToSeismicDlg::addToolBarTools()
{
    mAddButton( "z2t.png", editD2TPushed, "View/Edit Model" );
    mAddButton( "saveflow.png",saveD2TPushed, "Save Model" );
}    


void uiWellTieToSeismicDlg::addControl()
{
    controlview_ = new uiWellTieControlView( this, toolbar_, &viewer() );
    controlview_->setPickSetMGR( dataholder_->pickmgr() );
}


void uiWellTieToSeismicDlg::drawFields()
{
    uiGroup* taskgrp = new uiGroup( this, "task group" );
    taskgrp->attach( ensureBelow, viewer() );
    taskgrp->attach( rightBorder );
    createTaskFields( taskgrp );

    uiGroup* disppropgrp = new uiGroup( this, "Display Properties group" );
    disppropgrp->attach( ensureBelow, viewer() );
    disppropgrp->attach( ensureBelow, logsdisp_[0] );
    disppropgrp->attach( ensureBelow, logsdisp_[1] );
    disppropgrp->attach( leftBorder );
    createDispPropFields( disppropgrp );

    uiGroup* informgrp = new uiGroup( this, "Indicator Group" );
    informgrp->attach( ensureBelow, disppropgrp );
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
    
    clearpicksbut_ = new uiPushButton( taskgrp, "&Clear picks",
	   mCB(this,uiWellTieToSeismicDlg,clearPicksPushed), true );
    clearpicksbut_->setSensitive( false );
    clearpicksbut_->attach( ensureRightOf, undobut_ );
}


void uiWellTieToSeismicDlg::createDispPropFields( uiGroup* dispgrp )
{
    cscorrfld_ = new uiCheckBox( dispgrp, "use checkshot corrections" );
    //cscorrfld_->setChecked(params_->iscscorr_);
    cscorrfld_->activated.notify(mCB(this,uiWellTieToSeismicDlg,doWork));

    csdispfld_ = new uiCheckBox( dispgrp, "display checkshot related curve" );
    //csdispfld_->setChecked(params_->iscsdisp_);
    csdispfld_->attach( alignedBelow, cscorrfld_ );

    zinftfld_ = new uiCheckBox( dispgrp, "Z in feet" );
    zinftfld_ ->attach( rightOf, csdispfld_);
    
    markerfld_ = new uiCheckBox( dispgrp, "display Markers" );
    markerfld_->attach( alignedAbove, zinftfld_ );


    putDispParams();

    cscorrfld_->activated.notify(mCB(this,uiWellTieToSeismicDlg,dispPropChg));
    csdispfld_->activated.notify(mCB(this,uiWellTieToSeismicDlg,dispPropChg));
    markerfld_->activated.notify(mCB(this,uiWellTieToSeismicDlg,dispPropChg));
    zinftfld_->activated.notify(mCB(this,uiWellTieToSeismicDlg,dispPropChg));
    
    setPrefWidthInChar( 60 );
}


void uiWellTieToSeismicDlg::dispDataChanged( CallBacker* )
{
    dataplayer_->setd2TModelFromData();
    applybut_->setSensitive( true );
}

void uiWellTieToSeismicDlg::applyReady( CallBacker* )
{
    if ( manip_ )
	manip_ = 0;
    else
	manip_ = 1;
    applybut_->setSensitive( manip_ ? true : false );
}


void uiWellTieToSeismicDlg::getDispParams()
{
    WellTieParams::uiParams* pms = dataholder_->uipms();
    pms->iscscorr_ = cscorrfld_->isChecked();
    pms->iscsdisp_ = csdispfld_->isChecked();
    pms->iszinft_ = zinftfld_->isChecked();
    dataholder_->dpms()->currvellognm_ = pms->iscscorr_ ? 
			setup_.corrvellognm_ : setup_.vellognm_;
}


void uiWellTieToSeismicDlg::putDispParams()
{
    WellTieParams::uiParams* pms = dataholder_->uipms();
    csdispfld_->setChecked( pms->iscsdisp_ );
    cscorrfld_->setChecked( pms->iscscorr_ );
    markerfld_->setChecked( pms->ismarker_ );
    zinftfld_->setChecked( pms->iszinft_ );
}


void uiWellTieToSeismicDlg::dispPropChg( CallBacker* )
{
    getDispParams();
    //drawData();
}


void uiWellTieToSeismicDlg::infoPushed( CallBacker* )
{
    infodlg_->go();
}


void uiWellTieToSeismicDlg::editD2TPushed( CallBacker* )
{
    uiD2TModelDlg d2tmdlg( this, *wd_, false );
    if ( d2tmdlg.go() )
    {
	applybut_->setSensitive( true );
	undobut_ ->setSensitive( true );
    }
}


bool uiWellTieToSeismicDlg::saveD2TPushed( CallBacker* )
{
    uiFileDialog* uifiledlg =
	new uiFileDialog( this, false,"","", "Save depth/time table" );
    uifiledlg->setDirectory(IOObjContext::getDataDirName(IOObjContext::WllInf));
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
    params_->resetDataParams();
    eventstretcher_->doWork(0);
    doWork( cb );
    //dataholder_->pickmgr_->clearAllPicks();
    applybut_->setSensitive( false );
    undobut_->setSensitive( true );
    manip_ = 0;
}


void uiWellTieToSeismicDlg::clearPicksPushed( CallBacker* )
{
    dataholder_->pickmgr()->clearAllPicks();
    drawData();
    checkIfPick();
}


void uiWellTieToSeismicDlg::checkIfPick()
{
    bool ispick = dataholder_->pickmgr()->checkIfPick();
    clearpicksbut_->setSensitive( ispick );
    applybut_->setSensitive( ispick );
}


bool uiWellTieToSeismicDlg::undoPushed( CallBacker* cb )
{
    if ( !dataplayer_->undoD2TModel() )
    	mErrRet( "Cannot go back to previous model" );
    doWork( cb );
    
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




uiWellTieInfoDlg::uiWellTieInfoDlg( uiParent* p, WellTieDataHolder* dh )
	: uiDialog(p,uiDialog::Setup("Cross-checking parameters", "",
				     mTODOHelpID).modal(false))
	, dataholder_(dh)
	, wd_(dh->wd())		 
	, params_(dataholder_->dpms()) 
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
			mCB(this,uiWellTieInfoDlg,userDepthsChanged));
    
    botmrkfld_ = new uiGenInput( markergrp, "", slis.setName("Bottom Marker") );
    botmrkfld_->attach( rightOf, topmrkfld_ );
    botmrkfld_->setValue( markernames_.size()-1 );
    botmrkfld_->setElemSzPol( uiObject::Medium );
    botmrkfld_->valuechanged.notify(
			mCB(this,uiWellTieInfoDlg,userDepthsChanged));
    
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
