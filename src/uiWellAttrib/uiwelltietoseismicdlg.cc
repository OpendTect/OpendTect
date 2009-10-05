/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiwelltietoseismicdlg.cc,v 1.57 2009-10-05 15:35:27 cvsbruno Exp $";

#include "uiwelltietoseismicdlg.h"
#include "uiwelltiecontrolview.h"
#include "uiwelltieeventstretch.h"
#include "uiwelltiestretch.h"
#include "uiwelltieview.h"
#include "uiwelltiewavelet.h"
#include "uiwelltiesavedatadlg.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uiflatviewer.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "uiseparator.h"
#include "uitoolbar.h"
#include "uiwelldlgs.h"
#include "uiwelllogdisplay.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "ctxtioobj.h"
#include "pixmap.h"
#include "survinfo.h"
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
#include "welltietoseismic.h"
#include "welltieunitfactors.h"

namespace WellTie
{

static const char*  helpid = "107.4.1";
static const char*  eventtypes[] = { "None","Extrema","Maxima",
				     "Minima","Zero-crossings",0 };
#define mErrRet(msg) \
{ uiMSG().error(msg); return false; }
uiTieWin::uiTieWin( uiParent* p, const WellTie::Setup& wts, 
		    const Attrib::DescSet& ads )
	: uiFlatViewMainWin(p,uiFlatViewMainWin::Setup("")
			    .withhanddrag(true)
			    .deleteonclose(false))
    	, setup_(WellTie::Setup(wts))
	, wd_(Well::MGR().get(wts.wellid_))
    	, controlview_(0)
	, datadrawer_(0)
	, dataholder_(0)
	, dataplayer_(0)
	, stretcher_(0)					
    	, infodlg_(0)
	, params_(0)       		 
{
    setTitle( ads );
    
    for ( int idx=0; idx<2; idx++ )
    { 
	uiWellLogDisplay::Setup wldsu; wldsu.nrmarkerchars(3);
	wldsu.border_.setLeft(0); wldsu.border_.setRight(0);
	logsdisp_ += new uiWellLogDisplay( this, wldsu );
    }

    uiTaskRunner* tr = new uiTaskRunner( p );
    params_	= new WellTie::Params( setup_, wd_, ads );
    dataholder_ = new WellTie::DataHolder( params_, wd_, setup_ );
    dataplayer_ = new WellTie::DataPlayer( dataholder_, ads, tr );
    infodlg_    = new WellTie::uiInfoDlg( this, dataholder_, dataplayer_ );
    datadrawer_ = new WellTie::uiTieView( this, &viewer(), *dataholder_, 
	    				&logsdisp_ );
    stretcher_  = new WellTie::uiEventStretch( this,dataholder_,*datadrawer_ );

    infodlg_->redrawNeeded.notify( mCB(datadrawer_,uiTieView,redrawViewer) );
    stretcher_->pickadded.notify( mCB(this,uiTieWin,checkIfPick) );
    stretcher_->timeChanged.notify(mCB(this,uiTieWin,timeChanged));
    
    initAll();
}


uiTieWin::~uiTieWin()
{
    stretcher_->timeChanged.remove(mCB(this,uiTieWin,timeChanged));
    stretcher_->pickadded.remove(mCB(this,uiTieWin,checkIfPick));
    infodlg_->redrawNeeded.remove( mCB(datadrawer_,uiTieView,redrawViewer) );
    
    delete stretcher_;
    delete infodlg_;
    delete datadrawer_;
    delete dataplayer_;
    delete dataholder_;

    delete  Well::MGR().release( setup_.wellid_ );
}


void uiTieWin::setTitle( const Attrib::DescSet& ads )
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


void uiTieWin::initAll()
{
    drawFields();
    addControl();
    doWork( 0 );
    dataholder_->pickmgr()->setData( dataholder_ );
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


bool uiTieWin::doWork( CallBacker* )
{
    getDispParams();

    if ( !params_->resetParams() )
	 mErrRet( "unable to handle log data, please check your input logs" );
    if ( !dataplayer_->computeAll() )
	mErrRet( "unable to compute data, please check your input data" ); 

    drawData();
    return true;
}


void uiTieWin::drawData()
{
    const bool viewall = controlview_->isZoomAtStart();
    datadrawer_->fullRedraw();
    controlview_->setSelView( false, viewall );
    infodlg_->drawData();
}


#define mAddButton(pm,func,tip) \
    toolbar_->addButton( pm, mCB(this,uiTieWin,func), tip )
void uiTieWin::addToolBarTools()
{
    toolbar_ = new uiToolBar( this, "Well Tie Control", uiToolBar::Right ); 
    mAddButton( "z2t.png", editD2TPushed, "View/Edit Model" );
    mAddButton( "save.png", saveDataPushed, "Save Data" );
}    


void uiTieWin::addControl()
{
    addToolBarTools();
    controlview_ = new WellTie::uiControlView( this, toolbar_, &viewer() );
    controlview_->setPickSetMGR( dataholder_->pickmgr() );
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
    disppropgrp->attach( ensureBelow, logsdisp_[0] );
    disppropgrp->attach( ensureBelow, logsdisp_[1] );
    disppropgrp->attach( leftBorder );
    createDispPropFields( disppropgrp );

    uiSeparator* horSepar = new uiSeparator( this );
    horSepar->attach( stretchedBelow, disppropgrp );
    horSepar->attach( ensureBelow, vwrtaskgrp );

    uiPushButton* okbut = new uiPushButton( this, "&Ok/Save",
	      		mCB(this,uiTieWin,acceptOK), true );
    okbut->attach( leftBorder, 40 );
    okbut->attach( ensureBelow, horSepar );

    uiPushButton* infobut = new uiPushButton( this, "Info",
	      		mCB(this,uiTieWin,displayUserMsg), false );
    infobut->attach( hCentered );
    infobut->attach( ensureBelow, horSepar );
    const ioPixmap pixmap( "contexthelp.png" );
    uiToolButton* helpbut = new uiToolButton( this, 0, pixmap,
			mCB(this,uiTieWin,provideWinHelp) );
    helpbut->setToolTip( helpid );
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

}


void uiTieWin::createDispPropFields( uiGroup* dispgrp )
{
    dispgrp->setHSpacing( 50 );
    cscorrfld_ = new uiCheckBox( dispgrp, "use checkshot corrections" );
    cscorrfld_->display( wd_->haveCheckShotModel() );

    csdispfld_ = new uiCheckBox( dispgrp, "display checkshot related curve" );
    csdispfld_->display( wd_->haveCheckShotModel() );

    zinftfld_ = new uiCheckBox( dispgrp, "Z in feet" );
    zinftfld_ ->attach( rightOf, csdispfld_);

    zintimefld_ = new uiCheckBox( dispgrp, "Z in time" );
    zintimefld_ ->attach( alignedAbove, zinftfld_ );
    
    markerfld_ = new uiCheckBox( dispgrp, "display Markers" );
    markerfld_->attach( rightOf, zintimefld_ );
    markerfld_->display( wd_->haveMarkers() );

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
    WellTie::Params::uiParams* pms = dataholder_->uipms();
    pms->iscscorr_ = cscorrfld_->isChecked();
    pms->iscsdisp_ = csdispfld_->isChecked();
    pms->iszinft_ = zinftfld_->isChecked();
    pms->iszintime_ = zintimefld_->isChecked();
    pms->ismarkerdisp_ = markerfld_->isChecked();
}


void uiTieWin::putDispParams()
{
    WellTie::Params::uiParams* pms = dataholder_->uipms();
    csdispfld_->setChecked( pms->iscsdisp_ );
    cscorrfld_->setChecked( pms->iscscorr_ );
    markerfld_->setChecked( pms->ismarkerdisp_ );
    zinftfld_->setChecked( pms->iszinft_ );
    zintimefld_->setChecked( pms->iszintime_ );
}


void uiTieWin::dispPropChg( CallBacker* )
{
    getDispParams();
    zinftfld_->display( !dataholder_->uipms()->iszintime_ );
    if ( !datadrawer_->isEmpty() )
	drawData();
}


void uiTieWin::timeChanged( CallBacker* )
{
    applybut_->setSensitive( true );
}


void uiTieWin::csCorrChanged( CallBacker* cb )
{
    getDispParams();
    WellTie::Params::uiParams* pms = dataholder_->uipms();
    params_->resetVelLogNm();
    if ( pms->iscscorr_ )
	dataplayer_->computeD2TModel();
    else  
	dataplayer_->undoD2TModel();
    if ( wd_->haveD2TModel() && wd_->d2TModel()->size()<3 )
	dataplayer_->computeD2TModel();

    doWork( cb );
}


void uiTieWin::infoPushed( CallBacker* )
{
    infodlg_->go();
}


void uiTieWin::editD2TPushed( CallBacker* cb )
{
    uiD2TModelDlg d2tmdlg( this, *wd_, false );
    if ( d2tmdlg.go() )
	doWork( cb );
}


void uiTieWin::saveDataPushed( CallBacker* cb )
{
    uiSaveDataDlg dlg( this, dataholder_ );
    dlg.go();
}


void uiTieWin::eventTypeChg( CallBacker* )
{
    dataholder_->pickmgr()->setEventType(eventtypefld_->box()->currentItem());
    controlview_->setEditOn( true );
}


void uiTieWin::applyPushed( CallBacker* cb )
{
    stretcher_->doWork( cb );
    doWork( cb );
    clearPicks( cb );
    applybut_->setSensitive( false );
    undobut_->setSensitive( true );
}


void uiTieWin::clearPicks( CallBacker* cb )
{
    dataholder_->pickmgr()->clearAllPicks();
    datadrawer_->drawUserPicks();
    checkIfPick( cb );
}


void uiTieWin::clearLastPick( CallBacker* cb )
{
    dataholder_->pickmgr()->clearLastPicks();
    datadrawer_->drawUserPicks();
    checkIfPick( cb );
}


void uiTieWin::checkIfPick( CallBacker* )
{
    const bool ispick = dataholder_->pickmgr()->isPick();
    const bool issamesz = dataholder_->pickmgr()->isSameSize();
    clearpicksbut_->setSensitive( ispick );
    clearlastpicksbut_->setSensitive( ispick );
    applybut_->setSensitive( ispick && issamesz );
}


bool uiTieWin::undoPushed( CallBacker* cb )
{
    if ( !dataplayer_->undoD2TModel() )
    	mErrRet( "Cannot go back to previous model" );
    clearPicks( cb );
    doWork( cb );
    
    undobut_->setSensitive( false );
    applybut_->setSensitive( false );
    
    return true;	    
}


bool uiTieWin::rejectOK( CallBacker* )
{
    dataplayer_->cancelD2TModel();
    close();
    return true;
}


bool uiTieWin::acceptOK( CallBacker* )
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




uiInfoDlg::uiInfoDlg( uiParent* p, WellTie::DataHolder* dh, 
		      WellTie::DataPlayer* dp )
	: uiDialog(p,uiDialog::Setup("Cross-checking parameters", "",
				     "107.4.2").modal(false))
	, dataholder_(dh)
	, dataplayer_(dp)		 
	, wd_(dh->wd())		 
	, params_(dataholder_->dpms()) 
      	, crosscorr_(0)	       
	, wvltdraw_(0)
	, redrawNeeded(this)
{
    setCtrlStyle( LeaveOnly );
    
    uiGroup* viewersgrp = new uiGroup( this, "Viewers group" );
    uiGroup* wvltgrp = new uiGroup( viewersgrp, "wavelet group" );
    uiGroup* corrgrp = new uiGroup( viewersgrp, "CrossCorrelation group" );

    wvltdraw_ = new WellTie::uiWaveletView( wvltgrp, dataholder_ );
    wvltdraw_->activeWvltChged.notify(mCB(this,WellTie::uiInfoDlg,wvltChanged));
    estwvltlengthfld_ = new uiGenInput(wvltgrp,"Estimated wavelet length (ms)");
    estwvltlengthfld_ ->attach( centeredBelow, wvltdraw_ );
    estwvltlengthfld_->valuechanged.notify( mCB(this,uiInfoDlg,propChanged) );
    estwvltlengthfld_->setValue( 
		    dataholder_->wvltset()[0]->samplePositions().width()*1000 );

    uiSeparator* verSepar = new uiSeparator( viewersgrp, false );
    verSepar->attach( rightTo, wvltgrp );

    corrgrp->attach( rightOf, verSepar );
    crosscorr_ = new WellTie::uiCorrView( corrgrp, *dataholder_ );

    uiSeparator* horSepar = new uiSeparator( this );
    horSepar->attach( stretchedAbove, viewersgrp );
    
    uiGroup* markergrp =  new uiGroup( this, "User Z Range Group" );
    markergrp->attach( centeredAbove, horSepar );

    const char* choice[] = { "Markers", "Times", "Depths", 0 };
    choicefld_ = new uiGenInput( markergrp, "Compute Data between", 
	    				StringListInpSpec(choice) );
    choicefld_->valuechanged.notify( mCB(this,uiInfoDlg,propChanged) );
    
    markernames_.add( Well::TrackSampler::sKeyDataStart() );
    if ( wd_->haveMarkers() )
    {
	for ( int idx=0; idx<wd_->markers().size(); idx++)
	    markernames_.add( wd_->markers()[idx]->name() );
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
    delete crosscorr_;
    wvltdraw_->activeWvltChged.remove(mCB(this,WellTie::uiInfoDlg,wvltChanged));
    wvltdraw_->activeWvltChged.remove(mCB(this,WellTie::uiInfoDlg,propChanged));
    delete wvltdraw_;
}


bool uiInfoDlg::getMarkerDepths( Interval<float>& zrg )
{
    zrg.start = wd_->d2TModel()->getDepth( 0 );
    zrg.stop = wd_->track().dah( wd_->track().size()-1 );

    const Interval<int> mintv = zrangeflds_[0]->getIInterval();
    const bool zinft = SI().depthsInFeetByDefault();
    const Well::Marker* topmarkr = 
			wd_->markers().getByName( zrangeflds_[0]->text(0) );
    const Well::Marker* botmarkr = 
			wd_->markers().getByName( zrangeflds_[0]->text(1) );

    if ( mintv.start == mintv.stop )
    { topmarkr = 0; botmarkr = 0; }

    if ( topmarkr )
	zrg.start = topmarkr->dah();

    if ( botmarkr )
	zrg.stop = botmarkr->dah();

    zrg.sort();
	
    return true;
}


void uiInfoDlg::propChanged( CallBacker* )
{
    const int selidx = choicefld_->getIntValue();
    Interval<float> zrg = zrangeflds_[selidx]->getFInterval();
    if ( !selidx ) getMarkerDepths( zrg );

    for ( int idx=0; idx<zrangeflds_.size(); idx++ )
    {
	zrangeflds_[idx]->display( idx == selidx );
	zlabelflds_[idx]->display( idx == selidx );
    }

    if ( !selidx )
	zrangeflds_[2]->setValue( zrg );
    else if ( selidx == 1 )
	zrangeflds_[2]->setValue( params_->d2T( zrg, false ) );
    else if ( !selidx || selidx == 2 )
	zrangeflds_[1]->setValue( params_->d2T( zrg ) );

    const Interval<float> timerg = selidx==1 ? zrg : params_->d2T( zrg );
    const double wvltlgth = (double)estwvltlengthfld_->getIntValue()/1000;
    if ( wvltlgth >= timerg.width() )
    { 
	uiMSG().error("the wavelet must be shorter than the computation time"); 
	return; 
    }
    
    params_->estwvltlength_ = (int)round(wvltlgth/SI().zStep());
    params_->timeintvs_[2]= timerg;
    params_->resetTimeParams();

    wvltChanged(0);
}


void uiInfoDlg::drawData()
{
    wvltdraw_->redrawWavelets();
    crosscorr_->setCrossCorrelation();
}


void uiInfoDlg::wvltChanged( CallBacker* cb )
{
    if ( !dataplayer_->computeWvltPack() ) 
	return;

    drawData();
    redrawNeeded.trigger();
}

}; //namespace Well
