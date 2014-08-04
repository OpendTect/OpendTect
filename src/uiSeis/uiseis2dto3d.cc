/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2011
________________________________________________________________________

-*/



static const char* rcsID mUsedVar = "$Id$";

#include "ctxtioobj.h"
#include "cubesampling.h"
#include "seis2dto3d.h"
#include "seisjobexecprov.h"
#include "seisselection.h"
#include "seistrctr.h"
#include "survinfo.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseis2dto3d.h"
#include "uiseisioobjinfo.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uitaskrunner.h"

uiSeis2DTo3D::uiSeis2DTo3D( uiParent* p )
	: uiDialog( p, Setup( "create 3D cube from to 2D LineSet",
			      "Specify process parameters",
			      "103.2.24") )
	, inctio_(*mMkCtxtIOObj(SeisTrc))
	, outctio_((*uiSeisSel::mkCtxtIOObj(Seis::Vol,false)))
	, seis2dto3d_(*new Seis2DTo3D)
{
    inpfld_ = new uiSeisSel( this, inctio_, uiSeisSel::Setup( Seis::Line ) );
    interpoltypefld_ = new uiGenInput( this, "Type of interpolation",
			     BoolInpSpec(true,"Nearest trace","FFT based") );
    interpoltypefld_->attach( alignedBelow, inpfld_ );
    interpoltypefld_->valuechanged.notify(mCB(this,uiSeis2DTo3D,typeChg));

    winfld_ = new uiGenInput( this,"Interpolation window (Inl/Crl)",
							IntInpIntervalSpec() );
    winfld_->attach( alignedBelow, interpoltypefld_ );
    winfld_->setValue( Interval<float>(150,150) );

    reusetrcsbox_ = new uiCheckBox( this, "Re-use interpolated traces" );
    reusetrcsbox_->attach( alignedBelow, winfld_ );

    velfiltfld_ = new uiGenInput( this, "Maximum velocity to pass (m/s)" );
    velfiltfld_->setValue( 2000 );
    velfiltfld_->attach( alignedBelow, reusetrcsbox_ );

    outctio_.ctxt.forread = false;
    outfld_ = new uiSeisSel( this, outctio_, uiSeisSel::Setup(Seis::Vol) );
    outfld_->attach( alignedBelow, velfiltfld_ );

    outsubselfld_ = uiSeisSubSel::get( this, Seis::SelSetup(Seis::Vol) );
    outsubselfld_->attachObj()->attach( alignedBelow, outfld_ );

    typeChg( 0 );
}


uiSeis2DTo3D::~uiSeis2DTo3D()
{
    delete inctio_.ioobj; delete &inctio_;
    delete outctio_.ioobj; delete &outctio_;
    delete &seis2dto3d_;
}


#define mErrRet(s) { uiMSG().error(s); return false; }
bool uiSeis2DTo3D::acceptOK( CallBacker* )
{
    if ( !inpfld_->commitInput() )
	mErrRet("Missing Input\nPlease select the input seismics")
    if ( !outfld_->commitInput() )
	mErrRet("Missing Output\nPlease enter a name for the output seismics")
    else if ( outctio_.ioobj->implExists(false)
	   && !uiMSG().askGoOn("Output cube exists. Overwrite?") )
	return false;

    seis2dto3d_.setInput( *inctio_.ioobj, inpfld_->attrNm() );

    CubeSampling cs(false);
    outsubselfld_->getSampling( cs.hrg );
    outsubselfld_->getZRange( cs.zrg );

    const int wininlstep = winfld_->getIInterval().start;
    const int wincrlstep = winfld_->getIInterval().stop;
    const float maxvel = velfiltfld_->getfValue();
    const bool reusetrcs = reusetrcsbox_->isChecked();

    seis2dto3d_.setParams( wininlstep, wincrlstep, maxvel, reusetrcs );
    seis2dto3d_.setOutput( *outctio_.ioobj, cs );
    seis2dto3d_.setIsNearestTrace( interpoltypefld_->getBoolValue() );

    if ( seis2dto3d_.errMsg() )
	uiMSG().error( seis2dto3d_.errMsg() );

    uiTaskRunner taskrunner( this );
    if ( !TaskRunner::execute( &taskrunner, seis2dto3d_ ) )
	return seis2dto3d_.errMsg();

    if ( !SI().has3D() )
	uiMSG().warning( "3D cube created successfully. "
			 "You need to change survey type to 'Both 2D and 3D' "
			 "in survey setup to display/use the cube" );
    return true;
}



void uiSeis2DTo3D::typeChg( CallBacker* )
{
    bool isfft = !interpoltypefld_->getBoolValue();
    winfld_->display( isfft );
    reusetrcsbox_->display( isfft );
    velfiltfld_->display( isfft );
}



uiSeis2DTo3DFullBatch::uiSeis2DTo3DFullBatch( uiParent* p )
	: uiFullBatchDialog( p,
		     uiFullBatchDialog::Setup("Create 3D cube from a 2D DataSet")
		     .procprognm("od_process_2dto3d"))
	, inctio_(*mMkCtxtIOObj(SeisTrc))
	, outctio_((*uiSeisSel::mkCtxtIOObj(Seis::Vol,false)))
{
    setHelpID( "103.2.24" );
    inpfld_ = new uiSeisSel( uppgrp_, inctio_, uiSeisSel::Setup( Seis::Line ) );
    inpfld_->selectionDone.notify(
			mCB(this,uiSeis2DTo3DFullBatch,setParFileNameCB) );

    interpoltypefld_ = new uiGenInput( uppgrp_, "Type of interpolation",
			     BoolInpSpec(true,"Nearest trace","FFT based") );
    interpoltypefld_->attach( alignedBelow, inpfld_ );
    interpoltypefld_->valuechanged.notify(
				mCB(this,uiSeis2DTo3DFullBatch,typeChg) );

    winfld_ = new uiGenInput( uppgrp_,"Interpolation stepout (Inl/Crl)",
							IntInpIntervalSpec() );
    winfld_->attach( alignedBelow, interpoltypefld_ );
    winfld_->setValue( Interval<float>(100,100) );

    reusetrcsbox_ = new uiCheckBox( uppgrp_, "Re-use interpolated traces" );
    reusetrcsbox_->attach( alignedBelow, winfld_ );

    velfiltfld_ = new uiGenInput( uppgrp_, "Maximum velocity to pass (m/s)" );
    velfiltfld_->setValue( 2000 );
    velfiltfld_->attach( alignedBelow, reusetrcsbox_ );

    outctio_.ctxt.forread = false;
    outfld_ = new uiSeisSel( uppgrp_, outctio_, uiSeisSel::Setup(Seis::Vol) );
    outfld_->selectionDone.notify(
			mCB(this,uiSeis2DTo3DFullBatch,setParFileNameCB) );
    outfld_->attach( alignedBelow, velfiltfld_ );

    outsubselfld_ = uiSeisSubSel::get( uppgrp_, Seis::SelSetup(Seis::Vol) );
    outsubselfld_->attachObj()->attach( alignedBelow, outfld_ );

    typeChg( 0 );
    setParFileName();

    uppgrp_->setHAlignObj( inpfld_ );
    addStdFields( false, false, true );
    setMode( Multi );
}


uiSeis2DTo3DFullBatch::~uiSeis2DTo3DFullBatch()
{
    delete inctio_.ioobj; delete &inctio_;
    delete outctio_.ioobj; delete &outctio_;
}


void uiSeis2DTo3DFullBatch::typeChg( CallBacker* )
{
    bool isfft = !interpoltypefld_->getBoolValue();
    winfld_->display( isfft );
    reusetrcsbox_->display( isfft );
    velfiltfld_->display( isfft );
}


void uiSeis2DTo3DFullBatch::setParFileName()
{
    BufferString parfnm( "2d_to_3d_" );
    parfnm.add( inpfld_->attrNm() );
    setParFileNmDef( parfnm );
}


void uiSeis2DTo3DFullBatch::setParFileNameCB( CallBacker* )
{
    setParFileName();
}


#define mErrRet(s) { uiMSG().error(s); return false; }
bool uiSeis2DTo3DFullBatch::checkInpFlds() const
{
    if ( !inpfld_->commitInput() )
	mErrRet( "Missing Input\nPlease select the input seismics" )

    if ( !outfld_->commitInput() )
	mErrRet( "Missing Output\nPlease enter a name for the output seismics" )

    return true;
}


bool uiSeis2DTo3DFullBatch::prepareProcessing()
{
    if ( !checkInpFlds() ) return false;

    return true;
}


bool uiSeis2DTo3DFullBatch::fillSeisPar( IOPar& par )
{
    par.set( Seis2DTo3D::sKeyInput(), inpfld_->key() );
    par.set( sKey::Attribute(), inpfld_->attrNm() );

    par.set( SeisJobExecProv::sKeySeisOutIDKey(), outfld_->key() );

    IOPar sampling;
    outsubselfld_->fillPar( sampling );

    IOPar subsel;
    subsel.mergeComp( sampling, sKey::Subsel() );
    uiSeisIOObjInfo ioobjinfo( *outctio_.ioobj, true );
    SeisIOObjInfo::SpaceInfo spi( outsubselfld_->expectedNrSamples(),
				  outsubselfld_->expectedNrTraces() );
    subsel.set( "Estimated MBs", ioobjinfo.expectedMBs(spi) );
    if ( !ioobjinfo.checkSpaceLeft(spi) )
	return false;

    par.mergeComp( subsel, sKey::Output() );
    return true;
}


void uiSeis2DTo3DFullBatch::fillParamsPar( IOPar& par )
{
    const bool isnearest = interpoltypefld_->getBoolValue();
    par.setYN( Seis2DTo3D::sKeyIsNearest(), isnearest );
    if ( !isnearest )
    {
	par.set( Seis2DTo3D::sKeyStepout(), winfld_->getIInterval() );
	par.setYN( Seis2DTo3D::sKeyReUse(), reusetrcsbox_->isChecked() );
	par.set( Seis2DTo3D::sKeyMaxVel(), velfiltfld_->getfValue() );
    }
}


bool uiSeis2DTo3DFullBatch::fillPar( IOPar& batchpar )
{
    if ( !fillSeisPar(batchpar) )
	return false;

    IOPar par;
    par.setEmpty();
    fillParamsPar( par );
    batchpar.mergeComp( par, sKey::Pars() );

    if ( !SI().has3D() )
	uiMSG().warning( "After processing you will need to change\n"
			 "the survey type to 'Both 2D and 3D'\n"
			 "in survey setup to display/use the cube" );

    return true;
}

