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

#include "hostdata.h"
#include "settings.h"
#include "singlebatchjobdispatch.h"

#include "uigeninput.h"
#include "uidialog.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uislider.h"
#include "uicombobox.h"
#include "uimsg.h"
#include "uistrings.h"

uiBatchJobDispatcherSel::uiBatchJobDispatcherSel( uiParent* p, bool optional,
						  const Batch::JobSpec& js )
    : uiGroup(p,"Batch job dispatcher selector")
    , jobspec_(js)
    , optsbut_(0)
    , selfld_(0)
    , dobatchbox_(0)
    , selectionChange(this)
    , checked(this)
    , jobname_("batch_processing")
{
    init( optional );
}


uiBatchJobDispatcherSel::uiBatchJobDispatcherSel( uiParent* p, bool optional,
					  Batch::JobSpec::ProcType proctyp )
    : uiGroup(p,"Batch job dispatcher selector")
    , jobspec_(proctyp)
    , optsbut_(0)
    , selfld_(0)
    , dobatchbox_(0)
    , selectionChange(this)
    , checked(this)
    , jobname_("batch_processing")
{
    init( optional );
}


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
	{ pErrMsg("Huh? No dispatcher launchers at all"); return; }

    uiString optionsbuttxt = uiStrings::sOptions();
    const CallBack fldchkcb( mCB(this,uiBatchJobDispatcherSel,fldChck) );
    uiObject* attachobj = 0;
    const bool onlyonechoice = uidispatchers_.size() == 1;
    if ( onlyonechoice )
    {
	if ( !optional )
	    optionsbuttxt = uiStrings::phrJoinStrings(tr("Execution"),
					uiStrings::sOptions());
	else
	{
	    dobatchbox_ = new uiCheckBox( this, tr("Execute in Batch") );
	    dobatchbox_->activated.notify( fldchkcb );
	    setHAlignObj( dobatchbox_ );
	    attachobj = dobatchbox_;
	}
    }
    else
    {
	selfld_ = new uiGenInput( this, tr("Batch execution"),
				  StringListInpSpec());
	selfld_->valuechanged.notify( mCB(this,uiBatchJobDispatcherSel,selChg));
	setHAlignObj( selfld_ );
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
    else
	setHAlignObj( optsbut_ );

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
    return selidx < 0 ? uiString::emptyString()
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
	if ( cursel == uidispatchers_[idx]->name().getFullString() )
	    return idx;
    }

    pErrMsg( "Huh? list selection not in factory list" );
    return -1;
}


const uiString uiBatchJobDispatcherSel::selectedInfo() const
{
    const int selidx = selIdx();
    return selidx < 0 ? uiString::emptyString()
		      : uidispatchers_[selidx]->getInfo();
}


bool uiBatchJobDispatcherSel::start()
{
    const int selidx = selIdx();
    if ( selidx < 0 ) return false;

    uiBatchJobDispatcherLauncher* dl = uidispatchers_[selidx];
    dl->dispatcher().setJobName( jobname_.buf() );
    return dl->go( this );
}


void uiBatchJobDispatcherSel::setJobName( const char* nm )
{ jobname_ = nm; }


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


bool uiBatchJobDispatcherLauncher::go( uiParent* p )
{
    if ( !dispatcher().go(jobspec_) )
    {
	uiString errmsg = dispatcher().errMsg();
	uiMSG().error(
		errmsg.isSet() ? errmsg : tr("Cannot start required program") );
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
    , sjd_(*new Batch::SingleJobDispatcherRemote)
{
#ifdef __unix__
    const HostDataList hdl( false );
    const int niceval = hdl.niceLevel();
    const StepInterval<int> nicerg(
		    OS::CommandExecPars::cMachineUserPriorityRange( false ) );
    jobspec_.execpars_.prioritylevel_ =
				-1.f * mCast(float,niceval) / nicerg.width();
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


static const float cPrioBound = 19.0f; // happens to be UNIX choice


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
    , remhostfld_(0)
{
    Batch::SingleJobDispatcher::getDefParFilename( js.prognm_, defparfnm_ );

    BufferStringSet hnms;
    hdl_.fill( hnms, false );
    if ( !hnms.isEmpty() )
    {
	remhostfld_ = new uiGenInput( this, tr("Execute remote"),
				      StringListInpSpec(hnms) );
	remhostfld_->setWithCheck( true );
	const HostData* curhost = hdl_.find( sjd_.remotehost_.str() );
	remhostfld_->setChecked( curhost );
	remhostfld_->valuechanged.notify(
		     mCB(this,uiSingleBatchJobDispatcherPars,hostChgCB) );
	remhostfld_->checked.notify(
		     mCB(this,uiSingleBatchJobDispatcherPars,hostChgCB) );
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

    hostChgCB(0);
}

void hostChgCB( CallBacker* )
{
    const HostData* curhost = hdl_.find( remhostfld_ && remhostfld_->isChecked()
					 ? remhostfld_->text()
					 : HostData::localHostName() );
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

bool acceptOK( CallBacker* )
{
    if ( remhostfld_ && remhostfld_->isChecked() )
    {
	const HostData* machine = hdl_.find( remhostfld_->text() );
	if ( !machine )
	    return false;

	if ( machine->getHostName() )
	    sjd_.remotehost_.set( machine->getHostName() );
	else if ( machine->getIPAddress() )
	    sjd_.remotehost_.set( machine->getIPAddress() );
	else
	    return false;
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

    uiGenInput*			remhostfld_;
    uiSlider*			unixpriofld_;
    uiSlider*			windowspriofld_;
				/* Flipped priority: Low <---> Normal */
    uiLabel*			sliderlbl_;

};


void uiSingleBatchJobDispatcherLauncher::editOptions( uiParent* p )
{
    const HostDataList hdl( false );
    uiSingleBatchJobDispatcherPars dlg( p, hdl, sjd_, jobspec_ );
    dlg.go();
}
