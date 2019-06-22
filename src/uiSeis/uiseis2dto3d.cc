/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2011
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

#include "keystrs.h"
#include "od_helpids.h"
#include "seis2dto3d.h"
#include "seisjobexecprov.h"
#include "survinfo.h"
#include "trckeyzsampling.h"


uiSeis2DTo3D::uiSeis2DTo3D(uiParent* p, uiString& titletext)
	: uiDialog( p, Setup( titletext,
			      mNoDlgTitle,
			      mODHelpKey(mSeis2DTo3DHelpID) ) )
{
	uiSeisSel::Setup sssu( Seis::Line );
    sssu.steerpol( Seis::InclSteer );
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

bool uiSeis2DTo3D::fillSeisPar()
{
    IOPar& iop = jobSpec().pars_;

    iop.set( Seis2DTo3D::sKeyInput(), inpfld_->key() );
    iop.set( SeisJobExecProv::sKeySeisOutIDKey(), outfld_->key() );

    IOPar sampling;
    possubsel_->fillPar( sampling );
    IOPar subsel;
    subsel.mergeComp( sampling, sKey::Subsel() );
    uiSeisIOObjInfo ioobjinfo( this, *outfld_->ioobj() );
    TrcKeyZSampling cs = possubsel_->envelope();
    SeisIOObjInfo::SpaceInfo spi( cs.nrZ(), (int)cs.hsamp_.totalNr() );
    subsel.set( "Estimated MBs", ioobjinfo.expectedMBs(spi) );
    if ( !ioobjinfo.checkSpaceLeft(spi,true) )
	return false;

    iop.mergeComp( subsel, sKey::Output() );
    return true;
}

bool uiSeis2DTo3D::fillPar()
{
    if ( !fillSeisPar() )
	return false;

    IOPar par;
	BufferString method = Seis2DTo3DImpl::sFactoryKeyword();

	par.set(Seis2DTo3D::sKeyType(), method);
    par.set(Seis2DTo3D::sKeyPow(), powfld_->getFValue() );
    par.set(Seis2DTo3D::sKeyTaper(), taperfld_->getFValue() );
    par.setYN(Seis2DTo3D::sKeySmrtScale(), smrtscalebox_->isChecked() );
    jobSpec().pars_.mergeComp( par, sKey::Pars() );
    return true;
}

bool uiSeis2DTo3D::acceptOK()
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
	uiMSG().warning( tr("After processing you will need to change\n"
			     "the survey type to 'Both 2D and 3D'\n"
			     "in survey setup to display/use the cube") );

    batchfld_->setJobName( outfld_->getInput() );
    return batchfld_->start();
}
