/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseiscopy.h"

#include "ioman.h"
#include "keystrs.h"
#include "od_helpids.h"
#include "seiscopy.h"
#include "seissingtrcproc.h"
#include "survinfo.h"
#include "zdomain.h"

#include "uibatchjobdispatchersel.h"
#include "uicombobox.h"
#include "uimsg.h"
#include "uiscaler.h"
#include "uiseisioobjinfo.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uiseistransf.h"
#include "uitaskrunner.h"

static const char* sProgName = "od_copy_seis";


uiSeisCopyCube::uiSeisCopyCube( uiParent* p, const IOObj* startobj )
    : uiDialog(p,Setup(tr("Copy cube"),mNoDlgTitle,mODHelpKey(mSeisCopyHelpID)))
    , ismc_(false)
{
    setCtrlStyle( RunAndClose );

    const Seis::GeomType gt = Seis::Vol;
    IOObjContext inctxt( uiSeisSel::ioContext(gt,true) );
    uiSeisSel::Setup sssu( gt );
    sssu.steerpol( uiSeisSel::Setup::InclSteer ).enabotherdomain( true );

    inpfld_ = new uiSeisSel( this, inctxt, sssu );
    if ( startobj )
	inpfld_->setInput( startobj->key() );
    mAttachCB( inpfld_->selectionDone, uiSeisCopyCube::inpSelCB );

    compfld_ = new uiLabeledComboBox( this, tr("Component(s)") );
    compfld_->attach( alignedBelow, inpfld_ );
    mAttachCB( compfld_->box()->selectionChanged, uiSeisCopyCube::compSel );

    uiSeisTransfer::Setup sts( gt );
    sts.withnullfill( true ).withmultiz( true )
       .withstep( true ).onlyrange( false ).fornewentry( true );
    transffld_ = new uiSeisTransfer( this, sts );
    transffld_->attach( alignedBelow, compfld_ );

    IOObjContext outctxt( uiSeisSel::ioContext(gt,false) );
    outfld_ = new uiSeisSel( this, outctxt, sssu );
    outfld_->attach( alignedBelow, transffld_ );

    Batch::JobSpec js( sProgName );
    js.execpars_.needmonitor_ = true;
    batchfld_ = new uiBatchJobDispatcherSel( this, true, js );
    batchfld_->attach( alignedBelow, outfld_ );

    mAttachCB( postFinalize(), uiSeisCopyCube::initDlgCB );
}


uiSeisCopyCube::~uiSeisCopyCube()
{
    detachAllNotifiers();
}


void uiSeisCopyCube::initDlgCB( CallBacker* )
{
    inpSelCB( nullptr );
}


void uiSeisCopyCube::inpSelCB( CallBacker* )
{
    const IOObj* inioobj = inpfld_->ioobj( true );
    ismc_ = false;
    if ( !inioobj )
	return;

    transffld_->updateFrom( *inioobj );

    const SeisIOObjInfo oinf( *inioobj );
    ismc_ = oinf.isOK() && oinf.nrComponents() > 1;
    if ( ismc_ )
    {
	BufferStringSet cnms; oinf.getComponentNames( cnms );
	compfld_->box()->setEmpty();
	compfld_->box()->addItem( tr("<All>") );
	compfld_->box()->addItems( cnms );
    }

    compfld_->display( ismc_ );
    outfld_->updateOutputOpts( ismc_ );
    const ZDomain::Info* zinfo = transffld_->zDomain();
    outfld_->setZDomain( zinfo ? *zinfo : SI().zDomainInfo() );
}


void uiSeisCopyCube::compSel( CallBacker* )
{
    const int sel = compfld_->box()->currentItem();
    outfld_->updateOutputOpts( sel == 0 );
}


bool uiSeisCopyCube::acceptOK( CallBacker* )
{
    const IOObj* inioobj = inpfld_->ioobj();
    if ( !inioobj )
	return false;

    const IOObj* outioobj = outfld_->ioobj( true );
    if ( !outioobj )
	return false;

    if ( outioobj->implExists(false) &&
	    !uiMSG().askOverwrite(uiStrings::sOutputFileExistsOverwrite()) )
	return false;

    IOPar& outpars = outioobj->pars();
    outpars.addFrom( inioobj->pars() );
    const uiSeisIOObjInfo ioobjinfo( *outioobj, true );
    SeisIOObjInfo::SpaceInfo spi( transffld_->spaceInfo() );
    if ( !ioobjinfo.checkSpaceLeft(spi) )
	return false;

    const bool issteer = outpars.find( sKey::Type() ).
						isEqual( sKey::Steering() );
    const int compnr = ismc_ ? compfld_->box()->currentItem()-1 : -1;
    if ( issteer && compnr>-1 )
	outpars.set( sKey::Type(), sKey::Attribute() );

    IOM().commitChanges( *outioobj );

    if ( batchfld_->wantBatch() )
    {
	const BufferString jobname( "Copy_", outioobj->name() );
	batchfld_->setJobName( jobname );
	IOPar& jobpars = batchfld_->jobSpec().pars_;
	jobpars.setEmpty();
	const Seis::GeomType gt = inpfld_->geomType();
	Seis::putInPar( gt, jobpars );
	jobpars.set( "Task", "Copy" );

	IOPar inppar;
	inpfld_->fillPar( inppar );
	if ( compnr > 0 )
	    inppar.set( sKey::Component(), compnr );

	jobpars.mergeComp( inppar, sKey::Input() );

	IOPar outpar;
	transffld_->fillPar( outpar );
	outfld_->fillPar( outpar );
	jobpars.mergeComp( outpar, sKey::Output() );

	batchfld_->saveProcPars( *outioobj );
	if ( !batchfld_->start() )
	    uiMSG().error( uiStrings::sBatchProgramFailedStart() );

	return false;
    }

    PtrMan<Executor> exec = transffld_->getTrcProc( *inioobj, *outioobj,
						"Copying 3D Cube",
						uiStrings::sEmptyString(),
						compnr );
    mDynamicCastGet(SeisSingleTraceProc*,stp,exec.ptr())
    if ( !stp )
	return false;

    PtrMan<SeisSingleTraceProc> workstp( stp );
    exec.release();
    if ( !workstp->isOK() )
    {
	uiMSG().error( stp->errMsg() );
	return false;
    }

    SeisCubeCopier copier( workstp.release(), compnr );
    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute(copier) )
    {
	uiMSG().error( copier.uiMessage() );
	return false;
    }

    const uiString msg = tr( "%1 successfully copied.\n\n"
			     "Do you want to copy more %2?" )
			     .arg(outioobj->name()).arg(outioobj->group());
    const bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				      tr("No, close window") );

    return !ret;
}


// uiSeisCopy2DDataSet

uiSeisCopy2DDataSet::uiSeisCopy2DDataSet( uiParent* p, const IOObj* obj,
					  const char* fixedoutputtransl )
    : uiDialog(p,
	Setup(uiStrings::phrCopy(uiStrings::sVolDataName(true,false,false)),
	      uiString::empty(),mODHelpKey(mSeisCopyLineSetHelpID)))
{
    setCtrlStyle( RunAndClose );

    const Seis::GeomType gt = Seis::Line;

    IOObjContext ioctxt = uiSeisSel::ioContext( gt, true );
    uiSeisSel::Setup sssu( gt );
    sssu.steerpol( uiSeisSel::Setup::InclSteer ).enabotherdomain( true );
    inpfld_ = new uiSeisSel( this, ioctxt, sssu );
    mAttachCB( inpfld_->selectionDone, uiSeisCopy2DDataSet::inpSelCB );

    Seis::SelSetup selsu( gt );
    selsu.multiline( true ).seltxt( uiStrings::phrSelect(
		tr("%1 to copy").arg(uiStrings::sLine(mPlural).toLower()) ) );
    subselfld_ = new uiMultiZSeisSubSel( this, selsu );
    subselfld_->attach( alignedBelow, inpfld_ );
    if ( obj )
	inpfld_->setInput( obj->key() );

    scalefld_ = new uiScaler( this, tr("Scale values"), true );
    scalefld_->attach( alignedBelow, subselfld_ );

    ioctxt.forread_ = false;
    if ( fixedoutputtransl )
	ioctxt.fixTranslator( fixedoutputtransl );

    outpfld_ = new uiSeisSel( this, ioctxt, sssu );
    outpfld_->attach( alignedBelow, scalefld_ );

    Batch::JobSpec js( sProgName ); js.execpars_.needmonitor_ = true;
    batchfld_ = new uiBatchJobDispatcherSel( this, true, js );
    batchfld_->attach( alignedBelow, outpfld_ );

    mAttachCB( postFinalize(), uiSeisCopy2DDataSet::initDlgCB );
}


uiSeisCopy2DDataSet::~uiSeisCopy2DDataSet()
{
    detachAllNotifiers();
}


void uiSeisCopy2DDataSet::initDlgCB( CallBacker* )
{
    inpSelCB( nullptr );
}


void uiSeisCopy2DDataSet::inpSelCB( CallBacker* )
{
    const IOObj* obj = inpfld_->ioobj( true );
    if ( !obj )
	return;

    subselfld_->setInput( *obj );
    const ZDomain::Info* zinfo = subselfld_->zDomain();
    outpfld_->setZDomain( zinfo ? *zinfo : SI().zDomainInfo() );
}


bool uiSeisCopy2DDataSet::acceptOK( CallBacker* )
{
    const IOObj* inioobj = inpfld_->ioobj();
    if ( !inioobj )
	return false;

    const IOObj* outioobj = outpfld_->ioobj( true );
    if ( !outioobj )
	return false;

    if ( outioobj->implExists(false) )
    {
	TypeSet<Pos::GeomID> ingeomids;
	mDynamicCastGet(const uiSeis2DSubSel*,subselfld,
			subselfld_->getSelGrp());
	subselfld->selectedGeomIDs( ingeomids );

	TypeSet<Pos::GeomID> outgeomids;
	const SeisIOObjInfo outinfo( outioobj );
	outinfo.getGeomIDs( outgeomids );

	bool haveoverlap = false;
	for ( int idx=0; idx<ingeomids.size(); idx++ )
	    if ( outgeomids.isPresent(ingeomids[idx]) )
		{ haveoverlap = true; break; }

	if ( haveoverlap &&
		!uiMSG().askOverwrite(uiStrings::sOutputFileExistsOverwrite()) )
	    return false;
    }

    IOPar& outpars = outioobj->pars();
    outpars.addFrom( inioobj->pars() );

    IOPar procpars;
    subselfld_->getSelGrp()->fillPar( procpars );
    scalefld_->fillPar( procpars );

    if ( batchfld_->wantBatch() )
    {
	const BufferString jobname( "Copy_", outioobj->name() );
	batchfld_->setJobName( jobname );
	IOPar& jobpars = batchfld_->jobSpec().pars_;
	jobpars.setEmpty();
	const Seis::GeomType gt = inpfld_->geomType();
	Seis::putInPar( gt, jobpars );
	jobpars.set( "Task", "Copy" );

	IOPar inppar;
	inpfld_->fillPar( inppar );
	jobpars.mergeComp( inppar, sKey::Input() );

	IOPar outpar( procpars );
	outpfld_->fillPar( outpar );
	jobpars.mergeComp( outpar, sKey::Output() );

	if ( !batchfld_->start() )
	    uiMSG().error( uiStrings::sBatchProgramFailedStart() );

	return false;
    }

    Seis2DCopier copier( *inioobj, *outioobj, procpars );
    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute(copier) )
	{ uiMSG().error( copier.uiMessage() ); return false; }

    const uiString msg = tr( "%1 successfully copied.\n\n"
			     "Do you want to copy more %2?" )
			     .arg(outioobj->name()).arg(outioobj->group());
    const bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				      tr("No, close window") );
    return !ret;
}
