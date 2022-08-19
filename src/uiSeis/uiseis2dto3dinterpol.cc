/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseis2dto3dinterpol.h"

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
#include "seis2dto3dinterpol.h"
#include "seisjobexecprov.h"
#include "survinfo.h"


uiSeis2DTo3DInterPol::uiSeis2DTo3DInterPol(uiParent* p, uiString& titletext)
	: uiDialog( p, Setup( titletext,
			      mNoDlgTitle,
			      mODHelpKey(mSeis2DTo3DHelpID) ) )
{
    uiSeisSel::Setup sssu( Seis::Line );
    sssu.steerpol( uiSeisSel::Setup::InclSteer );
    sssu.enabotherdomain(true);
    inpfld_ = new uiSeisSel( this, uiSeisSel::ioContext(Seis::Line,true),
			     sssu );

	powfld_ = new uiGenInput( this, tr("Operator decay"));
    powfld_->attach( alignedBelow , inpfld_);
    powfld_->setValue(2);

    taperfld_ = new uiGenInput( this, tr("Taper angle"));
    taperfld_->attach( alignedBelow , powfld_);
    taperfld_->setValue(0);

    smrtscalebox_ = new uiCheckBox (this , tr("Smart scaling"));
    smrtscalebox_->attach(alignedBelow , taperfld_);
    smrtscalebox_->setChecked(true);

    outfld_ = new uiSeisSel( this, uiSeisSel::ioContext(Seis::Vol,false),
			     uiSeisSel::Setup(Seis::Vol) );
    outfld_->attach( alignedBelow, smrtscalebox_ );

    possubsel_ =  new uiPosSubSel( this, uiPosSubSel::Setup(false,true) );
    possubsel_->attach( alignedBelow, outfld_ );

    batchfld_ = new uiBatchJobDispatcherSel( this, false,
					    Batch::JobSpec::TwoDto3D );
    IOPar& iop = jobSpec().pars_;
    iop.set( IOPar::compKey(sKey::Output(),sKey::Type()), "Cube" );
    batchfld_->attach( alignedBelow, possubsel_ );
}

Batch::JobSpec& uiSeis2DTo3DInterPol::jobSpec()
{
    return batchfld_->jobSpec();
}

#define mErrRet(s) { uiMSG().error(s); return false; }
bool uiSeis2DTo3DInterPol::prepareProcessing()
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

bool uiSeis2DTo3DInterPol::fillSeisPar()
{
    IOPar& iop = jobSpec().pars_;

    iop.set( Seis2DTo3DInterPol::sKeyInput(), inpfld_->key() );
    iop.set( SeisJobExecProv::sKeySeisOutIDKey(), outfld_->key() );
    iop.set( Seis2DTo3DInterPol::sKeyCreaterType(),
				    Seis2DTo3DInterPol::getCreatorFormat());

    IOPar sampling;

    possubsel_->fillPar( sampling );
    IOPar subsel;
    subsel.mergeComp( sampling, sKey::Subsel() );
    uiSeisIOObjInfo ioobjinfo( *(outfld_->ioobj()), true );
    TrcKeyZSampling cs = possubsel_->envelope();
    cs.fillPar(subsel);
    SeisIOObjInfo::SpaceInfo spi( cs.nrZ(), (int)cs.hsamp_.totalNr() );
    subsel.set( "Estimated MBs", ioobjinfo.expectedMBs(spi) );
    if ( !ioobjinfo.checkSpaceLeft(spi) )
	return false;

    iop.mergeComp( subsel, sKey::Output() );

    return true;
}

bool uiSeis2DTo3DInterPol::fillPar()
{
    if ( !fillSeisPar() )
	return false;

    IOPar par;

    BufferString method = Seis2DTo3DInterPolImpl::sFactoryKeyword();

    par.set(Seis2DTo3DInterPol::sKeyType(), method);
    par.set(Seis2DTo3DInterPol::sKeyPow(), powfld_->getFValue() );
    par.set(Seis2DTo3DInterPol::sKeyTaper(), taperfld_->getFValue() );
    par.setYN(Seis2DTo3DInterPol::sKeySmrtScale(), smrtscalebox_->isChecked() );
    jobSpec().pars_.mergeComp( par, sKey::Pars() );
    return true;
}

bool uiSeis2DTo3DInterPol::acceptOK( CallBacker* )
{
    if ( taperfld_->getFValue() < 0 || taperfld_->getFValue() > 90 )
    {
	uiMSG().error( uiStrings::phrEnter(tr("a taper angle"
			  "value between 0 and 90 degrees")) );
	return false;
    }

    if ( !prepareProcessing() || !fillPar() )
	return false;

    if ( !SI().has3D() )
	uiMSG().warning( tr( "After processing you will need to change\n"
			     "the survey type to 'Both 2D and 3D'\n"
			     "in survey setup to display/use the cube" ) );

    batchfld_->setJobName( outfld_->getInput() );
    return batchfld_->start();
}
