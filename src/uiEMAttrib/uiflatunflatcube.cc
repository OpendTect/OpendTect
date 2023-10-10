/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiflatunflatcube.h"

#include "emhorizon3d.h"
#include "emhorizonztransform.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "od_helpids.h"
#include "seiszaxisstretcher.h"
#include "survinfo.h"

#include "uigeninput.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uitaskrunner.h"


uiFlatUnflatCube::uiFlatUnflatCube( uiParent* p )
    : uiDialog(p,Setup(tr("Flatten / Unflatten Seismic Data"),
		       mNoDlgTitle,mODHelpKey(mFlatUnflatCubeHelpID)))
{
    setCtrlStyle( RunAndClose );

    modefld_ = new uiGenInput( this, uiStrings::sMode(),
			      BoolInpSpec(true,tr("Flatten"),tr("Unflatten")));

    const  Seis::GeomType geomtype = Seis::Vol;
    const IOObjContext ctxtin = uiSeisSel::ioContext( geomtype, true );
    seisinfld_ = new uiSeisSel( this, ctxtin, geomtype );
    mAttachCB( seisinfld_->selectionDone, uiFlatUnflatCube::inpSelCB );
    seisinfld_->attach( alignedBelow, modefld_ );

    const IOObjContext horctxt = EMHorizon3DTranslatorGroup::ioContext();
    horfld_ = new uiIOObjSel( this, horctxt );
    mAttachCB( horfld_->selectionDone, uiFlatUnflatCube::horSelCB );
    horfld_->attach( alignedBelow, seisinfld_ );

    uiString lbl = tr("Flat value %1").arg( SI().getUiZUnitString() );
    flatvalfld_ = new uiGenInput( this, lbl, FloatInpSpec() );
    flatvalfld_->attach( alignedBelow, horfld_ );

    Seis::SelSetup ss( geomtype );
    ss.onlyrange(true).withoutz(true).withstep(false);
    rgfld_ = uiSeisSubSel::get( this, ss );
    rgfld_->attach( alignedBelow, flatvalfld_ );

    const IOObjContext ctxtout = uiSeisSel::ioContext( geomtype, false );
    seisoutfld_ = new uiSeisSel( this, ctxtout, geomtype );
    seisoutfld_->attach( alignedBelow, rgfld_ );

    mAttachCB( postFinalize(), uiFlatUnflatCube::finalizeCB );
}


uiFlatUnflatCube::~uiFlatUnflatCube()
{
    detachAllNotifiers();
}


void uiFlatUnflatCube::finalizeCB( CallBacker* )
{
    inpSelCB( nullptr );
    horSelCB( nullptr );
}


void uiFlatUnflatCube::setHorizon( const MultiID& key )
{
    horfld_->setInput( key );
}


void uiFlatUnflatCube::inpSelCB( CallBacker* )
{
    const bool doflat = modefld_->getBoolValue();
    if ( doflat )
	return;

    const IOObj* seisioobjin = seisinfld_->ioobj( true );
    if ( !seisioobjin )
	return;

    rgfld_->setInput( *seisioobjin );
// Read from IOObj's IOPar;
// Set horizon and flat value
}


void uiFlatUnflatCube::horSelCB( CallBacker* )
{
    const IOObj* horioobjin = horfld_->ioobj( true );
    if ( !horioobjin )
	return;

    const EM::IOObjInfo eminfo( horioobjin );
    const Interval<float> zrg = eminfo.getZRange();
    flatvalfld_->setValue(
		zrg.isUdf() ? 0.f : zrg.center()*SI().zDomain().userFactor() );
    flatvalfld_->setNrDecimals( SI().nrZDecimals() );
}


bool uiFlatUnflatCube::acceptOK( CallBacker* )
{
    const IOObj* seisioobjin = seisinfld_->ioobj();
    const IOObj* seisioobjout = seisoutfld_->ioobj();
    const IOObj* horioobjin = horfld_->ioobj();
    if ( !seisioobjin || !seisioobjout || !horioobjin )
	return false;

    uiTaskRunner uitr( this );
    const EM::EMObject* emobj =
		EM::EMM().loadIfNotFullyLoaded( horioobjin->key(), &uitr );
    ConstRefMan<EM::Horizon> horizon = dCast(const EM::Horizon*,emobj);
    if ( !horizon )
    {
	uiMSG().error( tr("Cannot read input horizon") );
	return false;
    }

    RefMan<EM::HorizonZTransform> tf = new EM::HorizonZTransform;
    tf->setHorizon( *horizon );
    float flatzval = flatvalfld_->getFValue();
    if ( mIsUdf(flatzval) )
	flatzval = 0.f;

    tf->setFlatZValue( flatzval / SI().zDomain().userFactor() );

    const bool fwd = modefld_->getBoolValue();

    TrcKeyZSampling tkzs;
    rgfld_->getSampling( tkzs );
    tkzs.zsamp_ = tf->getZInterval( !fwd );
    SeisZAxisStretcher stretcher( *seisioobjin, *seisioobjout, tkzs,
				  *tf, fwd );
    stretcher.setUdfVal( 0.f );
    if ( !uitr.execute(stretcher) )
    {
// stretcher does not give a good error message at the moment.
//	uiMSG().errorWithDetails( stretcher.errorWithDetails() );
	return false;
    }

    uiString msg = tr("Cube successfully %1. Do you want to %2 more cubes?")
			.arg(fwd ? "flattened" : "unflattened" )
			.arg(fwd ? "flattend" : "unflatten");
    const bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				      tr("No, close window") );
    return !ret;
}
