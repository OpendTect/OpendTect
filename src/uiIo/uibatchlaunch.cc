/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra / Bert
 Date:          January 2002 / Mar 2014
________________________________________________________________________

-*/

#include "uibatchlaunch.h"

#include "uiclusterjobprov.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uilabel.h"
#include "uitoolbutton.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uiusershowwait.h"
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


// uiProcSettingsGroup
static const char* sKeyClusterProc = "dTect.Enable Cluster Processing";
static const char* sKeyClusterProcEnv = "DTECT_CLUSTER_PROC";


const uiString uiStartBatchJobDialog::sKeyNoParFiles()
{
    return tr("<No job files found>");
}


uiProcSettingsGroup::uiProcSettingsGroup( uiParent* p, Settings& setts )
    : uiSettingsGroup(p,setts)
    , initialnrinl_( InlineSplitJobDescProv::defaultNrInlPerJob() )
    , initialcpenabled_( clusterProcEnabled() )
{
    nrinlfld_ = new uiGenInput( this, tr("Default number of inlines per job"),
				IntInpSpec(initialnrinl_,1,10000) );

    clusterfld_ = new uiGenInput( this, tr("Enable cluster processing"),
				  BoolInpSpec(initialcpenabled_) );
    clusterfld_->attach( alignedBelow, nrinlfld_ );

    bottomobj_ = clusterfld_;
}


bool uiProcSettingsGroup::clusterProcEnabled() const
{
    bool isenab = false;
    const bool hassetting = setts_.getYN( sKeyClusterProc, isenab );
    if ( !hassetting )
	isenab = GetEnvVarYN( sKeyClusterProcEnv );
    return isenab;
}


void uiProcSettingsGroup::doCommit( uiRetVal& )
{
    const int nrinl = nrinlfld_->getIntValue();
    if ( nrinl != initialnrinl_ )
    {
	changed_ = true;
	InlineSplitJobDescProv::setDefaultNrInlPerJob( nrinl );
    }

    const bool cpenabled  = clusterfld_->getBoolValue();
    updateSettings( initialcpenabled_, cpenabled, sKeyClusterProc );
}


// uiStartBatchJobDialog
uiStartBatchJobDialog::uiStartBatchJobDialog( uiParent* p )
    : uiDialog(p,Setup(tr("Start/ReStart a batch job"),mNoDlgTitle,
                        mODHelpKey(mRestartBatchDialogHelpID) ))
    , canresume_(false)
{
    uiGroup* topgrp = new uiGroup( this, "Top Group" );

    jobsfld_ = new uiListBox( topgrp, "Stored Batch Job" );
    jobsfld_->box()->setPrefHeightInChar( 10 );
    mAttachCB( jobsfld_->selectionChanged, uiStartBatchJobDialog::itmSel );

    vwfilebut_ = new uiToolButton( topgrp, "info", tr("View/Edit job file"),
		      mCB(this,uiStartBatchJobDialog,viewFile) );
    vwfilebut_->attach( rightOf, jobsfld_ );
    rmfilebut_ = uiToolButton::getStd( topgrp, OD::Delete,
		 mCB(this,uiStartBatchJobDialog,rmFile),
		 uiStrings::phrDelete(tr("Job File")) );
    rmfilebut_->attach( centeredRightOf, jobsfld_ );

    topgrp->setFrame( true );
    topgrp->setHAlignObj( jobsfld_ );

    uiGroup* botgrp = new uiGroup( this, "Bottom Group" );
    invalidsellbl_ = new uiLabel( botgrp, sKeyNoParFiles() );

    batchfld_ = new uiBatchJobDispatcherSel( botgrp, false,
					     Batch::JobSpec::Attrib );
    mAttachCB( batchfld_->selectionChange, uiStartBatchJobDialog::launcherSel );

    resumefld_ = new uiGenInput( botgrp, tr("Use already processed data"),
	BoolInpSpec(false,uiStrings::sYes(),tr("No, start from scratch")) );
    resumefld_->attach( alignedBelow, batchfld_ );
    resumefld_->attach( alignedBelow, invalidsellbl_ );

    botgrp->setHAlignObj( batchfld_ );
    botgrp->attach( leftAlignedBelow, topgrp );

    mAttachCB( afterPopup, uiStartBatchJobDialog::fillList );
}


uiStartBatchJobDialog::~uiStartBatchJobDialog()
{
    detachAllNotifiers();
}


void uiStartBatchJobDialog::fillList( CallBacker* )
{
    NotifyStopper ns( jobsfld_->selectionChanged );

    uiUserShowWait usw( parent(), tr("Scanning Proc directory") );
    jobsfld_->setEmpty();
    filenames_.setEmpty();
    const DirList dl( GetProcFileName(0), File::FilesInDir, "*.par" );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	filenames_.add( dl.fullPath(idx) );
	const BufferString& fnm = dl.get( idx );
	jobsfld_->addItem( toUiString(Batch::JobDispatcher::getJobName(fnm)) );
    }

    usw.readyNow();
    const bool hasjobs = !filenames_.isEmpty();
    batchfld_->display( hasjobs );
    resumefld_->display( hasjobs );
    if ( !hasjobs )
	return;

    ns.enableNotification();
    jobsfld_->setCurrentItem( 0 );
}


void uiStartBatchJobDialog::itmSel( CallBacker* )
{
    const bool haveparfiles = !jobsfld_->isEmpty();
    bool emptyfile = false;
    batchfld_->jobSpec().prognm_.setEmpty();
    if ( haveparfiles )
    {
	const int selidx = jobsfld_->currentItem();
	if ( selidx >= 0 && filenames_.validIdx(selidx) )
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
		batchfld_->setJobName( jobsfld_->itemText(selidx) );
	    }
	}
    }

    const bool canrun = canRun();
    batchfld_->display( canrun );
    invalidsellbl_->display( !canrun );
    if ( !canrun )
	invalidsellbl_->setText( !haveparfiles ? sKeyNoParFiles()
		    : (emptyfile ? tr("<Empty file>") : tr("<Pre-5.0 Job>")) );

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
	    canresume_ = uibjdl->dispatcher().canResume( batchfld_->jobSpec() );
    }

    resumefld_->display( canrun && canresume_ );
}


void uiStartBatchJobDialog::viewFile( CallBacker* )
{
    const int selidx = jobsfld_->currentItem();
    if ( selidx < 0 || !filenames_.validIdx(selidx) )
	{ pErrMsg("Huh"); return; }

    uiTextFileDlg* dlg = new uiTextFileDlg( this, filenames_.get(selidx) );
    dlg->setDeleteOnClose( true );
    dlg->setCaption( toUiString("%1: %2").arg( uiStrings::sJob() )
			    .arg(jobsfld_->textOfItem(selidx)) );
    dlg->go();
}


void uiStartBatchJobDialog::rmFile( CallBacker* )
{
    const int selidx = jobsfld_->currentItem();
    if ( selidx < 0 || !filenames_.validIdx(selidx) )
	{ pErrMsg("Huh"); return; }

    const OD::String& fnm( filenames_.get(selidx) );
    if ( !File::remove(fnm) )
    { uiMSG().error(uiStrings::phrCannotRemove(tr("job file"))); return; }

    jobsfld_->removeItem( selidx );
    filenames_.removeSingle( selidx );
    int newsel = selidx;
    if ( newsel >= filenames_.size() )
	newsel--;
    if ( newsel >= 0 )
	jobsfld_->setCurrentItem( newsel );
    itmSel( nullptr );
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


bool uiStartBatchJobDialog::acceptOK()
{
    if ( !canRun() )
    {
	const int selidx = jobsfld_->currentItem();
	if ( selidx >= 0 )
	    uiMSG().error(tr("Cannot run selected job"));
	else if ( jobsfld_->isEmpty() )
	    return true;
	else
	    uiMSG().error(uiStrings::phrSelect(tr("a job to run")));
	return false;
    }

    if ( canresume_ )
	Batch::JobDispatcher::setUserWantsResume( batchfld_->jobSpec().pars_,
		resumefld_->getBoolValue() );

    return batchfld_->start();
}
