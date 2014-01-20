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

#include "uigeninput.h"
#include "uidialog.h"
#include "uibutton.h"
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
    Factory<uiBatchJobDispatcherLauncher>& fact
				= uiBatchJobDispatcherLauncher::factory();
    const BufferStringSet& nms = fact.getNames();
    for ( int idx=0; idx<nms.size(); idx++ )
    {
	uiBatchJobDispatcherLauncher* dl = fact.create( nms.get(idx) );
	if ( dl && (proctyp == Batch::JobSpec::NonODBase
		  || dl->isSuitedFor(jobspec_.prognm_)) )
	    uidispatchers_ += dl;
    }

    if ( uidispatchers_.isEmpty() )
	{ pErrMsg("Huh? No dispatcher launchers at all" ); return; }

    BufferString optionsbuttxt( "&Options" );
    const CallBack fldchkcb( mCB(this,uiBatchJobDispatcherSel,fldChck) );
    uiObject* optattachobj = 0;
    if ( uidispatchers_.size() == 1 )
    {
	if ( !optional )
	    optionsbuttxt.set( "Batch execution &Options" );
	else
	{
	    dobatchbox_ = new uiCheckBox( this, "Execute in &Batch" );
	    dobatchbox_->activated.notify( fldchkcb );
	    optattachobj = dobatchbox_;
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
	optattachobj = selfld_->attachObj();
    }

    optsbut_ = new uiPushButton( this, "&Options",
		    mCB(this,uiBatchJobDispatcherSel,optsPush), false );
    if ( optattachobj )
	optsbut_->attach( rightOf, optattachobj );

    setHAlignObj( optattachobj ? optattachobj : (uiObject*)optsbut_ );

    postFinalise().notify( mCB(this,uiBatchJobDispatcherSel,initFlds) );
}


void uiBatchJobDispatcherSel::initFlds( CallBacker* )
{
    setJobSpec( jobspec_ );
    fldChck( 0 );
}


void uiBatchJobDispatcherSel::setJobSpec( const Batch::JobSpec& js )
{
    jobspec_ = js;
    if ( !selfld_ )
	return;

    BufferStringSet nms;
    for ( int idx=0; idx<uidispatchers_.size(); idx++ )
    {
	if ( uidispatchers_[idx]->canHandle( js ) )
	    nms.add( uidispatchers_[idx]->name() );
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
    return dl->go( this, jobspec_ );
}


void uiBatchJobDispatcherSel::selChg( CallBacker* )
{
    const int selidx = selIdx();
    optsbut_->display( selidx < 0 ? false
				  : uidispatchers_[selidx]->hasOptions() );
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

mImplFactory(uiBatchJobDispatcherLauncher,uiBatchJobDispatcherLauncher::factory)


bool uiBatchJobDispatcherLauncher::canHandle( const Batch::JobSpec& js ) const
{
    return isSuitedFor( js.prognm_ );
}


const char* uiSingleBatchJobDispatcherLauncher::getInfo() const
{
    Batch::SingleJobDispatcher sjd;
    mDeclStaticString( ret );
    ret = sjd.description();
    return ret.buf();
}


#include "uilabel.h"

class uiSingleBatchJobDispatcherPars : public uiDialog
{
public:

uiSingleBatchJobDispatcherPars( uiParent* p, OS::CommandExecPars& pars )
    : uiDialog(p,Setup("Batch execution parameters",mNoDlgTitle,mTODOHelpID))
    , pars_(pars)
{
    new uiLabel( this, "TODO: implement" );
}

bool acceptOK( CallBacker* )
{
    return true;
}

    OS::CommandExecPars& pars_;

};


void uiSingleBatchJobDispatcherLauncher::editOptions(
				uiBatchJobDispatcherSel* p )
{
    uiSingleBatchJobDispatcherPars dlg( p, p->jobSpec().execpars_ );
    dlg.go();
}


bool uiSingleBatchJobDispatcherLauncher::go( uiParent* p,
					     const Batch::JobSpec& js )
{
    Batch::SingleJobDispatcher sjd;
    if ( !sjd.go(js) )
    {
	const char* errmsg = sjd.errMsg();
	uiMSG().error( errmsg ? errmsg : "Cannot start batch program" );
	return false;
    }
    return true;
}
