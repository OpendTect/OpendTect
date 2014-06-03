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
#include "uimsg.h"
#include "uitextfile.h"
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
#include "od_helpids.h"


// uiProcSettings
static const char* sKeyClusterProc = "dTect.Enable Cluster Processing";
static const char* sKeyClusterProcEnv = "DTECT_CLUSTER_PROC";
static const char* sKeyNoParFiles = "<No job files found>";


static bool enabClusterProc()
{
    bool enabclusterproc = false;
    const bool hassetting =
	Settings::common().getYN( sKeyClusterProc, enabclusterproc );
    if ( !hassetting )
	enabclusterproc = GetEnvVarYN( sKeyClusterProcEnv );
    return enabclusterproc;
}


uiProcSettings::uiProcSettings( uiParent* p, Settings& setts )
    : uiSettingsGroup(p,"Processing",setts)
{
    nrinl_ = InlineSplitJobDescProv::defaultNrInlPerJob();
    nrinlfld_ = new uiGenInput( this, "Default number of inlines per job",
				IntInpSpec(nrinl_,1,10000) );

    enabclusterproc_ = enabClusterProc();
    clusterfld_ = new uiGenInput( this, "Enable cluster processing",
				  BoolInpSpec(enabclusterproc_) );
    clusterfld_->attach( alignedBelow, nrinlfld_ );
}


HelpKey uiProcSettings::helpKey() const
{ return mODHelpKey(mProcSettingsHelpID); }


bool uiProcSettings::acceptOK()
{
    InlineSplitJobDescProv::setDefaultNrInlPerJob( nrinlfld_->getIntValue() );
    Settings::common().setYN( sKeyClusterProc, clusterfld_->getBoolValue() );
    return true;
}


// uiStartBatchJobDialog
uiStartBatchJobDialog::uiStartBatchJobDialog( uiParent* p )
    : uiDialog(p,Setup("(Re-)Start a batch job",mNoDlgTitle,
                        mODHelpKey(mRestartBatchDialogHelpID) ))
    , canresume_(false)
{
    uiGroup* topgrp = new uiGroup( this, "Top Group" );

    uiLabeledListBox* llb = new uiLabeledListBox( topgrp, "Stored Batch Job" );
    llb->setPrefHeightInChar( 10 );
    jobsfld_ = llb->box();
    jobsfld_->addItem( "Scanning Proc directory ...." );

    vwfilebut_ = new uiToolButton( topgrp, "info", "View/Edit job file",
		      mCB(this,uiStartBatchJobDialog,viewFile) );
    vwfilebut_->attach( rightOf, llb );
    rmfilebut_ = new uiToolButton( topgrp, "trashcan", "Remove job file",
		      mCB(this,uiStartBatchJobDialog,rmFile) );
    rmfilebut_->attach( centeredRightOf, llb );

    topgrp->setFrame( true );
    topgrp->setHAlignObj( llb );

    invalidsellbl_ = new uiLabel( this, sKeyNoParFiles );
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
    {
	batchfld_->display( false );
	resumefld_->display( false );
    }
}


void uiStartBatchJobDialog::itmSel( CallBacker* )
{
    const bool haveparfiles = !jobsfld_->isEmpty();
    bool emptyfile = false;
    batchfld_->jobSpec().prognm_.setEmpty();
    if ( haveparfiles )
    {
	const int selidx = jobsfld_->currentItem();
	if ( selidx >= 0 )
	{
	    od_istream strm( filenames_.get(selidx) );
	    if ( !strm.isOK() )
		emptyfile = true;
	    else
	    {
		ascistream astrm( strm );
		IOPar iop( astrm );
		strm.close();
		batchfld_->jobSpec().usePar( iop );
		batchfld_->jobSpecUpdated();
		batchfld_->setJobName( jobsfld_->textOfItem(selidx) );
	    }
	}
    }

    const bool canrun = canRun();
    batchfld_->display( canrun );
    invalidsellbl_->display( !canrun );
    if ( !canrun )
	invalidsellbl_->setText( !haveparfiles ? sKeyNoParFiles
			   : (emptyfile ? "<Empty file>" : "<Pre-5.0 Job>") );

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

    uiTextFileDlg* dlg = new uiTextFileDlg( this, filenames_.get(selidx) );
    dlg->setDeleteOnClose( true );
    dlg->setCaption( BufferString( "Job: ", jobsfld_->textOfItem(selidx) ) );
    dlg->go();
}


void uiStartBatchJobDialog::rmFile( CallBacker* )
{
    const int selidx = jobsfld_->currentItem();
    if ( selidx < 0 )
	{ pErrMsg("Huh"); return; }

    const OD::String& fnm( filenames_.get(selidx) );
    if ( !File::remove(fnm) )
	{ uiMSG().error( "Could not remove job file" ); return; }

    jobsfld_->removeItem( selidx );
    filenames_.removeSingle( selidx );
    int newsel = selidx;
    if ( newsel >= filenames_.size() )
	newsel--;
    if ( newsel >= 0 )
	jobsfld_->setCurrentItem( newsel );
    else
	itmSel( 0 );
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
