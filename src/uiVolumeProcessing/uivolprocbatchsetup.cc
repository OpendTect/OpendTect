/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uivolprocbatchsetup.h"

#include "ctxtioobj.h"
#include "ioman.h"
#include "ioobj.h"
#include "pixmap.h"
#include "seisselection.h"
#include "volprocchain.h"
#include "volproctrans.h"

#include "uibutton.h"
#include "uibatchjobdispatchersel.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uipossubsel.h"
#include "uiseissel.h"
#include "uiveldesc.h"
#include "uivolprocchain.h"
#include "od_helpids.h"

namespace VolProc
{

uiBatchSetup::uiBatchSetup( uiParent* p, const IOObj* initialsetup )
    : uiDialog( p, uiDialog::Setup("Volume Builder: Create output",
				   mNoDlgTitle, 
                                   mODHelpKey(mVolProcBatchSetupHelpID) ) )
    , chain_( 0 )
{
    IOObjContext setupcontext = VolProcessingTranslatorGroup::ioContext();
    setupcontext.forread = true;
    setupsel_ = new uiIOObjSel( this, setupcontext,
	   			"Volume Builder setup" );
    if ( initialsetup )
	setupsel_->setInput( *initialsetup );
    setupsel_->selectionDone.notify( mCB(this,uiBatchSetup,setupSelCB) );

    editsetup_ = new uiPushButton( this, "Create",
	    ioPixmap(uiChain::pixmapFileName()),
	    mCB(this, uiBatchSetup, editPushCB), false );
    editsetup_->attach( rightOf, setupsel_ );

    possubsel_ = new uiPosSubSel( this, uiPosSubSel::Setup(false,true) );
    possubsel_->attach( alignedBelow, setupsel_ );

    outputsel_ = new uiSeisSel( this, uiSeisSel::ioContext(Seis::Vol,false),
	    			uiSeisSel::Setup(Seis::Vol) );
    outputsel_->attach( alignedBelow, possubsel_ );

    batchfld_ = new uiBatchJobDispatcherSel( this, false,
					     Batch::JobSpec::Vol );
    batchfld_->attach( alignedBelow, outputsel_ );

    setupSelCB( 0 );
}


uiBatchSetup::~uiBatchSetup()
{
    if ( chain_ ) chain_->unRef();
}


bool uiBatchSetup::retrieveChain()
{
    if ( !chain_ )
    {
	chain_ = new Chain;
	chain_->ref();
    }

    const IOObj* setupioobj = setupsel_->ioobj( true );
    if ( !setupioobj || chain_->storageID()==setupioobj->key() )
	return true;

    BufferString errmsg;
    MouseCursorChanger mcc( MouseCursor::Wait );
    const bool res =
	VolProcessingTranslator::retrieve( *chain_, setupioobj, errmsg );
    if ( !res )
    {
	if ( chain_ ) chain_->unRef();
	chain_ = 0;
    }

    return res;
}


void uiBatchSetup::editPushCB( CallBacker* )
{
    if ( !retrieveChain() )
	return;

    uiChain dlg( this, *chain_, false );
    if ( dlg.go() )
	setupsel_->setInput( dlg.storageID() );
} 


bool uiBatchSetup::prepareProcessing()
{
    if ( !setupsel_->ioobj() || !outputsel_->ioobj() )
	return false;

    return true;
}


bool uiBatchSetup::fillPar()
{
    const IOObj* setupioobj = setupsel_->ioobj( true );
    PtrMan<IOObj> outputioobj = outputsel_->getIOObj( true );
    if ( !setupioobj || !outputioobj )
	return false; 

    IOPar& par = batchfld_->jobSpec().pars_;
    par.set( VolProcessingTranslatorGroup::sKeyChainID(), setupioobj->key() );

    // TODO: Make this more general, e.g remove all Attrib related keys
    par.set( "Output.0.Seismic.ID", outputioobj->key() );

    IOPar cspar;
    possubsel_->fillPar( cspar );
    par.mergeComp( cspar, IOPar::compKey(sKey::Output(),sKey::Subsel()) );
    return true;
}


void uiBatchSetup::setupSelCB( CallBacker* )
{
    const IOObj* outputioobj = setupsel_->ioobj( true );
    editsetup_->setText( outputioobj ? "Edit ..." : "Create ..." );

    retrieveChain();
    const bool needsfullvol = chain_ ? chain_->needsFullVolume() : true;
    IOPar& par = batchfld_->jobSpec().pars_;
    par.setYN( Batch::VolMMProgDef::sKeyNeedsFullVolYN(), needsfullvol );
    batchfld_->jobSpecUpdated();
}


bool uiBatchSetup::acceptOK( CallBacker* )
{
    if ( !prepareProcessing() || !fillPar() )
	return false;

    batchfld_->setJobName( outputsel_->ioobj()->name() );
    return batchfld_->start();
}

} // namespace VolProc


// class VolMMProgDef

bool Batch::VolMMProgDef::isSuitedFor( const char* prognm ) const
{
    BufferString pnm( prognm );
    return pnm == Batch::JobSpec::progNameFor( Batch::JobSpec::Vol );
}


bool Batch::VolMMProgDef::canHandle( const Batch::JobSpec& js ) const
{
    const IOPar& par = js.pars_;
    bool needsfullvol = false;
    par.getYN( Batch::VolMMProgDef::sKeyNeedsFullVolYN(), needsfullvol );
    return !needsfullvol;
}
