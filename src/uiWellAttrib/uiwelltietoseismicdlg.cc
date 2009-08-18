/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiwelltietoseismicdlg.cc,v 1.45 2009-08-18 07:49:16 cvsbruno Exp $";

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

static const char*  eventtypes[] = { "None","Extrema","Maxima",
				     "Minima","Zero-crossings",0 };
#define mErrRet(msg) \
{ uiMSG().error(msg); return false; }
uiWellTieToSeismicDlg::uiWellTieToSeismicDlg( uiParent* p, 
					      const WellTieSetup& wts,
					      const Attrib::DescSet& ads )
	: uiFlatViewMainWin(p,uiFlatViewMainWin::Setup("")
						.withhanddrag(true)
						.deleteonclose(true))
    	, setup_(WellTieSetup(wts))
	, wd_(Well::MGR().get(wts.wellid_))
	, dataplayer_(0)
	, datadrawer_(0)
    	, controlview_(0)
    	, infodlg_(0)
	, params_(0)       		 
{
    setTitle( ads );
    viewer().setHandDrag( true );
    
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
	    		mCB(this,uiWellTieToSeismicDlg,compute) );
    eventstretcher_->timeChanged.notify(
	    		mCB(this,uiWellTieToSeismicDlg,timeChanged) );
    eventstretcher_->pickadded.notify(
	    		mCB(this,uiWellTieToSeismicDlg,checkIfPick) );
    initAll();
}


uiWellTieToSeismicDlg::~uiWellTieToSeismicDlg()
{
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
	    		mCB(this,uiWellTieToSeismicDlg,compute) );
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
    dispPropChg( 0 );
}


void uiWellTieToSeismicDlg::displayUserMsg( CallBacker* )
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


bool uiWellTieToSeismicDlg::doWork( CallBacker* )
{
    if ( !compute(0) )
	return false;
    drawData();
    return true;
}


bool uiWellTieToSeismicDlg::compute( CallBacker* )
{
    getDispParams();
    if ( !params_->resetParams() )
	 mErrRet( "unable to handle log data, please check your input logs" );
    if ( !dataplayer_->computeAll() )
	mErrRet( "unable to compute data, please check your input data" ); 
    resetInfoDlg();
    return true;
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
    disppropgrp->attach( ensureLeftOf, vwrtaskgrp );
    disppropgrp->attach( ensureBelow, viewer() );
    disppropgrp->attach( ensureBelow, logsdisp_[0] );
    disppropgrp->attach( ensureBelow, logsdisp_[1] );
    disppropgrp->attach( leftBorder );
    createDispPropFields( disppropgrp );

    uiSeparator* horSepar = new uiSeparator( this );
    horSepar->attach( stretchedBelow, disppropgrp );
    horSepar->attach( ensureBelow, vwrtaskgrp );

    uiPushButton* okbut = new uiPushButton( this, "&Ok/Save",
	      		mCB(this,uiWellTieToSeismicDlg,acceptOK), true );
    okbut->attach( leftBorder, 40 );
    okbut->attach( ensureBelow, horSepar );

    uiPushButton* infobut = new uiPushButton( this, "Info",
	      		mCB(this,uiWellTieToSeismicDlg,displayUserMsg), false );
    infobut->attach( hCentered );
    infobut->attach( ensureBelow, horSepar );
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
    
    infobut_ = new uiPushButton( taskgrp, "Display additional information",
	               mCB(this,uiWellTieToSeismicDlg,infoPushed), false );
    infobut_->attach( ensureBelow, applybut_ );
    infobut_->attach( hCentered );

}


void uiWellTieToSeismicDlg::createDispPropFields( uiGroup* dispgrp )
{
    dispgrp->setHSpacing( 50 );
    cscorrfld_ = new uiCheckBox( dispgrp, "use checkshot corrections" );
    cscorrfld_->display( params_->uipms_.iscsavailable_ );

    csdispfld_ = new uiCheckBox( dispgrp, "display checkshot related curve" );
    csdispfld_->display( params_->uipms_.iscsavailable_ );

    zinftfld_ = new uiCheckBox( dispgrp, "Z in feet" );
    zinftfld_ ->attach( rightOf, csdispfld_);

    zintimefld_ = new uiCheckBox( dispgrp, "Z in time" );
    zintimefld_ ->attach( alignedAbove, zinftfld_ );
    
    markerfld_ = new uiCheckBox( dispgrp, "display Markers" );
    markerfld_->attach( rightOf, zinftfld_ );
    markerfld_->display( wd_->haveMarkers() );

    putDispParams();

    const CallBack pccb( mCB(this,uiWellTieToSeismicDlg,dispPropChg) );
    cscorrfld_->activated.notify(mCB(this,uiWellTieToSeismicDlg,csCorrChanged));
    cscorrfld_->activated.notify( pccb );
    csdispfld_->activated.notify( pccb );
    markerfld_->activated.notify( pccb );
    zinftfld_->activated.notify( pccb );
    zintimefld_->activated.notify( pccb );

    zinftfld_->setChecked( SI().depthsInFeetByDefault() );
}


void uiWellTieToSeismicDlg::getDispParams()
{
    WellTieParams::uiParams* uipms = dataholder_->uipms();
    WellTieParams::DataParams* dpms = dataholder_->dpms();
    uipms->iscscorr_ = cscorrfld_->isChecked();
    uipms->iscsdisp_ = csdispfld_->isChecked();
    uipms->iszinft_ = zinftfld_->isChecked();
    uipms->iszintime_ = zintimefld_->isChecked();
    uipms->ismarkerdisp_ = markerfld_->isChecked();
}


void uiWellTieToSeismicDlg::putDispParams()
{
    WellTieParams::uiParams* pms = dataholder_->uipms();
    csdispfld_->setChecked( pms->iscsdisp_ );
    cscorrfld_->setChecked( pms->iscscorr_ );
    markerfld_->setChecked( pms->ismarkerdisp_ );
    zinftfld_->setChecked( pms->iszinft_ );
    zintimefld_->setChecked( pms->iszintime_ );
}


void uiWellTieToSeismicDlg::dispPropChg( CallBacker* )
{
    getDispParams();
    zinftfld_->display( !dataholder_->uipms()->iszintime_ );
    if ( !datadrawer_->isEmpty() )
	drawData();
}


void uiWellTieToSeismicDlg::timeChanged( CallBacker* )
{
    applybut_->setSensitive( true );
}


void uiWellTieToSeismicDlg::csCorrChanged( CallBacker* )
{
    getDispParams();
    WellTieParams::uiParams* pms = dataholder_->uipms();
    params_->resetVellLognm();
    if ( pms->iscscorr_ )
	dataplayer_->computeD2TModel();
    else  
	dataplayer_->undoD2TModel();
    if ( wd_->haveD2TModel() && wd_->d2TModel()->size()<3 )
	dataplayer_->computeD2TModel();

    doWork(0);
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
    controlview_->setEditOn( true );
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
    clearPicks( 0 );
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
    float startdah = wd_->d2TModel()->getDepth( 0 );
    float stopdah = wd_->track().dah( wd_->track().size()-1 );

    if ( topmrkfld_->getIntValue() == botmrkfld_->getIntValue() )
	return false;

    const bool zinft = SI().depthsInFeetByDefault();
    const Well::Marker* topmarkr = wd_->markers().getByName(topmrkfld_->text());
    const Well::Marker* botmarkr = wd_->markers().getByName(botmrkfld_->text());

    if ( topmarkr )
	startdah = topmarkr->dah();

    if ( botmarkr )
	stopdah = botmarkr->dah();

    if ( startdah > stopdah )
    { float tmp; mSWAP( startdah, stopdah, tmp ); }
    
    params_->corrstartdah_ = startdah;
    params_->corrstopdah_  = stopdah;

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

