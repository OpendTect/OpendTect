/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra / Bert
 Date:          January 2002 / Mar 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uibatchlaunch.h"

#include "uiclusterjobprov.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uilabel.h"
#include "uitoolbutton.h"
#include "uibuttongroup.h"
#include "uimsg.h"
#include "uibatchjobdispatchersel.h"
#include "uibatchjobdispatcherlauncher.h"

#include "batchjobdispatch.h"
#include "jobdescprov.h"
#include "settings.h"
#include "envvars.h"
#include "oddirs.h"
#include "file.h"
#include "dirlist.h"
#include "ascstream.h"
#include "od_istream.h"

static const char* sKeyClusterProc = "dTect.Enable Cluster Processing";
static const char* sKeyClusterProcEnv = "DTECT_CLUSTER_PROC";

static bool enabClusterProc()
{
    bool enabclusterproc = false;
    const bool hassetting =
	Settings::common().getYN( sKeyClusterProc, enabclusterproc );
    if ( !hassetting )
	enabclusterproc = GetEnvVarYN( sKeyClusterProcEnv );
    return enabclusterproc;
}


uiProcSettings::uiProcSettings( uiParent* p )
    : uiDialog(p,Setup("Processing settings",mNoDlgTitle,"103.2.27"))
{
    const int nrinl = InlineSplitJobDescProv::defaultNrInlPerJob();
    nrinlfld_ = new uiGenInput( this, "Default number of inlines per job",
				IntInpSpec(nrinl,1,10000) );

    const bool enabclusterproc = enabClusterProc();
    clusterfld_ = new uiGenInput( this, "Enable cluster processing",
				  BoolInpSpec(enabclusterproc) );
    clusterfld_->attach( alignedBelow, nrinlfld_ );
}


bool uiProcSettings::acceptOK( CallBacker* )
{
    InlineSplitJobDescProv::setDefaultNrInlPerJob( nrinlfld_->getIntValue() );
    Settings::common().setYN( sKeyClusterProc, clusterfld_->getBoolValue() );
    Settings::common().write( false );
    return true;
}


uiStartBatchJobDialog::uiStartBatchJobDialog( uiParent* p )
    : uiDialog(p,Setup("(Re-)Start a batch job",mNoDlgTitle,"101.2.1"))
    , canresume_(false)
{
    uiGroup* topgrp = new uiGroup( this, "Top Group" );

    uiLabeledListBox* llb = new uiLabeledListBox( topgrp, "Stored Batch Job" );
    jobsfld_ = llb->box();
    jobsfld_->addItem( "Scanning Proc directory ...." );

    uiButtonGroup* bgrp = new uiButtonGroup( topgrp, "Man buttons",
	    					uiObject::Vertical );
    bgrp->attach( rightOf, llb );
    vwfilebut_ = new uiToolButton( bgrp, "info", "View/Edit job file",
	    	      mCB(this,uiStartBatchJobDialog,viewFile) );
    rmfilebut_ = new uiToolButton( bgrp, "trashcan", "Remove job file",
	    	      mCB(this,uiStartBatchJobDialog,rmFile) );

    topgrp->setFrame( true );
    topgrp->setHAlignObj( llb );

    invalidsellbl_ = new uiLabel( this, "<Invalid Job>" );
    invalidsellbl_->attach( alignedBelow, topgrp );

    batchfld_ = new uiBatchJobDispatcherSel( this, false,
					     Batch::JobSpec::Attrib );
    batchfld_->selectionChange.notify(
				mCB(this,uiStartBatchJobDialog,launcherSel) );
    batchfld_->attach( alignedBelow, topgrp );

    resumefld_ = new uiGenInput( this, "Use already processed data",
			BoolInpSpec(false,"Yes","No (start from scratch)") );
    resumefld_->attach( alignedBelow, batchfld_ );

    afterPopup.notify( mCB(this,uiStartBatchJobDialog,fillList) );
}


void uiStartBatchJobDialog::fillList( CallBacker* )
{
    jobsfld_->setEmpty();
    DirList dl( GetProcFileName(0), DirList::FilesOnly, "*.par" );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	filenames_.add( dl.fullPath(idx) );
	const BufferString& fnm = dl.get( idx );
	jobsfld_->addItem( Batch::JobDispatcher::getJobName(fnm) );
    }

    jobsfld_->selectionChanged.notify( mCB(this,uiStartBatchJobDialog,itmSel) );
    if ( !filenames_.isEmpty() )
	jobsfld_->setCurrentItem( 0 );
    else
	invalidsellbl_->setText( "(No Proc/*.par files)" );
}


void uiStartBatchJobDialog::itmSel( CallBacker* )
{
    const int selidx = jobsfld_->currentItem();
    if ( selidx >= 0 )
    {
	od_istream strm( filenames_.get(selidx) );
	if ( strm.isOK() )
	{
	    ascistream astrm( strm );
	    IOPar iop( astrm );
	    strm.close();
	    batchfld_->jobSpec().prognm_.setEmpty();
	    batchfld_->jobSpec().usePar( iop );
	    batchfld_->jobSpecUpdated();
	    batchfld_->setJobName( jobsfld_->textOfItem(selidx) );
	}
    }

    const bool canrun = canRun();
    invalidsellbl_->display( !canrun );
    batchfld_->display( canrun );
    setButSens();

    launcherSel(0);
}


void uiStartBatchJobDialog::launcherSel( CallBacker* )
{
    const bool canrun = canRun();
    canresume_ = canRun();
    if ( canresume_ )
    {
	uiBatchJobDispatcherLauncher* uibjdl = batchfld_->selectedLauncher();
	if ( !uibjdl )
	    canresume_ = false;
	else
	    canresume_ = uibjdl->dispatcher().canResume(
		    batchfld_->jobSpec() );
    }

    resumefld_->display( canrun && canresume_ );
}


void uiStartBatchJobDialog::viewFile( CallBacker* )
{
    const int selidx = jobsfld_->currentItem();
    if ( selidx < 0 )
	{ pErrMsg("Huh"); return; }
    const BufferString& fnm( filenames_.get(selidx) );

    uiMSG().error( "TODO: implement view/edit job file:\n", fnm );
}


void uiStartBatchJobDialog::rmFile( CallBacker* )
{
    const int selidx = jobsfld_->currentItem();
    if ( selidx < 0 )
	{ pErrMsg("Huh"); return; }
    const BufferString& fnm( filenames_.get(selidx) );

    uiMSG().error( "TODO: implement remove job file:\n", fnm );
}


bool uiStartBatchJobDialog::canRun() const
{
    return !batchfld_->jobSpec().prognm_.isEmpty();
}


void uiStartBatchJobDialog::setButSens()
{
    const int selidx = jobsfld_->currentItem();
    const bool haveselection = filenames_.validIdx( selidx );
    vwfilebut_->setSensitive( haveselection );
    rmfilebut_->setSensitive( haveselection
			   && File::isWritable(filenames_.get(selidx)) );
}


bool uiStartBatchJobDialog::acceptOK( CallBacker* )
{
    if ( !canRun() )
    {
	const int selidx = jobsfld_->currentItem();
	if ( selidx >= 0 )
	    uiMSG().error( "Cannot run selected job" );
	else if ( jobsfld_->isEmpty() )
	    return true;
	else
	    uiMSG().error( "Please select a job to run" );
	return false;
    }

    if ( canresume_ )
	Batch::JobDispatcher::setUserWantsResume( batchfld_->jobSpec().pars_,
		resumefld_->getBoolValue() );

    return batchfld_->start();
}
