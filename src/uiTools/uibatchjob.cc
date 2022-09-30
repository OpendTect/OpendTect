/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibatchjobdispatchersel.h"
#include "uibatchprocdlg.h"
#include "uibatchjobdispatcherlauncher.h"

#include "batchjobdispatch.h"
#include "batchprogtracker.h"
#include "clientservicebase.h"
#include "errmsg.h"
#include "file.h"
#include "genc.h"
#include "hostdata.h"
#include "ioobj.h"
#include "keystrs.h"
#include "netserver.h"
#include "oddirs.h"
#include "singlebatchjobdispatch.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uinotsaveddlg.h"
#include "uislider.h"
#include "uistrings.h"

static const char* sGroupName = "Batch job dispatcher selector";


uiBatchJobDispatcherSel::uiBatchJobDispatcherSel( uiParent* p, bool optional,
						  ProcType proctyp,
						  OS::LaunchType launchtype )
    : uiGroup(p,sGroupName)
    , jobspec_(proctyp,launchtype)
    , selectionChange(this)
    , checked(this)
{
    init( optional );
}


uiBatchJobDispatcherSel::uiBatchJobDispatcherSel( uiParent* p, bool optional,
						  const JobSpec& js )
    : uiGroup(p,sGroupName)
    , jobspec_(js)
    , selectionChange(this)
    , checked(this)
{
    init( optional );
}


uiBatchJobDispatcherSel::~uiBatchJobDispatcherSel()
{}


void uiBatchJobDispatcherSel::init( bool optional )
{
    Factory1Param<uiBatchJobDispatcherLauncher,Batch::JobSpec&>& fact
				= uiBatchJobDispatcherLauncher::factory();
    const BufferStringSet& nms = fact.getNames();
    for ( int idx=0; idx<nms.size(); idx++ )
    {
	uiBatchJobDispatcherLauncher* dl = fact.create( nms.get(idx), jobspec_);
	if ( dl && dl->isSuitedFor(jobspec_.prognm_) )
	    uidispatchers_ += dl;
	else
	    delete dl;
    }

    if ( uidispatchers_.isEmpty() )
	{ pErrMsg("No batch dispatcher launchers at all"); return; }

    uiString optionsbuttxt = uiStrings::sOptions();
    const CallBack fldchkcb( mCB(this,uiBatchJobDispatcherSel,fldChck) );
    uiObject* attachobj = 0;
    const bool onlyonechoice = uidispatchers_.size() == 1;
    if ( onlyonechoice )
    {
	if ( !optional )
	    optionsbuttxt = tr("Execution Options");
	else
	{
	    dobatchbox_ = new uiCheckBox( this, tr("Execute in Batch") );
	    dobatchbox_->activated.notify( fldchkcb );
	    attachobj = dobatchbox_;
	}
    }
    else
    {
	selfld_ = new uiGenInput( this, tr("Batch execution"),
				  StringListInpSpec());
	selfld_->valuechanged.notify( mCB(this,uiBatchJobDispatcherSel,selChg));
	if ( optional )
	{
	    selfld_->setWithCheck( true );
	    selfld_->setChecked( false );
	    selfld_->checked.notify( fldchkcb );
	}
	attachobj = selfld_->attachObj();
    }

    optsbut_ = uiButton::getStd( this, OD::Options,
		mCB(this,uiBatchJobDispatcherSel,optsPush), false,
		optionsbuttxt );
    if ( attachobj )
	optsbut_->attach( rightOf, attachobj );

    if ( selfld_ )
	setHAlignObj( selfld_ );
    else
	setHAlignObj( optsbut_ );

    postFinalize().notify( mCB(this,uiBatchJobDispatcherSel,initFlds) );
}


void uiBatchJobDispatcherSel::initFlds( CallBacker* )
{
    jobSpecUpdated();
    fldChck( 0 );
}


void uiBatchJobDispatcherSel::setJobSpec( const JobSpec& js )
{
    jobspec_ = js;
    jobSpecUpdated();
}


void uiBatchJobDispatcherSel::jobSpecUpdated()
{
    if ( !selfld_ )
	return;

    uiStringSet nms;
    for ( int idx=0; idx<uidispatchers_.size(); idx++ )
    {
	uiBatchJobDispatcherLauncher* dl = uidispatchers_[idx];
	if ( dl->canHandleJobSpec() )
	    nms.add( dl->name() );
    }

    const BufferString oldsel = selfld_->text();
    selfld_->newSpec( StringListInpSpec(nms), 0 );
    if ( !oldsel.isEmpty() )
	selfld_->setText( oldsel );

    selChg( 0 );
}


void uiBatchJobDispatcherSel::setWantBatch( bool yn )
{
    if ( selfld_ && selfld_->isCheckable() )
	selfld_->setChecked( yn );
    if ( dobatchbox_ )
	dobatchbox_->setChecked( yn );
}


bool uiBatchJobDispatcherSel::wantBatch() const
{
    if ( noLaunchersAvailable() )
	return false;

    if ( selfld_ )
	return !selfld_->isCheckable() || selfld_->isChecked();

    return dobatchbox_ ? dobatchbox_->isChecked() : true;
}


uiBatchJobDispatcherLauncher* uiBatchJobDispatcherSel::selectedLauncher()
{
    const int selidx = selIdx();
    return selidx < 0 ? 0 : uidispatchers_[selidx];
}


uiString uiBatchJobDispatcherSel::selected() const
{
    const int selidx = selIdx();
    return selidx < 0 ? uiString::empty()
		      : uidispatchers_[selidx]->name();
}


int uiBatchJobDispatcherSel::selIdx() const
{
    if ( !selfld_ )
	return optsbut_ ? 0 : -1;

    const BufferString cursel( selfld_->text() );
    if ( cursel.isEmpty() )
	return -1;

    for ( int idx=0; idx<uidispatchers_.size(); idx++ )
    {
	if ( cursel == toString(uidispatchers_[idx]->name()) )
	    return idx;
    }

    pErrMsg( "Huh? list selection not in factory list" );
    return -1;
}


const uiString uiBatchJobDispatcherSel::selectedInfo() const
{
    const int selidx = selIdx();
    return selidx < 0 ? uiString::empty()
		      : uidispatchers_[selidx]->getInfo();
}


bool uiBatchJobDispatcherSel::start( Batch::ID* batchid )
{
    const int selidx = selIdx();
    if ( selidx < 0 )
    {
	uiMSG().error( tr("Please select a batch execution method") );
	return false;
    }

    uiBatchJobDispatcherLauncher* dl = uidispatchers_[selidx];
    dl->dispatcher().setJobName( jobname_.buf() );
    if ( dl->go(this,batchid) )
    {
	mAttachCB( NotSavedPrompter::NSP().promptSaving,
		    uiBatchJobDispatcherSel::removeBatchProcess );
	return true;
    }

    return false;
}


bool uiBatchJobDispatcherSel::saveProcPars( const IOObj& ioobj ) const
{
    FilePath fp( ioobj.fullUserExpr() );
    if ( fp.pathOnly().isEmpty() )
    {
	FilePath survfp( GetDataDir(), ioobj.dirName() );
	if ( !survfp.exists() )
	    return false;

	fp.setPath( survfp.fullPath() );
    }

    fp.setExtension( "proc" );
    return jobspec_.pars_.write( fp.fullPath(), sKey::Pars() );
}


void uiBatchJobDispatcherSel::setJobName( const char* nm )
{
    jobname_ = nm;
}


BufferStringSet uiBatchJobDispatcherSel::getActiveProgramList() const
{
    BufferStringSet activeprogramlist;
    BatchServiceClientMgr& mgr = BatchServiceClientMgr::getMgr();
    const TypeSet<Network::Service::ID>& servids = BPT().getServiceIDs();
    for (const auto& servid : servids)
    {
	FilePath fp(mgr.getLockFileFP(servid));
	fp.setExtension(nullptr);
	activeprogramlist.add(fp.fileName());
    }

    return activeprogramlist;
}


void uiBatchJobDispatcherSel::removeBatchProcess( CallBacker* )
{
    BatchProgramTracker& bpt = eBPT();
    const BufferStringSet& activebatchprogs = getActiveProgramList();
    const TypeSet<Network::Service::ID>& servids = bpt.getServiceIDs();
    NotSavedPrompter& nsp = NotSavedPrompter::NSP();
    for( int idx=0; idx<activebatchprogs.size(); idx++ )
    {
	const auto& activebatchprog = activebatchprogs.get( idx );
	const auto& dataid = servids[idx];
	nsp.addObject( activebatchprog.buf(),
	    mCB(this,uiBatchJobDispatcherSel,triggerRemove), false, &dataid );
    }
}


void uiBatchJobDispatcherSel::triggerRemove( CallBacker* )
{
    const Network::Service::ID* servid =
	(Network::Service::ID*)NotSavedPrompter::NSP().getCurrentObjectData();

    if ( !servid )
	return;

    eBPT().cleanProcess( *servid );
}


void uiBatchJobDispatcherSel::selChg( CallBacker* )
{
    const int selidx = selIdx();
    uiBatchJobDispatcherLauncher* uidisp = selidx < 0 ? 0
					 : uidispatchers_[selidx];
    optsbut_->display( uidisp ? uidisp->hasOptions() : false );

    fldChck( 0 );
    selectionChange.trigger();
}


void uiBatchJobDispatcherSel::fldChck( CallBacker* )
{
    optsbut_->setSensitive( wantBatch() );
    checked.trigger();
}


void uiBatchJobDispatcherSel::optsPush( CallBacker* )
{
    const int selidx = selIdx();
    if ( uidispatchers_.validIdx(selidx) )
	uidispatchers_[selidx]->editOptions( this );
}


// --- Launcher stuff

mImplFactory1Param(uiBatchJobDispatcherLauncher,Batch::JobSpec&,
		   uiBatchJobDispatcherLauncher::factory)

uiBatchJobDispatcherLauncher::uiBatchJobDispatcherLauncher( Batch::JobSpec& js )
    : jobspec_(js)
{}


uiBatchJobDispatcherLauncher::~uiBatchJobDispatcherLauncher()
{}


bool uiBatchJobDispatcherLauncher::go( uiParent* p, Batch::ID* jobid )
{
    if ( !dispatcher().go(jobspec_,jobid) )
    {
	uiRetVal ret( tr("Cannot start program %1").arg(jobspec_.prognm_) );
	ret.add( dispatcher().errMsg() );
	uiMSG().error( ret );
	return false;
    }

    return true;
}


bool uiBatchJobDispatcherLauncher::isSuitedFor( const char* prognm ) const
{
    return dispatcher().isSuitedFor( jobspec_.prognm_ );
}


bool uiBatchJobDispatcherLauncher::canHandleJobSpec() const
{
    return dispatcher().canHandle( jobspec_ );
}


uiString uiBatchJobDispatcherLauncher::getInfo() const
{
    return dispatcher().description();
}



const Batch::JobDispatcher& uiBatchJobDispatcherLauncher::dispatcher() const
{ return const_cast<uiBatchJobDispatcherLauncher*>(this)->gtDsptchr(); }



uiSingleBatchJobDispatcherLauncher::uiSingleBatchJobDispatcherLauncher(
							Batch::JobSpec& js )
    : uiBatchJobDispatcherLauncher(js)
    , sjd_(*new Batch::SingleJobDispatcher)
    , hdl_(false)
{
#ifdef __unix__
    const int niceval = hdl_.niceLevel();
    const StepInterval<int> nicerg(
        OS::CommandExecPars::cMachineUserPriorityRange(false));
    jobspec_.execpars_.prioritylevel_ =
        -1.f * mCast(float, niceval) / nicerg.width();
#endif
}


uiSingleBatchJobDispatcherLauncher::~uiSingleBatchJobDispatcherLauncher()
{
    delete &sjd_;
}


Batch::JobDispatcher& uiSingleBatchJobDispatcherLauncher::gtDsptchr()
{
    return sjd_;
}


bool uiSingleBatchJobDispatcherLauncher::go( uiParent* p, Batch::ID* jobid )
{
    if ( !sjd_.remotehost_.isEmpty() )
    {
	hdl_.refresh();
	const HostData* localhost = hdl_.find(BufferString(GetLocalHostName()));
	if ( !localhost )
	{
	    uiMSG().error( tr("Cannot find configuration for localhost") );
	    return false;
	}

	const FilePath localbasedatadir( GetBaseDataDir() );
	if ( localbasedatadir != localhost->getDataRoot() )
	{
	    uiMSG().error( tr("Current Data Root: '%1'\ndoes not match path "
			    "in batch processing configuration file:\n'%2'\n"
			    "Cannot continue")
			    .arg( localbasedatadir.fullPath() )
			    .arg( localhost->getDataRoot().fullPath() ) );
	    return false;
	}
    }

    return uiBatchJobDispatcherLauncher::go( p, jobid );
}



class uiSingleBatchJobDispatcherPars : public uiDialog
{ mODTextTranslationClass(uiSingleBatchJobDispatcherPars);
public:

uiSingleBatchJobDispatcherPars( uiParent* p, const HostDataList& hdl,
				Batch::SingleJobDispatcher& sjd,
				Batch::JobSpec& js )
    : uiDialog(p,Setup(tr("Batch execution parameters"),
		       tr("Options for '%1' program").arg(js.prognm_),
                       mODHelpKey(mSingleBatchJobDispatcherParsHelpID)))
    , sjd_(sjd)
    , execpars_(js.execpars_)
    , hdl_(hdl)
{
    Batch::SingleJobDispatcher::getDefParFilename( js.prognm_, defparfnm_ );

    BufferStringSet hnms;
    hdl_.fill( hnms, false );
    if ( !hnms.isEmpty() )
    {
	remhostfld_ = new uiGenInput( this, tr("Execute remote"),
				      StringListInpSpec(hnms) );
	remhostfld_->setWithCheck( true );
	const HostData* curhost = sjd_.remotehost_.isEmpty() ? nullptr
				: hdl_.find( sjd_.remotehost_.str() );
	remhostfld_->setChecked( curhost );
	mAttachCB( remhostfld_->valuechanged,
		   uiSingleBatchJobDispatcherPars::hostChgCB );
	mAttachCB( remhostfld_->checked,
		   uiSingleBatchJobDispatcherPars::hostChgCB );
	if ( curhost )
	{
	    const BufferString fullhostnm( curhost->getFullDispString() );
	    remhostfld_->setText( fullhostnm.str() );
	}
    }

    uiSlider::Setup ssu( tr("Job Priority") );
    ssu.nrdec( 7 );
    unixpriofld_ = new uiSlider( this, ssu );
    const StepInterval<int> unixmachpriorg(
			OS::CommandExecPars::cMachineUserPriorityRange(false) );
    StepInterval<float> sliderrg( -1.f, 0.f, 1.f/
				  mCast(float,unixmachpriorg.nrSteps()) );
    unixpriofld_->setInterval( sliderrg );
    unixpriofld_->setValue( js.execpars_.prioritylevel_ );
    if ( remhostfld_ )
	unixpriofld_->attach( alignedBelow, remhostfld_ );

    windowspriofld_ = new uiSlider( this, ssu );
    const StepInterval<int> winmachpriorg(
			OS::CommandExecPars::cMachineUserPriorityRange(true) );
    sliderrg.step = 1.f / mCast(float,winmachpriorg.nrSteps() );
    windowspriofld_->setInterval( sliderrg );
    windowspriofld_->setValue( js.execpars_.prioritylevel_ );
    if ( remhostfld_ )
	windowspriofld_->attach( alignedBelow, remhostfld_ );

    sliderlbl_ = new uiLabel( this, tr("Left:Low, Right: Normal") );
    sliderlbl_->attach( rightOf, unixpriofld_ );

    hostChgCB(nullptr);
}

~uiSingleBatchJobDispatcherPars()
{
    detachAllNotifiers();
}

void hostChgCB( CallBacker* )
{
    const HostData* curhost = hdl_.find( remhostfld_ && remhostfld_->isChecked()
					 ? remhostfld_->text()
					 : GetLocalHostName() );
    if ( !curhost )
    {
#ifdef __win__
	unixpriofld_->display( false );
	windowspriofld_->display( true );
#else
	unixpriofld_->display( true );
	windowspriofld_->display( false );
#endif
	return;
    }

    const bool iswin = curhost->isWindows();

    unixpriofld_->display( !iswin );
    windowspriofld_->display( iswin );
}

bool acceptOK( CallBacker* ) override
{
    if ( remhostfld_ && remhostfld_->isChecked() )
    {
	const HostData* machine = hdl_.find( remhostfld_->text() );
	if ( !machine )
	    return false;

	uiString msg;
	if ( !machine->isOK(msg) )
	    return false;

	sjd_.remotehost_.set( machine->connAddress() );
    }
    else
	sjd_.remotehost_.setEmpty();

    const uiSlider* priofld = windowspriofld_->isDisplayed()
			    ? windowspriofld_
			    : unixpriofld_;
    execpars_.prioritylevel_ = priofld->getFValue();

    return true;
}

    const HostDataList&		hdl_;
    Batch::SingleJobDispatcher&	sjd_;
    OS::CommandExecPars&	execpars_;
    BufferString		defparfnm_;

    uiGenInput*			remhostfld_ = nullptr;
    uiSlider*			unixpriofld_;
    uiSlider*			windowspriofld_;
				/* Flipped priority: Low <---> Normal */
    uiLabel*			sliderlbl_;

};


void uiSingleBatchJobDispatcherLauncher::editOptions( uiParent* p )
{
    hdl_.refresh();
    uiSingleBatchJobDispatcherPars dlg( p, hdl_, sjd_, jobspec_ );
    dlg.go();
}



uiBatchProcDlg::uiBatchProcDlg( uiParent* p, const uiString& dlgnm,
				bool optional, ProcType pt )
    : uiDialog(p,Setup(dlgnm, mNoDlgTitle, mNoHelpKey).modal(false))
{
    setCtrlStyle( RunAndClose );

    pargrp_ = new uiGroup( this, "Parameters group" );

    batchgrp_ = new uiGroup( this, "Batch group" );
    batchgrp_->attach( alignedBelow, pargrp_ );
    batchjobfld_ = new uiBatchJobDispatcherSel( batchgrp_, optional, pt );
    batchgrp_->setHAlignObj( batchjobfld_ );
}


uiBatchProcDlg::~uiBatchProcDlg()
{}


void uiBatchProcDlg::getJobName( BufferString& jobnm ) const
{
    jobnm = "Batch_processing";
}


bool uiBatchProcDlg::acceptOK( CallBacker* )
{
    if ( !prepareProcessing() )
	return false;

    IOPar& par = batchjobfld_->jobSpec().pars_;
    par.setEmpty();
    BufferString jobnm;
    getJobName( jobnm );
    batchjobfld_->setJobName( jobnm );
    if ( !fillPar(par) )
	return false;

    if ( !batchjobfld_->start(&batchid_) )
	uiMSG().error( tr("Could not start batch program") );

    return false;
}


void uiBatchProcDlg::setProgName( const char* prognm )
{
    batchjobfld_->jobSpec().prognm_ = prognm;
}
