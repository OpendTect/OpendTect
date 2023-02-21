/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseis2dto3d.h"

#include "uibatchjobdispatchersel.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uipossubsel.h"
#include "uiseisioobjinfo.h"
#include "uiseissel.h"
#include "uiseissubsel.h"

#include "trckeyzsampling.h"
#include "od_helpids.h"
#include "seis2dto3d.h"
#include "seisjobexecprov.h"
#include "survinfo.h"


uiSeis2DTo3D::uiSeis2DTo3D( uiParent* p )
	: uiDialog( p, Setup( tr("Create 3D cube from to 2DDataSet"),
			      mNoDlgTitle,
			      mODHelpKey(mSeis2DTo3DHelpID) ) )
{
    inpfld_ = new uiSeisSel( this, uiSeisSel::ioContext(Seis::Line,true),
			     uiSeisSel::Setup(Seis::Line) );

    mkParamsGrp();

    outfld_ = new uiSeisSel( this, uiSeisSel::ioContext(Seis::Vol,false),
			     uiSeisSel::Setup(Seis::Vol) );
    outfld_->attach( alignedBelow, velfiltfld_ );

    possubsel_ =  new uiPosSubSel( this, uiPosSubSel::Setup(false,true) );
    possubsel_->attach( alignedBelow, outfld_ );

    batchfld_ = new uiBatchJobDispatcherSel( this, false,
					     Batch::JobSpec::TwoDto3D );
    IOPar& iop = jobSpec().pars_;
    iop.set( IOPar::compKey(sKey::Output(),sKey::Type()), "Cube" );
    batchfld_->attach( alignedBelow, possubsel_ );

    typeChg( 0 );
}


uiSeis2DTo3D::~uiSeis2DTo3D()
{}


Batch::JobSpec& uiSeis2DTo3D::jobSpec()
{
    return batchfld_->jobSpec();
}


#define mErrRet(s) { uiMSG().error(s); return false; }
bool uiSeis2DTo3D::prepareProcessing()
{
    const IOObj* inioobj = inpfld_->ioobj();
    const IOObj* outioobj = outfld_->ioobj();
    if ( !inioobj || !outioobj )
	return false;

    inpfld_->processInput();
    if ( !inpfld_->existingTyped() )
	mErrRet( tr("Missing Input\nPlease select the input seismics") )

    outfld_->processInput();
    if ( !outfld_->existingTyped() )
	mErrRet( tr("Missing Output\nPlease enter an output name") )

    return true;
}


void uiSeis2DTo3D::mkParamsGrp()
{
    interpoltypefld_ = new uiGenInput( this, tr("Type of interpolation"),
			BoolInpSpec(true,tr("Nearest trace"),tr("FFT based")) );
    interpoltypefld_->attach( alignedBelow, inpfld_ );
    interpoltypefld_->valueChanged.notify(mCB(this,uiSeis2DTo3D,typeChg));

    winfld_ = new uiGenInput( this,tr("Interpolation window (Inl/Crl)"),
							IntInpIntervalSpec() );
    winfld_->attach( alignedBelow, interpoltypefld_ );
    winfld_->setValue( Interval<float>(100,100) );

    reusetrcsbox_ = new uiCheckBox( this, tr("Re-use interpolated traces") );
    reusetrcsbox_->attach( alignedBelow, winfld_ );

    velfiltfld_ = new uiGenInput( this, tr("Maximum velocity to pass (m/s)") );
    velfiltfld_->setValue( 2000 );
    velfiltfld_->attach( alignedBelow, reusetrcsbox_ );
}


bool uiSeis2DTo3D::fillSeisPar()
{
    IOPar& iop = jobSpec().pars_;
    iop.set( Seis2DTo3D::sKeyInput(), inpfld_->key() );
    iop.set( SeisJobExecProv::sKeySeisOutIDKey(), outfld_->key() );
    iop.set( Seis2DTo3D::sKeyCreaterType(), Seis2DTo3D::getCreatorFormat() );

    IOPar sampling;
    possubsel_->fillPar( sampling );

    IOPar subsel;
    subsel.mergeComp( sampling, sKey::Subsel() );
    uiSeisIOObjInfo ioobjinfo( *(outfld_->ioobj()), true );
    TrcKeyZSampling cs = possubsel_->envelope();
    SeisIOObjInfo::SpaceInfo spi( cs.nrZ(), (int)cs.hsamp_.totalNr() );
    subsel.set( "Estimated MBs", ioobjinfo.expectedMBs(spi) );
    if ( !ioobjinfo.checkSpaceLeft(spi) )
	return false;

    iop.mergeComp( subsel, sKey::Output() );
    return true;
}


void uiSeis2DTo3D::fillParamsPar( IOPar& par )
{
    const bool isnearest = interpoltypefld_->getBoolValue();
    par.setYN( Seis2DTo3D::sKeyIsNearest(), isnearest );
    if ( !isnearest )
    {
	par.set( Seis2DTo3D::sKeyStepout(), winfld_->getIInterval() );
	par.setYN( Seis2DTo3D::sKeyReUse(), reusetrcsbox_->isChecked() );
	par.set( Seis2DTo3D::sKeyMaxVel(), velfiltfld_->getFValue() );
    }
}


bool uiSeis2DTo3D::fillPar()
{
    if ( !fillSeisPar() )
	return false;

    IOPar par;
    fillParamsPar( par );
    jobSpec().pars_.mergeComp( par, sKey::Pars() );
    batchfld_->saveProcPars( *outfld_->ioobj() );
    return true;
}


bool uiSeis2DTo3D::acceptOK( CallBacker* )
{
    if ( !prepareProcessing() || !fillPar() )
	return false;

    if ( !SI().has3D() )
	uiMSG().warning( tr( "After processing you will need to change\n"
			     "the survey type to 'Both 2D and 3D'\n"
			     "in survey setup to display/use the cube" ) );

    batchfld_->setJobName( outfld_->getInput() );
    return batchfld_->start();
}


void uiSeis2DTo3D::typeChg( CallBacker* )
{
    bool isfft = !interpoltypefld_->getBoolValue();
    winfld_->display( isfft );
    reusetrcsbox_->display( isfft );
    velfiltfld_->display( isfft );
}
