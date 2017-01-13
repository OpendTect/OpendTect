/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiseiscopy.h"

#include "seiscopy.h"
#include "keystrs.h"
#include "scaler.h"
#include "ioman.h"
#include "zdomain.h"
#include "seissingtrcproc.h"

#include "uimsg.h"
#include "uiseissel.h"
#include "uiseisioobjinfo.h"
#include "uiseislinesel.h"
#include "uiseistransf.h"
#include "uiscaler.h"
#include "uicombobox.h"
#include "uitaskrunner.h"
#include "uibatchjobdispatchersel.h"
#include "od_helpids.h"

static const char* sProgName = "od_copy_seis";


uiSeisCopyCube::uiSeisCopyCube( uiParent* p, const IOObj* startobj )
    : uiDialog(p,Setup(tr("Copy cube"),mNoDlgTitle,mODHelpKey(mSeisCopyHelpID)))
    , ismc_(false)
{
    setCtrlStyle( RunAndClose );

    IOObjContext inctxt( uiSeisSel::ioContext(Seis::Vol,true) );
    uiSeisSel::Setup sssu( Seis::Vol );
    sssu.steerpol( uiSeisSel::Setup::InclSteer );

    inpfld_ = new uiSeisSel( this, inctxt, sssu );
    inpfld_->selectionDone.notify( mCB(this,uiSeisCopyCube,inpSel) );

    compfld_ = new uiLabeledComboBox( this, tr("Component(s)") );
    compfld_->attach( alignedBelow, inpfld_ );

    uiSeisTransfer::Setup sts( Seis::Vol );
    if ( startobj )
    {
	inpfld_->setInput( startobj->key() );
	SeisIOObjInfo oinf( *startobj );
	sts.zdomkey_ = oinf.zDomainDef().key();
	if ( sts.zdomkey_ != ZDomain::SI().key() )
	    inpfld_->setSensitive( false );
    }
    sts.withnullfill(true).withstep(true).onlyrange(false).fornewentry(true);
    transffld_ = new uiSeisTransfer( this, sts );
    transffld_->attach( alignedBelow, compfld_ );

    IOObjContext outctxt( uiSeisSel::ioContext(Seis::Vol,false) );
    outfld_ = new uiSeisSel( this, outctxt, sssu );
    outfld_->attach( alignedBelow, transffld_ );

    Batch::JobSpec js( sProgName ); js.execpars_.needmonitor_ = true;
    batchfld_ = new uiBatchJobDispatcherSel( this, true, js );
    batchfld_->attach( alignedBelow, outfld_ );

    postFinalise().notify( mCB(this,uiSeisCopyCube,inpSel) );
}


void uiSeisCopyCube::inpSel( CallBacker* cb )
{
    const IOObj* inioobj = inpfld_->ioobj( true );
    ismc_ = false;
    if ( !inioobj )
	return;

    transffld_->updateFrom( *inioobj );

    SeisIOObjInfo oinf( *inioobj );
    ismc_ = oinf.isOK() && oinf.nrComponents() > 1;
    if ( ismc_ )
    {
	BufferStringSet cnms; oinf.getComponentNames( cnms );
	compfld_->box()->setEmpty();
	compfld_->box()->addItem( tr("<All>") );
	compfld_->box()->addItems( cnms );
    }
    compfld_->display( ismc_ );
}


bool uiSeisCopyCube::acceptOK( CallBacker* )
{
    const IOObj* inioobj = inpfld_->ioobj();
    if ( !inioobj )
	return false;
    const IOObj* outioobj = outfld_->ioobj();
    if ( !outioobj )
	return false;

    const int compnr = ismc_ ? compfld_->box()->currentItem()-1 : -1;

    IOPar& outpars = outioobj->pars();
    outpars.addFrom( inioobj->pars() );
    const bool issteer =
	FixedString(sKey::Steering()) == outpars.find( sKey::Type() );
    if ( issteer && compnr>-1 )
	outpars.set( sKey::Type(), sKey::Attribute() );
    IOM().commitChanges( *outioobj );


    if ( batchfld_->wantBatch() )
    {
	Batch::JobSpec& js = batchfld_->jobSpec();
	IOPar inppar; inpfld_->fillPar( inppar );
	inppar.set( sKey::Component(), compnr );
	js.pars_.mergeComp( inppar, sKey::Input() );
	transffld_->fillPar( js.pars_ );
	IOPar outpar; outfld_->fillPar( outpar );
	js.pars_.mergeComp( outpar, sKey::Output() );
	batchfld_->setJobName( outioobj->name() );
	if ( !batchfld_->start() )
	    uiMSG().error( uiStrings::sBatchProgramFailedStart() );

	return false;
    }

    Executor* exec = transffld_->getTrcProc( *inioobj, *outioobj, "",
						uiStrings::sEmptyString() );
    mDynamicCastGet(SeisSingleTraceProc*,stp,exec)
    SeisCubeCopier copier( stp, compnr );
    uiTaskRunner taskrunner( this );
    taskrunner.execute( copier );

    return false;
}


// uiSeisCopy2DDataSet
uiSeisCopy2DDataSet::uiSeisCopy2DDataSet( uiParent* p, const IOObj* obj,
					  const char* fixedoutputtransl )
    : uiDialog(p,
	Setup(uiStrings::phrCopy(uiStrings::sVolDataName(true,false,false)),
	      uiString::emptyString(),mODHelpKey(mSeisCopyLineSetHelpID)))
{
    IOObjContext ioctxt = uiSeisSel::ioContext( Seis::Line, true );
    inpfld_ = new uiSeisSel( this, ioctxt, uiSeisSel::Setup(Seis::Line) );
    inpfld_->selectionDone.notify( mCB(this,uiSeisCopy2DDataSet,inpSel) );

    subselfld_ = new uiSeis2DMultiLineSel( this, uiStrings::phrSelect(
		     tr("%1 to copy").arg(uiStrings::sLine(mPlural))), true );
    subselfld_->attach( alignedBelow, inpfld_ );
    if ( obj )
    {
	inpfld_->setInput( obj->key() );
	subselfld_->setInput( obj->key() );
    }

    scalefld_ = new uiScaler( this, tr("Scale values"), true );
    scalefld_->attach( alignedBelow, subselfld_ );

    ioctxt.forread_ = false;
    if ( fixedoutputtransl )
	ioctxt.fixTranslator( fixedoutputtransl );

    outpfld_ = new uiSeisSel( this, ioctxt, uiSeisSel::Setup(Seis::Line) );
    outpfld_->attach( alignedBelow, scalefld_ );

    Batch::JobSpec js( sProgName ); js.execpars_.needmonitor_ = true;
    batchfld_ = new uiBatchJobDispatcherSel( this, true, js );
    batchfld_->attach( alignedBelow, outpfld_ );
}


void uiSeisCopy2DDataSet::inpSel( CallBacker* )
{
    if ( inpfld_->ioobj(true) )
	subselfld_->setInput( inpfld_->key() );
}


bool uiSeisCopy2DDataSet::acceptOK( CallBacker* )
{
    const IOObj* inioobj = inpfld_->ioobj();
    if ( !inioobj )
	return false;
    const IOObj* outioobj = outpfld_->ioobj();
    if ( !outioobj )
	return false;

    IOPar procpars;
    subselfld_->fillPar( procpars );
    scalefld_->fillPar( procpars );

    if ( batchfld_->wantBatch() )
    {
	Batch::JobSpec& js = batchfld_->jobSpec();
	js.pars_.merge( procpars );
	IOPar inppar; inpfld_->fillPar( inppar );
	js.pars_.mergeComp( inppar, sKey::Input() );
	IOPar outpar; outpfld_->fillPar( outpar );
	js.pars_.mergeComp( outpar, sKey::Output() );

	batchfld_->setJobName( outioobj->name() );
	if ( !batchfld_->start() )
	    uiMSG().error( uiStrings::sBatchProgramFailedStart() );

	return false;
    }

    Seis2DCopier copier( *inioobj, *outioobj, procpars );
    uiTaskRunner taskrunner( this );
    taskrunner.execute( copier );

    return false;
}
