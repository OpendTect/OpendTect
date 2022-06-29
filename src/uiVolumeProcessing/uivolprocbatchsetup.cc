/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/


#include "uivolprocbatchsetup.h"

#include "ctxtioobj.h"
#include "ioman.h"
#include "ioobj.h"
#include "seisjobexecprov.h"
#include "seisselection.h"
#include "volprocchain.h"
#include "volproctrans.h"
#include "volprocchainoutput.h"

#include "uibutton.h"
#include "uibatchjobdispatchersel.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uipixmap.h"
#include "uiseissubsel.h"
#include "uiseissel.h"
#include "uiveldesc.h"
#include "uivolprocchain.h"
#include "uitaskrunner.h"
#include "od_helpids.h"

namespace VolProc
{

uiBatchSetup::uiBatchSetup( uiParent* p, const IOObj* initialsetup, bool is2d )
    : uiDialog(p,uiDialog::Setup(tr("Volume Builder %1: Create output")
				   .arg(is2d?"2D":"3D"),
				 mNoDlgTitle,
				 mODHelpKey(mVolProcBatchSetupHelpID)))
    , chain_(0)
    , is2d_(is2d)
{
    setCtrlStyle( RunAndClose );

    IOObjContext ctxt = is2d ? VolProcessing2DTranslatorGroup::ioContext()
			     : VolProcessingTranslatorGroup::ioContext();
    ctxt.forread_ = true;
    setupsel_ = new uiIOObjSel( this, ctxt, tr("Volume Builder setup") );
    if ( initialsetup )
	setupsel_->setInput( *initialsetup );
    setupsel_->selectionDone.notify( mCB(this,uiBatchSetup,setupSelCB) );

    editsetup_ = new uiPushButton( this, uiStrings::sCreate(),
	    uiPixmap(uiChain::pixmapFileName()),
	    mCB(this, uiBatchSetup, editPushCB), false );
    editsetup_->attach( rightOf, setupsel_ );

    const Seis::GeomType seistype = is2d ? Seis::Line : Seis::Vol;
    Seis::SelSetup selsu( seistype ); selsu.multiline( true );
    subsel_ = uiSeisSubSel::get( this, selsu );
    subsel_->attach( alignedBelow, setupsel_ );

    uiSeisSel::Setup uiselsu( seistype );
    uiselsu.confirmoverwr(!is2d);
    outputsel_ = new uiSeisSel( this, uiSeisSel::ioContext(seistype,false),
				uiselsu );
    outputsel_->attach( alignedBelow, subsel_ );

    batchfld_ = new uiBatchJobDispatcherSel( this, true, Batch::JobSpec::Vol );
    batchfld_->attach( alignedBelow, outputsel_ );
    batchfld_->setWantBatch( true );

    setupSelCB( 0 );
}


uiBatchSetup::~uiBatchSetup()
{
    if ( chain_ ) chain_->unRef();
}


void uiBatchSetup::setIOObj( const IOObj* ioobj )
{
    if ( !ioobj )
	setupsel_->setEmpty();
    else
	setupsel_->setInput( *ioobj );
}


bool uiBatchSetup::retrieveChain()
{
    if ( !chain_ )
    {
	chain_ = new Chain;
	chain_->ref();
    }

    const IOObj* ioobj = setupsel_->ioobj( true );
    if ( !ioobj || chain_->storageID()==ioobj->key() )
	return true;

    uiString errmsg;
    MouseCursorChanger mcc( MouseCursor::Wait );
    if ( !VolProcessingTranslator::retrieve(*chain_,ioobj,errmsg) )
    {
	chain_->unRef();
	chain_ = 0;
	return false;
    }

    return true;
}


void uiBatchSetup::editPushCB( CallBacker* )
{
    if ( !retrieveChain() )
	return;

    uiChain dlg( this, *chain_, false, is2d_ );
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
    par.set( sKey::Target(), outputioobj->name() );

    IOPar subselpar;
    subsel_->fillPar( subselpar );
    par.mergeComp( subselpar, IOPar::compKey(sKey::Output(),sKey::Subsel()) );

    batchfld_->saveProcPars( *outputioobj );
    return true;
}


void uiBatchSetup::setupSelCB( CallBacker* )
{
    const IOObj* outputioobj = setupsel_->ioobj( true );
    editsetup_->setText( outputioobj
			? m3Dots(uiStrings::sEdit())
			: m3Dots(uiStrings::sCreate()) );

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

    const IOObj* outputioobj = outputsel_->ioobj( true );
    if ( !outputioobj )
	return false;

    if ( batchfld_->wantBatch() )
    {
	batchfld_->setJobName( outputioobj->name() );
	if ( !batchfld_->start() )
	    uiMSG().error( uiStrings::sBatchProgramFailedStart() );

	return false;
    }

    VolProc::ChainOutput vco;
    vco.usePar( batchfld_->jobSpec().pars_ );
    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute(vco) ) // TODO: More details in message
	uiMSG().error( tr("Error occured during processing") );

    return false;
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
    if ( !isSuitedFor(js.prognm_) )
	return false;

    const IOPar& par = js.pars_;
    bool needsfullvol = false;
    par.getYN( Batch::VolMMProgDef::sKeyNeedsFullVolYN(), needsfullvol );
    return !needsfullvol;
}


bool Batch::VolMMProgDef::canResume( const JobSpec& js ) const
{
    return canHandle(js) && SeisJobExecProv::isRestart(js.pars_);
}


bool Batch::VolClusterProgDef::isSuitedFor( const char* prognm ) const
{
    BufferString pnm( prognm );
    return pnm == Batch::JobSpec::progNameFor( Batch::JobSpec::Vol );
}


bool Batch::VolClusterProgDef::canHandle( const Batch::JobSpec& js ) const
{
    if ( !isSuitedFor(js.prognm_) )
	return false;

    const IOPar& par = js.pars_;
    bool needsfullvol = false;
    par.getYN( Batch::VolMMProgDef::sKeyNeedsFullVolYN(), needsfullvol );
    return !needsfullvol;
}
