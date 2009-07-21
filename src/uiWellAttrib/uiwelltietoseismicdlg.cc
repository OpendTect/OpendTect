/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiwelltietoseismicdlg.cc,v 1.39 2009-07-21 08:58:33 cvsbert Exp $";

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
#include "uiioobjsel.h"
#include "uicombobox.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "uiseparator.h"
#include "uitoolbar.h"
#include "uiwelldlgs.h"
#include "uiwelllogdisplay.h"
#include "uiworld2ui.h"

#include "arrayndimpl.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "ctxtioobj.h"
#include "survinfo.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "welllog.h"
#include "welllogset.h"
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

static bool isfirstdisplay_ = true;
static const char*  eventtypes[] = {"None","Extrema","Maxima",
				    "Minima","Zero-crossings",0};
#define mErrRet(msg) \
{ uiMSG().error(msg); return false; }
uiWellTieToSeismicDlg::uiWellTieToSeismicDlg( uiParent* p, 
					      const WellTieSetup& wts,
					      const Attrib::DescSet& ads )
	: uiFlatViewMainWin(p,uiFlatViewMainWin::Setup("")
						.deleteonclose(false))
    	, setup_(WellTieSetup(wts))
	, wd_(Well::MGR().get(wts.wellid_))
	, dataplayer_(0)
	, datadrawer_(0)
    	, controlview_(0)
    	, infodlg_(0)
	, params_(0)       		 
{
    setTitle( ads );
    
    for ( int idx=0; idx<2; idx++ )
    { 
	uiWellLogDisplay::Setup wldsu; wldsu.nrmarkerchars(3).border(5);
	logsdisp_ += new uiWellLogDisplay( this, wldsu );
    }

    uiTaskRunner* tr = new uiTaskRunner( p );
    params_ 	     = new WellTieParams( setup_, wd_, ads );
    dataholder_      = new WellTieDataHolder( params_, wd_, setup_ );
    infodlg_ 	     = new uiWellTieInfoDlg( this, dataholder_ );
    dataplayer_      = new WellTieToSeismic( dataholder_, ads, tr );
    datadrawer_      = new uiWellTieView( this, &viewer(), 
	    				  dataholder_, &logsdisp_ );
    eventstretcher_  = new uiWellTieEventStretch( this, dataholder_,
	    					  *datadrawer_ );
    infodlg_->applyPushed.notify(
	    		mCB(this,uiWellTieToSeismicDlg,doCrossCheckWork) );
    eventstretcher_->timeChanged.notify(
	    		mCB(this,uiWellTieToSeismicDlg,timeChanged) );
    eventstretcher_->pickadded.notify(
	    		mCB(this,uiWellTieToSeismicDlg,checkIfPick) );
    initAll();
}


uiWellTieToSeismicDlg::~uiWellTieToSeismicDlg()
{
    clearPicks(0);
    if ( eventstretcher_ )
    {
	eventstretcher_->timeChanged.remove(
			    mCB(this,uiWellTieToSeismicDlg,timeChanged) );
	eventstretcher_->pickadded.remove(
			    mCB(this,uiWellTieToSeismicDlg,checkIfPick) );
	delete eventstretcher_;
    }
    if ( infodlg_ )
    {
	infodlg_->applyPushed.remove(
	    		mCB(this,uiWellTieToSeismicDlg,doCrossCheckWork) );
	delete infodlg_;
    }
    if ( datadrawer_ )     delete datadrawer_;
    if ( dataplayer_ )     delete dataplayer_;
    if ( dataholder_ ) 	   delete dataholder_;
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
    infodlg_->setUserDepths();
    doWork( 0 );
    dataholder_->pickmgr()->setDataParams( dataholder_->dpms() );
    dataholder_->pickmgr()->setData( dataholder_->dispData() );
    show();
    displayUserMsg();
}


void uiWellTieToSeismicDlg::displayUserMsg()
{
    if ( isfirstdisplay_ )
    {
	BufferString msg = "To correlate synthetic to seimsic, "; 
	msg += "pick one or more synthetic events "; 
	msg += "and link them with the seismic events. "; 
	msg += "Each synthetic event must be coupled with a seismic event. "; 
	msg += "Once you are satisfied with the picking, push the ";
        msg += "'Apply Changes' button to re-extract the data and repeat "; 
	msg += "the picking operation if needed.";

	isfirstdisplay_ = uiMSG().showMsgNextTime( msg );
    }
}


void uiWellTieToSeismicDlg::doWork( CallBacker* )
{
    getDispParams();
    params_->resetParams();
    dataplayer_->computeAll();
    drawData();
    resetInfoDlg();
}


void uiWellTieToSeismicDlg::doCrossCheckWork( CallBacker* )
{
    getDispParams();
    params_->resetParams();
    dataplayer_->computeAll();
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
    controlview_->setSelView( false );
}


#define mAddButton(pm,func,tip) \
    toolbar_->addButton( pm, mCB(this,uiWellTieToSeismicDlg,func), tip )
void uiWellTieToSeismicDlg::addToolBarTools()
{
    toolbar_ = new uiToolBar( this, "Well Tie Control", uiToolBar::Right ); 
    mAddButton( "z2t.png", editD2TPushed, "View/Edit Model" );
    mAddButton( "saveflow.png",saveD2TPushed, "Save Model" );
}    


void uiWellTieToSeismicDlg::addControl()
{
    addToolBarTools();
    controlview_ = new uiWellTieControlView( this, toolbar_, &viewer() );
    controlview_->setPickSetMGR( dataholder_->pickmgr() );
}


void uiWellTieToSeismicDlg::drawFields()
{
    uiGroup* vwrtaskgrp = new uiGroup( this, "task group" );
    vwrtaskgrp->attach( alignedBelow, viewer() );
    vwrtaskgrp->attach( rightBorder );
    createViewerTaskFields( vwrtaskgrp );

    uiGroup* disppropgrp = new uiGroup( this, "Display Properties group" );
    disppropgrp->attach( ensureBelow, viewer() );
    disppropgrp->attach( ensureBelow, logsdisp_[0] );
    disppropgrp->attach( ensureBelow, logsdisp_[1] );
    disppropgrp->attach( leftBorder );
    createDispPropFields( disppropgrp );

    uiGroup* informgrp = new uiGroup( this, "Indicator Group" );
    informgrp->attach( ensureBelow, disppropgrp );
    informgrp->attach( hCentered );
    infobut_ = new uiPushButton( informgrp, "Display additional information",
	               mCB(this,uiWellTieToSeismicDlg,infoPushed), false );

    uiSeparator* horSepar = new uiSeparator( this );
    horSepar->attach( stretchedBelow, informgrp );

    uiPushButton* okbut = new uiPushButton( this, "&Ok/Save",
	      		mCB(this,uiWellTieToSeismicDlg,acceptOK), true );
    okbut->attach( leftBorder, 40 );
    okbut->attach( ensureBelow, horSepar );
    uiPushButton* cancelbut = new uiPushButton( this, "&Cancel",
	      		mCB(this,uiWellTieToSeismicDlg,rejectOK), true );
    cancelbut->attach( rightBorder );
    cancelbut->attach( ensureBelow, horSepar );
}


void uiWellTieToSeismicDlg::createViewerTaskFields( uiGroup* taskgrp )
{
    eventtypefld_ = new uiLabeledComboBox( taskgrp, "Track" );
    for ( int idx=0; eventtypes[idx]; idx++)
	eventtypefld_->box()->addItem( eventtypes[idx] );
    eventtypefld_->box()->selectionChanged.
	notify(mCB(this,uiWellTieToSeismicDlg,eventTypeChg));

    applybut_ = new uiPushButton( taskgrp, "&Apply Changes",
	   mCB(this,uiWellTieToSeismicDlg,applyPushed), true );
    applybut_->setSensitive( false );
    applybut_->attach( rightOf, eventtypefld_ );

    undobut_ = new uiPushButton( taskgrp, "&Undo",
	   mCB(this,uiWellTieToSeismicDlg,undoPushed), true );
    undobut_->attach( rightOf, applybut_ );
    undobut_->setSensitive( false );
    
    clearpicksbut_ = new uiPushButton( taskgrp, "&Clear picks",
	   mCB(this,uiWellTieToSeismicDlg,clearPicks), true );
    clearpicksbut_->setSensitive( false );
    clearpicksbut_->attach( rightOf, undobut_ );
    
    clearlastpicksbut_ = new uiPushButton( taskgrp, "&Clear last pick",
	   mCB(this,uiWellTieToSeismicDlg,clearLastPick), true );
    clearlastpicksbut_->setSensitive( false );
    clearlastpicksbut_->attach( rightOf, clearpicksbut_ );
}


void uiWellTieToSeismicDlg::createDispPropFields( uiGroup* dispgrp )
{
    cscorrfld_ = new uiCheckBox( dispgrp, "use checkshot corrections" );
    cscorrfld_->display( params_->uipms_.iscscorr_ );

    csdispfld_ = new uiCheckBox( dispgrp, "display checkshot related curve" );
    csdispfld_->display(params_->uipms_.iscscorr_);

    zinftfld_ = new uiCheckBox( dispgrp, "Z in feet" );
    zinftfld_ ->attach( rightOf, csdispfld_);
    
    markerfld_ = new uiCheckBox( dispgrp, "display Markers" );
    markerfld_->attach( alignedAbove, zinftfld_ );

    putDispParams();

    const CallBack pccb( mCB(this,uiWellTieToSeismicDlg,dispPropChg) );
    cscorrfld_->activated.notify(mCB(this,uiWellTieToSeismicDlg,doWork));
    cscorrfld_->activated.notify( pccb );
    csdispfld_->activated.notify( pccb );
    markerfld_->activated.notify( pccb );
    zinftfld_->activated.notify( pccb );

    zinftfld_->setChecked( SI().depthsInFeetByDefault() );
}


void uiWellTieToSeismicDlg::getDispParams()
{
    WellTieParams::uiParams* uipms = dataholder_->uipms();
    WellTieParams::DataParams* dpms = dataholder_->dpms();
    uipms->iscscorr_ = cscorrfld_->isChecked();
    uipms->iscsdisp_ = csdispfld_->isChecked();
    uipms->iszinft_ = zinftfld_->isChecked();
    uipms->ismarkerdisp_ = markerfld_->isChecked();
}


void uiWellTieToSeismicDlg::putDispParams()
{
    WellTieParams::uiParams* pms = dataholder_->uipms();
    csdispfld_->setChecked( pms->iscsdisp_ );
    cscorrfld_->setChecked( pms->iscscorr_ );
    markerfld_->setChecked( pms->ismarkerdisp_ );
    zinftfld_->setChecked( pms->iszinft_ );
}


void uiWellTieToSeismicDlg::dispPropChg( CallBacker* )
{
    getDispParams();
    if ( !datadrawer_->isEmpty() )
	drawData();
}


void uiWellTieToSeismicDlg::timeChanged( CallBacker* )
{
    applybut_->setSensitive( true );
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
	

void uiWellTieToSeismicDlg::eventTypeChg( CallBacker* )
{
    dataholder_->pickmgr()->setEventType(eventtypefld_->box()->currentItem());
}


void uiWellTieToSeismicDlg::applyPushed( CallBacker* cb )
{
    eventstretcher_->doWork(0);
    doWork( cb );
    clearPicks(0);
    applybut_->setSensitive( false );
    undobut_->setSensitive( true );
}


void uiWellTieToSeismicDlg::clearPicks( CallBacker* cb )
{
    dataholder_->pickmgr()->clearAllPicks();
    datadrawer_->drawUserPicks();
    checkIfPick( cb );
}


void uiWellTieToSeismicDlg::clearLastPick( CallBacker* cb )
{
    dataholder_->pickmgr()->clearLastPicks();
    datadrawer_->drawUserPicks();
    checkIfPick( cb );
}


void uiWellTieToSeismicDlg::checkIfPick( CallBacker* )
{
    const bool ispick = dataholder_->pickmgr()->isPick();
    const bool issamesz = dataholder_->pickmgr()->isSameSize();
    clearpicksbut_->setSensitive( ispick );
    clearlastpicksbut_->setSensitive( ispick );
    applybut_->setSensitive( ispick && issamesz );
}


bool uiWellTieToSeismicDlg::undoPushed( CallBacker* cb )

{
    if ( !dataplayer_->undoD2TModel() )
    	mErrRet( "Cannot go back to previous model" );
    clearPicks(0);
    doWork( cb );
    
    undobut_->setSensitive( false );
    applybut_->setSensitive( false );
    
    return true;	    
}


bool uiWellTieToSeismicDlg::rejectOK( CallBacker* )
{
    dataplayer_->cancelD2TModel();
    close();
    return true;
}


bool uiWellTieToSeismicDlg::acceptOK( CallBacker* )
{
    BufferString msg("This will overwrite your depth/time model, do you want to continue?");
    if ( uiMSG().askOverwrite(msg) )
    {
       if ( !dataplayer_->commitD2TModel() )
	    mErrRet("Cannot write new depth/time model")
	close();
    }
    return false;
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
    
    uiGroup* viewersgrp = new uiGroup( this, "Viewers group" );
    uiGroup* wvltgrp = new uiGroup( viewersgrp, "wavelet group" );
    wvltdraw_ = new uiWellTieWaveletView( wvltgrp, dataholder_ );

    uiGroup* corrgrp = new uiGroup( viewersgrp, "CrossCorrelation group" );
    corrgrp->attach( rightOf, wvltgrp );
    crosscorr_ = new uiWellTieCorrView( corrgrp, dataholder_ );

    uiSeparator* horSepar = new uiSeparator( this );
    horSepar->attach( stretchedAbove, viewersgrp );

    uiGroup* markergrp =  new uiGroup( this, "User Position Group" );
    markergrp->attach( centeredAbove, horSepar );

    markernames_.add( Well::TrackSampler::sKeyDataStart() );
    if ( wd_->haveMarkers() )
    {
	for ( int idx=0; idx<wd_->markers().size(); idx++)
	    markernames_.add( wd_->markers()[idx]->name() );
    }
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
    if ( crosscorr_ ) delete crosscorr_;
    if ( wvltdraw_ ) delete wvltdraw_;
}


void uiWellTieInfoDlg::applyMarkerPushed( CallBacker* )
{
    applymrkbut_->setSensitive( false );
    applyPushed.trigger();
}


bool uiWellTieInfoDlg::setUserDepths()
{
    float start = wd_->d2TModel()->getDepth( 0 );
    float stop = wd_->track().dah( wd_->track().size()-1 );

    if ( topmrkfld_->getIntValue() == botmrkfld_->getIntValue() )
	return false;

    const Well::Marker* topmarkr = wd_->markers().getByName(topmrkfld_->text());
    const Well::Marker* botmarkr = wd_->markers().getByName(botmrkfld_->text());

    if ( topmarkr )
	start = topmarkr->dah();

    if ( botmarkr )
	stop = botmarkr->dah();

    if ( start > stop )
    { float tmp; mSWAP( start, stop, tmp ); }
    
    params_->corrstartdah_ = start;
    params_->corrstopdah_  = stop;

    return true;
}


void uiWellTieInfoDlg::userDepthsChanged( CallBacker* )
{
    applymrkbut_->setSensitive(true);
    setUserDepths();
}


void uiWellTieInfoDlg::setXCorrel()
{
    crosscorr_->setCrossCorrelation();
}


void uiWellTieInfoDlg::setWvlts()
{
    wvltdraw_->initWavelets();
}

