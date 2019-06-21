/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2014
________________________________________________________________________

-*/

#include "uiseiscopy.h"

#include "ioobj.h"
#include "keystrs.h"
#include "od_helpids.h"
#include "seisioobjinfo.h"
#include "seissingtrcproc.h"

#include "uiseisprovider.h"
#include "uiseisstorer.h"
#include "uibatchjobdispatchersel.h"
#include "uitaskrunner.h"

static const char* sProgName = "od_copy_seis";


uiSeisCopy::uiSeisCopy( uiParent* p, const IOObj* startobj,
			const char* allowtransls )
    : uiDialog(p,Setup(tr("Copy Seismic Data"), mNoDlgTitle,
		mODHelpKey(mSeisCopyHelpID)))
{
    init( startobj, allowtransls );
}


uiSeisCopy::uiSeisCopy( uiParent* p, GeomType gt, const char* allowtransls )
    : uiDialog(p,Setup(tr("Copy Seismic Data"), mNoDlgTitle,
		mODHelpKey(mSeisCopyHelpID)))
{
    init( nullptr, allowtransls, gt );
}


void uiSeisCopy::init( const IOObj* startobj, const char* allowtransls,
			GeomType gt )
{
    setCtrlStyle( RunAndClose );

    uiSeisProvider::Setup provsu;
    if ( !startobj )
	provsu = uiSeisProvider::Setup( gt );
    else
    {
	const SeisIOObjInfo objinf( *startobj );
	if ( objinf.isOK() )
	    provsu = uiSeisProvider::Setup( objinf.geomType() );
    }
    provsu.steerpol( Seis::InclSteer ).compselpol( uiSeisProvider::SomeComps );
    provfld_ = new uiSeisProvider( this, provsu );
    if ( startobj )
	provfld_->set( startobj->key() );

    uiSeisStorer::Setup su;
    su.allowtransls_ = allowtransls;
    storfld_ = new uiSeisStorer( this, provfld_->getGTProv(), su );
    storfld_->attach( alignedBelow, provfld_ );

    Batch::JobSpec js( sProgName ); js.execpars_.needmonitor_ = true;
    batchfld_ = new uiBatchJobDispatcherSel( this, true, js );
    batchfld_->attach( alignedBelow, storfld_ );
}


DBKey uiSeisCopy::copiedID() const
{
    return storfld_->key();
}


bool uiSeisCopy::acceptOK()
{
    if ( !provfld_->isOK() || !storfld_->isOK() )
	return false;

    const bool inbatch = batchfld_->wantBatch();
    if ( inbatch )
    {
	Batch::JobSpec& js = batchfld_->jobSpec();
	IOPar inppar; provfld_->fillPar( inppar );
	js.pars_.mergeComp( inppar, sKey::Input() );
	IOPar outpar; storfld_->fillPar( outpar );
	js.pars_.mergeComp( outpar, sKey::Output() );
	batchfld_->setJobName( copiedID().name() );
	return batchfld_->start();
    }

    SeisSingleTraceProc stp( provfld_->get(false), storfld_->get(false) );
    uiTaskRunner taskrunner( this );
    return taskrunner.execute( stp );
}
