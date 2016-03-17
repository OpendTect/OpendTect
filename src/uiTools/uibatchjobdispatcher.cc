/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2014
________________________________________________________________________

-*/

#include "uibatchjobdispatchersel.h"
#include "uibatchjobdispatcherlauncher.h"

#include "batchjobdispatch.h"
#include "hostdata.h"
#include "settings.h"

#include "uigeninput.h"
#include "uidialog.h"
#include "uibutton.h"
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
    if ( (nms.size() > 1)&&
	 ((jobspec_.procTypeFor(jobspec_.prognm_) == Batch::JobSpec::Vol) ||
	 (jobspec_.procTypeFor(jobspec_.prognm_) == Batch::JobSpec::Attrib)) )
	selfld_->setValue( 1 );

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
{ mODTextTranslationClass(uiSingleBatchJobDispatcherPars);
public:

uiSingleBatchJobDispatcherPars( uiParent* p, Batch::SingleJobDispatcher& sjd,
				Batch::JobSpec& js )
    : uiDialog(p,Setup(tr("Batch execution parameters"),
		       tr("Options for '%1' program").arg(js.prognm_),
                       mODHelpKey(mSingleBatchJobDispatcherParsHelpID)))
    , sjd_(sjd)
    , execpars_(js.execpars_)
    , remhostfld_(0)
{
    Batch::SingleJobDispatcher::getDefParFilename( js.prognm_, defparfnm_ );

    BufferStringSet hnms;
    const HostDataList hdl( false );
    hdl.fill( hnms, false );
    if ( !hnms.isEmpty() )
    {
	remhostfld_ = new uiGenInput( this, tr("Execute remote"),
				      StringListInpSpec(hnms) );
	remhostfld_->setWithCheck( true );
	const HostData* curhost = hdl.find( sjd_.remotehost_.str() );
	remhostfld_->setChecked( curhost );
	if ( curhost )
	{
	    const BufferString fullhostnm( curhost->getFullDispString() );
	    remhostfld_->setText( fullhostnm.str() );
	}
    }

    uiSlider::Setup ssu( tr("Job Priority (if available)") );
    ssu.withedit( true );
    priofld_ = new uiSlider( this, ssu );
    if ( remhostfld_ )
	priofld_->attach( alignedBelow, remhostfld_ );

    priofld_->setInterval( -cPrioBound, cPrioBound, 1.0f );
    priofld_->setTickMarks( uiSlider::NoMarks );
    priofld_->setValue( execpars_.prioritylevel_ * cPrioBound );
}

bool acceptOK( CallBacker* )
{
    if ( remhostfld_ && remhostfld_->isChecked() )
    {
	const HostDataList hdl( false );
	const HostData* curhost = hdl.find( remhostfld_->text() );
	if ( !curhost )
	    return false;

	if ( curhost->getHostName() )
	    sjd_.remotehost_.set( curhost->getHostName() );
	else if ( curhost->getIPAddress() )
	    sjd_.remotehost_.set( curhost->getIPAddress() );
	else
	    return false;
    }
    else
	sjd_.remotehost_.setEmpty();

    execpars_.prioritylevel_ = priofld_->getFValue() / cPrioBound;

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
