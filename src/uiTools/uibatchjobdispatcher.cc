/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uibatchjobdispatcher.h"

#include "batchjobdispatch.h"

#include "uigeninput.h"
#include "uidialog.h"
#include "uibutton.h"
#include "uimsg.h"

mImplFactory(uiBatchJobDispatcherLauncher,uiBatchJobDispatcherLauncher::factory)



uiBatchJobDispatcherSel::uiBatchJobDispatcherSel( uiParent* p, bool opt )
    : uiGroup(p,"Batch job dispatcher selector")
    , jobspec_(*new Batch::JobSpec)
    , optsbut_(0)
    , selectionChange(this)
{
    Factory<uiBatchJobDispatcherLauncher>& fact
				= uiBatchJobDispatcherLauncher::factory();
    const BufferStringSet& nms = fact.getNames();
    for ( int idx=0; idx<nms.size(); idx++ )
    {
	uiBatchJobDispatcherLauncher* dl = fact.create( nms.get(idx) );
	if ( dl )
	    uidispatchers_ += dl;
    }

    selfld_ = new uiGenInput( this, "Batch execution" );
    selfld_->valuechanged.notify( mCB(this,uiBatchJobDispatcherSel,selChg) );
    if ( opt )
    {
	selfld_->setWithCheck( true );
	selfld_->setChecked( false );
	selfld_->checked.notify( mCB(this,uiBatchJobDispatcherSel,fldChck) );
	optsbut_ = new uiPushButton( this, "&Options",
			mCB(this,uiBatchJobDispatcherSel,optsPush), false );
	optsbut_->attach( rightOf, selfld_ );
    }

    setJobSpec( jobspec_ );
}


uiBatchJobDispatcherSel::~uiBatchJobDispatcherSel()
{
    delete &jobspec_;
}


void uiBatchJobDispatcherSel::setJobSpec( const Batch::JobSpec& js )
{
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

    jobspec_ = js;
}


bool uiBatchJobDispatcherSel::wantBatch() const
{
    return !selfld_->isCheckable() || selfld_->isChecked();
}


const char* uiBatchJobDispatcherSel::selected() const
{
    return selfld_->text();
}


int uiBatchJobDispatcherSel::selIdx() const
{
    const BufferString cursel = selfld_->text();
    if ( cursel.isEmpty() )
	{ pErrMsg("No dispatchers available"); return -1; }

    for ( int idx=0; idx<uidispatchers_.size(); idx++ )
    {
	if ( uidispatchers_[idx]->name() == cursel )
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
    if ( selidx < 0 ) return;
    optsbut_->display( uidispatchers_[selidx]->hasOptions() );
    fldChck( 0 );
}


void uiBatchJobDispatcherSel::fldChck( CallBacker* )
{
    optsbut_->setSensitive( selfld_->isChecked() );
}


void uiBatchJobDispatcherSel::optsPush( CallBacker* )
{
    const int selidx = selIdx();
    if ( uidispatchers_.validIdx(selidx) )
	uidispatchers_[selidx]->editOptions( this );
}


uiSingleBatchJobDispatcherLauncher::uiSingleBatchJobDispatcherLauncher()
    : uiBatchJobDispatcherLauncher(Batch::SingleJobDispatcher::sFactoryKey())
    , execpars_(*new OSCommandExecPars)
{
}


uiSingleBatchJobDispatcherLauncher::~uiSingleBatchJobDispatcherLauncher()
{
    delete &execpars_;
}


const char* uiSingleBatchJobDispatcherLauncher::getInfo() const
{
    Batch::SingleJobDispatcher sjd;
    mDeclStaticString( ret );
    ret = sjd.description();
    return ret.buf();
}


void uiSingleBatchJobDispatcherLauncher::initClass()
{
    uiBatchJobDispatcherLauncher::factory().addCreator( create,
			Batch::SingleJobDispatcher::sFactoryKey() );
}


#include "uilabel.h"

class uiSingleBatchJobDispatcherPars : public uiDialog
{
public:

uiSingleBatchJobDispatcherPars( uiParent* p, OSCommandExecPars& pars )
    : uiDialog(p,Setup("Batch execution parameters",mNoDlgTitle,mTODOHelpID))
    , pars_(pars)
{
    new uiLabel( this, "TODO: implement" );
}

bool acceptOK( CallBacker* )
{
    return true;
}

    OSCommandExecPars& pars_;

};


void uiSingleBatchJobDispatcherLauncher::editOptions( uiParent* p )
{
    uiSingleBatchJobDispatcherPars dlg( p, execpars_ );
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
