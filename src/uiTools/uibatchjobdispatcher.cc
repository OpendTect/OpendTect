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

mImplFactory(uiBatchJobDispatcherLauncher,uiBatchJobDispatcherLauncher::factory)


uiBatchJobDispatcherSel::uiBatchJobDispatcherSel( uiParent* p, bool opt )
    : uiGroup(p,"Batch job dispatcher selector")
    , jobspec_(*new Batch::JobSpec)
{
    selfld_ = new uiGenInput( this, "Batch execution" );
    if ( opt )
	{ selfld_->setWithCheck( true ); selfld_->setChecked( false ); }

    Factory<uiBatchJobDispatcherLauncher>& fact
				= uiBatchJobDispatcherLauncher::factory();
    const BufferStringSet& nms = fact.getNames();
    for ( int idx=0; idx<nms.size(); idx++ )
    {
	uiBatchJobDispatcherLauncher* dl = fact.create( nms.get(idx) );
	if ( dl )
	    uidispatchers_ += dl;
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


bool uiBatchJobDispatcherSel::start()
{
    const BufferString cursel = selfld_->text();
    if ( cursel.isEmpty() )
	{ pErrMsg("No dispatchers available"); return false; }

    uiBatchJobDispatcherLauncher* dl = 0;
    for ( int idx=0; idx<uidispatchers_.size(); idx++ )
    {
	if ( uidispatchers_[idx]->name() == cursel )
	    { dl = uidispatchers_[idx]; break; }
    }

    return dl->go( this, jobspec_ );
}


const char* uiSingleBatchJobDispatcherLauncher::getInfo() const
{
    Batch::SingleJobDispatcher sjd;
    mDeclStaticString( ret );
    ret = sjd.description();
    return ret.buf();
}


bool uiSingleBatchJobDispatcherLauncher::go( uiParent* p,
					     const Batch::JobSpec& js )
{
    //TODO implement
    return false;
}
