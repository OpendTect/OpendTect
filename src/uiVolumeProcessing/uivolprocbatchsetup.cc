/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/


#include "uivolprocbatchsetup.h"

#include "ioobjctxt.h"
#include "dbman.h"
#include "ioobj.h"
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

uiBatchSetup::uiBatchSetup( uiParent* p, bool is2d, const IOObj* initialsetup )
    : uiDialog( p, uiDialog::Setup(tr("Volume Builder: Create output"),
				   mNoDlgTitle,
                                   mODHelpKey(mVolProcBatchSetupHelpID) ) )
    , chain_( 0 )
    , is2d_( is2d )
{
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

    outputsel_ = new uiSeisSel( this, uiSeisSel::ioContext(seistype,false),
				uiSeisSel::Setup(Seis::Vol) );
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

    uiChain dlg( this, *chain_, is2d_, false );
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

    IOPar subselpar;
    mDynamicCastGet(uiSeis2DSubSel*,subsel2d,subsel_)
    if ( subsel2d )
    {
	TypeSet<Pos::GeomID> geomids;
	subsel2d->selectedGeomIDs( geomids );
	subselpar.set( sKey::NrGeoms(), geomids.size() );
	for ( int idx=0; idx<geomids.size(); idx++ )
	{
	    TrcKeyZSampling tkzs;
	    subsel2d->getSampling( tkzs, geomids[idx] );
	    IOPar tkzspar;
	    tkzs.fillPar( tkzspar );
	    subselpar.mergeComp( tkzspar, toString(idx) );
	}
    }
    else
    {
	subselpar.set( sKey::NrGeoms(), 1 );
	TrcKeyZSampling tkzs;
	subsel_->getSampling( tkzs );
	IOPar tkzspar;
	tkzs.fillPar( tkzspar );
	subselpar.mergeComp( tkzspar, toString(0) );
    }

    par.mergeComp( subselpar, IOPar::compKey(sKey::Output(),sKey::Subsel()) );
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


bool uiBatchSetup::acceptOK()
{
    if ( !prepareProcessing() || !fillPar() )
	return false;

    const IOObj* outputioobj = setupsel_->ioobj( true );
    if ( !outputioobj ) return false;

    if ( batchfld_->wantBatch() )
    {
	batchfld_->setJobName( outputioobj->name() );
	return batchfld_->start();
    }

    VolProc::ChainOutput vco;
    vco.usePar( batchfld_->jobSpec().pars_ );
    uiTaskRunner taskrunner( this );
    return taskrunner.execute( vco );
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
