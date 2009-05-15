/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiwelltietoseismicdlg.cc,v 1.6 2009-05-15 12:42:48 cvsbruno Exp $";

#include "uiwelltietoseismicdlg.h"
#include "uiwelltiecontrolview.h"
#include "uiwelltieview.h"
#include "uiwelltiestretch.h"
#include "uiwelltiewavelet.h"

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
	, datadrawer_(0)
    	, controlview_(0)
	, params_(0)       		 
{
    uiTaskRunner* tr = new uiTaskRunner( p );
    toolbar_ = new uiToolBar( this, "Well Tie Control" );
    vwrgrp_ = new uiGroup( this, "viewers group" );

    params_ 	= new WellTieParams( setup_, wd_, ads );
    datamgr_    = new WellTieDataMGR( params_ );
    dataplayer_ = new WellTieToSeismic( wd_, ads, params_, *datamgr_, tr );
    datadrawer_ = new uiWellTieView( vwrgrp_, *datamgr_, wd_, params_, ads );
    
    setWinTitle( ads );
    drawFields( vwrgrp_ );
    initAll();
    
    finaliseDone.notify( mCB(this,uiWellTieToSeismicDlg,setView) );
}


uiWellTieToSeismicDlg::~uiWellTieToSeismicDlg()
{
    delete params_;
    if ( datamgr_ ) delete ( datamgr_ );
    if ( datadrawer_ ) delete datadrawer_;
    if ( wd_ ) delete wd_;	  
    if ( wvltdraw_  )
	wvltdraw_->wvltChanged.remove(mCB(this,uiWellTieToSeismicDlg,wvltChg));
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
    params_->resetTimeParams(0);
    dataplayer_->computeAll();
    stretcher_->resetData();
    drawData();
}


void uiWellTieToSeismicDlg::doLogWork( bool ishighres )
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
    stretcher_   = new uiWellTieStretch( this, params_, wd_,  *datamgr_,
	   				 *datadrawer_, *controlview_ );
    stretcher_->dispdataChanging.notify( 
		    mCB(this,uiWellTieToSeismicDlg,dispDataChanging) );
    stretcher_->dispdataChanged.notify( 
		    mCB(this,uiWellTieToSeismicDlg,dispDataChanged) );
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
/*
    uiGroup* corrgrp =  new uiGroup( this, "correlation coefficient group" );
    corrgrp->attach( alignedBelow, wvltgrp );
    markernames_.add( Well::TrackSampler::sKeyDataStart() );
    for ( int idx=0; idx<Well::MGR().wells()[0]->markers().size(); idx++ )
	markernames_.add( Well::MGR().wells()[0]->markers()[idx]->name() );
    markernames_.add( Well::TrackSampler::sKeyDataEnd() );

    StringListInpSpec slis( markernames_ );

    corrtopmrkfld_ = new uiGenInput( corrgrp, "Compute correlation between",
    slis.setName("Top Marker") );

    corrtopmrkfld_->setValue( (int)0 );
    corrtopmrkfld_->setElemSzPol( uiObject::Medium );

    corrbotmrkfld_ = new uiGenInput( corrgrp, "", slis.setName("Bottom Marker") );
    corrbotmrkfld_->attach( rightOf, corrtopmrkfld_ );
    corrbotmrkfld_->setValue( markernames_.size()-1 );
    corrbotmrkfld_->setElemSzPol( uiObject::Medium );
    
    corrcoefffld_ = new uiGenInput( corrgrp, "Correlation coefficient", 
	    				FloatInpSpec(0));
    corrcoefffld_->attach( rightOf, corrbotmrkfld_ );*/
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
    cscorrfld_->setChecked(true);
    cscorrfld_->activated.notify(mCB(this,uiWellTieToSeismicDlg,checkShotChg));
    cscorrfld_->attach( rightOf, undobut_ );
    
    setPrefWidthInChar( 60 );
    updateButtons();	    
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
    doLogWork( true );
}


void uiWellTieToSeismicDlg::updateButtons()
{
    if ( !params_->iscsavailable_ )
	cscorrfld_->display(false);
}


void uiWellTieToSeismicDlg::checkShotChg( CallBacker* )
{
    params_->iscscorr_ = cscorrfld_->isChecked();
    datadrawer_->drawVelLog();
    applybut_->setSensitive(true);
}


void uiWellTieToSeismicDlg::dispDataChanged( CallBacker* )
{
    dataplayer_->setd2TModelFromData();
    applybut_->setSensitive( true );
}


void uiWellTieToSeismicDlg::dispDataChanging( CallBacker* )
{
    doLogWork( false );
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
    doWholeWork();
    applybut_->setSensitive( false );
    undobut_->setSensitive( true );
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
	mErrRet("Cannot write new depth/time model");

    return true;
}



