/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uibatchjobdispatchersel.h"
#include "uibatchjobdispatcherlauncher.h"

#include "batchjobdispatch.h"
#include "hostdata.h"

#include "uigeninput.h"
#include "uidialog.h"
#include "uibutton.h"
#include "uislider.h"
#include "uicombobox.h"
#include "uimsg.h"


uiBatchJobDispatcherSel::uiBatchJobDispatcherSel( uiParent* p, bool optional,
					  Batch::JobSpec::ProcType proctyp )
    : uiGroup(p,"Batch job dispatcher selector")
    , jobspec_(proctyp)
    , optsbut_(0)
    , selfld_(0)
    , dobatchbox_(0)
    , selectionChange(this)
{
    Factory1Param<uiBatchJobDispatcherLauncher,Batch::JobSpec&>& fact
				= uiBatchJobDispatcherLauncher::factory();
    const BufferStringSet& nms = fact.getNames();
    for ( int idx=0; idx<nms.size(); idx++ )
    {
	uiBatchJobDispatcherLauncher* dl = fact.create( nms.get(idx), jobspec_);
	if ( dl && (proctyp == Batch::JobSpec::NonODBase
		  || dl->isSuitedFor(jobspec_.prognm_)) )
	    uidispatchers_ += dl;
	else
	    delete dl;
    }

    if ( uidispatchers_.isEmpty() )
	{ pErrMsg("Huh? No dispatcher launchers at all" ); return; }

    BufferString optionsbuttxt( "&Options" );
    const CallBack fldchkcb( mCB(this,uiBatchJobDispatcherSel,fldChck) );
    uiObject* attachobj = 0;
    const bool onlyonechoice = uidispatchers_.size() == 1;
    if ( onlyonechoice )
    {
	if ( !optional )
	    optionsbuttxt.set( "Execution &Options" );
	else
	{
	    dobatchbox_ = new uiCheckBox( this,"Execute in &Batch; Job name");
	    dobatchbox_->activated.notify( fldchkcb );
	    attachobj = dobatchbox_;
	}
    }
    else
    {
	selfld_ = new uiGenInput( this, "Batch execution", StringListInpSpec());
	selfld_->valuechanged.notify( mCB(this,uiBatchJobDispatcherSel,selChg));
	if ( optional )
	{
	    selfld_->setWithCheck( true );
	    selfld_->setChecked( false );
	    selfld_->checked.notify( fldchkcb );
	}
	attachobj = selfld_->attachObj();
    }

    uiLabeledComboBox* lcb = 0;
    if ( onlyonechoice && optional )
    {
	jobnmfld_ = new uiComboBox( this, "Job name" );
	setHAlignObj( jobnmfld_ );
	if ( attachobj )
	    jobnmfld_->attach( rightOf, attachobj );
	attachobj = jobnmfld_;
    }
    else
    {
	lcb = new uiLabeledComboBox( this, "Job name" );
	jobnmfld_ = lcb->box();
	if ( onlyonechoice )
	    setHAlignObj( lcb );
	else
	    setHAlignObj( selfld_ );
	if ( attachobj )
	    lcb->attach( rightOf, attachobj );
	attachobj = lcb->attachObj();
    }
    BufferStringSet unms; Batch::JobDispatcher::getJobNames( unms );
    jobnmfld_->setReadOnly( false );
    jobnmfld_->addItem( "" );
    jobnmfld_->addItems( unms );

    optsbut_ = new uiPushButton( this, optionsbuttxt,
		    mCB(this,uiBatchJobDispatcherSel,optsPush), false );
    optsbut_->attach( rightOf, attachobj );

    postFinalise().notify( mCB(this,uiBatchJobDispatcherSel,initFlds) );
}


void uiBatchJobDispatcherSel::initFlds( CallBacker* )
{
    jobSpecUpdated();
    fldChck( 0 );
}


void uiBatchJobDispatcherSel::setJobSpec( const Batch::JobSpec& js )
{
    jobspec_ = js;
    jobSpecUpdated();
}


void uiBatchJobDispatcherSel::jobSpecUpdated()
{
    if ( !selfld_ )
	return;

    BufferStringSet nms;
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
}


bool uiBatchJobDispatcherSel::wantBatch() const
{
    if ( noLaunchersAvailable() )
	return false;

    if ( selfld_ )
	return !selfld_->isCheckable() || selfld_->isChecked();

    return dobatchbox_ ? dobatchbox_->isChecked() : true;
}


const char* uiBatchJobDispatcherSel::selected() const
{
    const int selidx = selIdx();
    return selidx < 0 ? "" : uidispatchers_[selidx]->name();
}


int uiBatchJobDispatcherSel::selIdx() const
{
    if ( !selfld_ )
	return optsbut_ ? 0 : -1;

    const BufferString cursel = selfld_->text();
    if ( cursel.isEmpty() )
	{ pErrMsg("No dispatchers available"); return -1; }

    for ( int idx=0; idx<uidispatchers_.size(); idx++ )
    {
	if ( cursel == uidispatchers_[idx]->name() )
	    return idx;
    }

    pErrMsg( "Huh? list selection not in factory list" );
    return -1;
}


const char* uiBatchJobDispatcherSel::selectedInfo() const
{
    const int selidx = selIdx();
    return selidx < 0 ? "" : uidispatchers_[selidx]->getInfo();
}


bool uiBatchJobDispatcherSel::start()
{
    const int selidx = selIdx();
    if ( selidx < 0 ) return false;

    uiBatchJobDispatcherLauncher* dl = uidispatchers_[selidx];
    dl->dispatcher().setJobName( jobnmfld_->text() );
    return dl->go( this );
}


void uiBatchJobDispatcherSel::setJobName( const char* nm )
{
    for ( int idx=0; idx<uidispatchers_.size(); idx++ )
	uidispatchers_[idx]->dispatcher().setJobName( nm );
    jobnmfld_->setText( nm );
}


void uiBatchJobDispatcherSel::selChg( CallBacker* )
{
    const int selidx = selIdx();
    uiBatchJobDispatcherLauncher* uidisp = selidx < 0 ? 0
					 : uidispatchers_[selidx];
    optsbut_->display( uidisp ? uidisp->hasOptions() : false );
    if ( uidisp )
	jobnmfld_->setText( uidisp->dispatcher().jobName() );

    fldChck( 0 );
}


void uiBatchJobDispatcherSel::fldChck( CallBacker* )
{
    optsbut_->setSensitive( wantBatch() );
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


bool uiBatchJobDispatcherLauncher::go( uiParent* p )
{
    if ( !dispatcher().go(jobspec_) )
    {
	const char* errmsg = dispatcher().errMsg();
	uiMSG().error( errmsg ? errmsg : "Cannot start required program" );
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
    return isSuitedFor( jobspec_.prognm_ );
}


const char* uiBatchJobDispatcherLauncher::getInfo() const
{
    return dispatcher().description();
}


uiSingleBatchJobDispatcherLauncher::uiSingleBatchJobDispatcherLauncher(
							Batch::JobSpec& js )
    : uiBatchJobDispatcherLauncher(js)
    , sjd_(*new Batch::SingleJobDispatcher)
{
}


uiSingleBatchJobDispatcherLauncher::~uiSingleBatchJobDispatcherLauncher()
{
    delete &sjd_;
}


Batch::JobDispatcher& uiSingleBatchJobDispatcherLauncher::gtDsptchr()
{
    return sjd_;
}


static const float cPrioBound = 19.0f; // happens to be UNIX choice


class uiSingleBatchJobDispatcherPars : public uiDialog
{
public:

uiSingleBatchJobDispatcherPars( uiParent* p, Batch::SingleJobDispatcher& sjd,
				Batch::JobSpec& js )
    : uiDialog(p,Setup("Batch execution parameters", BufferString(
		    "Options for '",js.prognm_,"' program"),
			mTODOHelpID))
    , sjd_(sjd)
    , execpars_(js.execpars_)
    , remhostfld_(0)
{
    Batch::SingleJobDispatcher::getDefParFilename( js.prognm_, defparfnm_ );

    BufferStringSet hnms; HostDataList hdl;
    hdl.fill( hnms, false );
    if ( hnms.size() > 1 )
    {
	remhostfld_ = new uiGenInput( this, "Execute remote",
				      StringListInpSpec(hnms) );
	remhostfld_->setWithCheck( true );
	remhostfld_->setChecked( false );
    }

    uiSliderExtra::Setup ssu( "Job Priority (if available)" );
    // ssu.withedit( true );
    uiSliderExtra* sle = new uiSliderExtra( this, ssu );
    if ( remhostfld_ )
	sle->attach( alignedBelow, remhostfld_ );
    priofld_ = sle->sldr();
    priofld_->setInterval( -cPrioBound, cPrioBound, 1.0f );
    priofld_->setTickMarks( uiSlider::NoMarks );
    priofld_->setValue( execpars_.prioritylevel_ * cPrioBound );
}

bool acceptOK( CallBacker* )
{
    if ( remhostfld_ && remhostfld_->isChecked() )
	sjd_.remotehost_.set( remhostfld_->text() );
    else
	sjd_.remotehost_.setEmpty();

    execpars_.prioritylevel_ = priofld_->getValue() / cPrioBound;

    return true;
}

    Batch::SingleJobDispatcher&	sjd_;
    OS::CommandExecPars&	execpars_;
    BufferString		defparfnm_;

    uiGenInput*			remhostfld_;
    uiSlider*			priofld_;

};


void uiSingleBatchJobDispatcherLauncher::editOptions( uiParent* p )
{
    uiSingleBatchJobDispatcherPars dlg( p, sjd_, jobspec_ );
    dlg.go();
}
