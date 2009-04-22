/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiwelltietoseismicdlg.cc,v 1.3 2009-04-22 13:37:11 cvsbruno Exp $";

#include "uiwelltietoseismicdlg.h"
#include "uiwelltiecontrolview.h"
#include "uiwelltieview.h"
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

#include "welldata.h"
#include "wellimpasc.h"
#include "welllogset.h"
#include "welllog.h"
#include "wellman.h"

#include "welltiecshot.h"
#include "welltiesetup.h"
#include "welltietoseismic.h"


#define mErrRet(msg) \
{ uiMSG().error(msg); return false; }

uiWellTieToSeismicDlg::uiWellTieToSeismicDlg( uiParent* p, 
					      const WellTieSetup& wts,
					      const Attrib::DescSet& ads, 
					      bool isdelclose)
	: uiDialog(p,uiDialog::Setup("Tie Well to seismics dialog", "",
		    			mTODOHelpID).modal(false))
    	, wtsetup_(WellTieSetup(wts))
	, wd_(Well::MGR().get(wts.wellid_))
	, wts_(0)
	, dataviewer_(0)
    	, controlview_(0)	  
{
    wtsetup_.iscsavailable_ = wd_->checkShotModel();			  
    setWinTitle( ads );
    vwrgrp_ = new uiGroup( this, "viewers group" );
    dps_ = new DataPointSet( false, false );
    uiTaskRunner* tr = new uiTaskRunner( p );
    wts_ = new WellTieToSeismic( wtsetup_, ads, *dps_, dispdata_, *wd_, tr );
    toolbar_ = new uiToolBar( this, "Well Tie Control" );
    dataviewer_ = new uiWellTieView( vwrgrp_, *dps_, dispdata_,
				     *wd_, wtsetup_, ads );
    drawFields( vwrgrp_ );
    initAll();

    finaliseDone.notify( mCB(this,uiWellTieToSeismicDlg,setView) );
}


uiWellTieToSeismicDlg::~uiWellTieToSeismicDlg()
{
    
    for ( int idx=0; idx<dispdata_.size(); idx++ )
	delete ( dispdata_.remove(idx) );
    if (dps_) delete dps_;
    if ( wvltdraw_  )
	wvltdraw_->wvltChanged.remove(mCB(this,uiWellTieToSeismicDlg,wvltChg));
    if ( dataviewer_ ) delete dataviewer_;
    if (wd_) delete wd_;	   
    
}


void uiWellTieToSeismicDlg::setWinTitle( const Attrib::DescSet& ads )
{
    const Attrib::Desc* ad = ads.getDesc( wtsetup_.attrid_ );
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
    doWork();
    addControl();
    addToolBarTools();
}


void uiWellTieToSeismicDlg::doWork()
{
    dps_->bivSet().empty();
    dps_->dataChanged();
    wts_->computeAll();
    drawData();
}


void uiWellTieToSeismicDlg::drawData()
{
    wvltdraw_->initWavelets( 0 );
    dataviewer_->setUpTimeAxis();
    dataviewer_->fullRedraw( vwrgrp_ );
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
    for (int vwridx=0; vwridx<dataviewer_->viewerSize(); vwridx++)
	viewer += dataviewer_->getViewer( vwridx );

    controlview_ = new uiWellTieControlView( this, toolbar_, viewer );
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
    wvltdraw_ = new uiWellTieWavelet( wvltgrp, wtsetup_ );
    wvltdraw_->wvltChanged.notify( mCB(this,uiWellTieToSeismicDlg,wvltChg) );
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
}


void uiWellTieToSeismicDlg::wvltChg( CallBacker* )
{
    wts_->computeSynthetics();
    dataviewer_->drawSynthetics();
    dataviewer_->drawDensLog();
}


void uiWellTieToSeismicDlg::updateButtons()
{
    if ( wtsetup_.iscsavailable_ )
	cscorrfld_->display(false);
}


void uiWellTieToSeismicDlg::checkShotChg( CallBacker* )
{
    wtsetup_.iscscorr_ = cscorrfld_->isChecked();
    dataviewer_->drawVelLog();
    applybut_->setSensitive(true);
}


bool uiWellTieToSeismicDlg::editD2TPushed( CallBacker* )
{
    uiD2TModelDlg* d2tmdlg = new uiD2TModelDlg( this, *wd_, false );
    if ( d2tmdlg->go() )
    {
	if ( !wts_->updateD2TModel()) return false;
	doWork();
	undobut_->setSensitive( false );
	applybut_->setSensitive( false );

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
	if ( !wts_->saveD2TModel(uifiledlg->fileName()) )
	    return true;
    }
    delete uifiledlg;
   
    return true;
}


void uiWellTieToSeismicDlg::applyPushed( CallBacker* )
{
    wtsetup_.iscscorr_ = cscorrfld_->isChecked();
    wts_->computeD2TModel();
    
    doWork();
    undobut_->setSensitive( true );
    applybut_->setSensitive( false );
}


bool uiWellTieToSeismicDlg::undoPushed( CallBacker* )
{
    if ( !wts_->undoD2TModel() )
    	mErrRet( "Cannot go back to previous model" );
    
    doWork();
    undobut_->setSensitive( false );
    applybut_->setSensitive( false );

    return true;	    
}


bool uiWellTieToSeismicDlg::rejectOK( CallBacker* )
{
    wts_->cancelD2TModel();

    return true;
}


bool uiWellTieToSeismicDlg::acceptOK( CallBacker* )
{
    if ( !wts_->commitD2TModel() )
	mErrRet("Cannot write new depth/time model");

    return true;
}



