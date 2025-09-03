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
#include "ioman.h"
#include "od_helpids.h"
#include "seiszaxisstretcher.h"
#include "survinfo.h"

#include "uigeninput.h"
#include "uiiosurface.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uitaskrunner.h"


uiFlatUnflatCube::uiFlatUnflatCube( uiParent* p )
    : uiDialog(p,Setup(tr("Flatten / Unflatten Seismic Data"),
		       mODHelpKey(mFlatUnflatCubeHelpID)))
{
    setCtrlStyle( RunAndClose );

    modefld_ = new uiGenInput( this, uiStrings::sMode(),
			      BoolInpSpec(true,tr("Flatten"),tr("Unflatten")));
    mAttachCB( modefld_->valueChanged, uiFlatUnflatCube::modeChgCB );

    const Seis::GeomType geomtype = Seis::Vol;
    IOObjContext seisctxt( uiSeisSel::ioContext(geomtype,true) );
    seisinfld_ = new uiSeisSel( this, seisctxt, geomtype );
    mAttachCB( seisinfld_->selectionDone, uiFlatUnflatCube::inpSelCB );
    seisinfld_->attach( alignedBelow, modefld_ );

    IOObjContext flatctxt( uiSeisSel::ioContext(geomtype,true) );
    flatctxt.requireZDomain( EM::flattenedZDomain(), false );
    uiSeisSel::Setup flatsu( geomtype );
    flatsu.enabotherdomain( true ).seltxt( tr("Input Flattened Cube") );
    flatinfld_ = new uiSeisSel( this, flatctxt, flatsu );
    mAttachCB( flatinfld_->selectionDone, uiFlatUnflatCube::inpSelCB );
    flatinfld_->attach( alignedBelow, modefld_ );

    horfld_ = new uiHorizon3DSel( this, true );
    mAttachCB( horfld_->selectionDone, uiFlatUnflatCube::horSelCB );
    horfld_->attach( alignedBelow, seisinfld_ );

    uiString lbl = tr("Flat value %1").arg( SI().getUiZUnitString() );
    flatvalfld_ = new uiGenInput( this, lbl, FloatInpSpec() );
    flatvalfld_->attach( alignedBelow, horfld_ );

    Seis::SelSetup ss( geomtype );
    ss.onlyrange(true).withoutz(true).withstep(false);
    rgfld_ = uiSeisSubSel::get( this, ss );
    rgfld_->attach( alignedBelow, flatvalfld_ );

    seisctxt.forread_ = false;
    seisoutfld_ = new uiSeisSel( this, seisctxt, geomtype );
    seisoutfld_->attach( alignedBelow, rgfld_ );

    flatctxt.forread_ = false;
    flatsu.seltxt( tr("Output Flattened Cube") );
    flatoutfld_ = new uiSeisSel( this, flatctxt, flatsu );
    flatoutfld_->attach( alignedBelow, rgfld_ );

    mAttachCB( postFinalize(), uiFlatUnflatCube::finalizeCB );
}


uiFlatUnflatCube::~uiFlatUnflatCube()
{
    detachAllNotifiers();
}


uiSeisSel* uiFlatUnflatCube::getInpFld()
{
    return doFlatten() ? seisinfld_ : flatinfld_;
}


uiSeisSel* uiFlatUnflatCube::getOutpFld()
{
    return doFlatten() ? flatoutfld_ : seisoutfld_;
}


void uiFlatUnflatCube::finalizeCB( CallBacker* )
{
    inpSelCB( nullptr );
    horSelCB( nullptr );
    modeChgCB( nullptr );
}


void uiFlatUnflatCube::setHorizon( const MultiID& key )
{
    horfld_->setInput( key );
}


bool uiFlatUnflatCube::doFlatten() const
{
    return modefld_->getBoolValue();
}


void uiFlatUnflatCube::modeChgCB( CallBacker* )
{
    const bool doflat = doFlatten();
    seisinfld_->display( doflat );
    flatoutfld_->display( doflat );

    flatinfld_->display( !doflat );
    seisoutfld_->display( !doflat );
    inpSelCB( nullptr );
}


void uiFlatUnflatCube::inpSelCB( CallBacker* )
{
    const bool doflat = doFlatten();
    if ( doflat )
	return;

    const IOObj* seisioobjin = getInpFld()->ioobj( true );
    if ( !seisioobjin )
	return;

    rgfld_->setInput( *seisioobjin );
    PtrMan<IOPar> iop = seisioobjin->pars().subselect( ZDomain::sKey() );
    if ( !iop )
	return;

    MultiID horid;
    if ( iop->get(sKey::ID(),horid) && !horid.isUdf() )
    {
	horfld_->setInput( horid );
	horSelCB( nullptr );
    }

    float horval = mUdf(float);
    if ( iop->get(EM::HorizonZTransform::sKeyReferenceZ(),horval) &&
	 !mIsUdf(horval) )
	flatvalfld_->setValue( horval );
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
    const IOObj* seisioobjin = getInpFld()->ioobj();
    getOutpFld()->reset();
    const IOObj* seisioobjout = getOutpFld()->ioobj();
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

    const bool doflat = doFlatten();
    if ( doflat )
    {
	ZDomain::Info zdinf( EM::flattenedZDomain() );
	zdinf.setID( horioobjin->key() );
	zdinf.pars_.set( EM::HorizonZTransform::sKeyReferenceZ(), flatzval );
	seisioobjout->pars().mergeComp( zdinf.pars_, ZDomain::sKey() );
	IOM().commitChanges( *seisioobjout );
    }

    TrcKeyZSampling tkzs;
    rgfld_->getSampling( tkzs );
    tkzs.zsamp_ = tf->getZInterval( !doflat );
    SeisZAxisStretcher stretcher( *seisioobjin, *seisioobjout, tkzs,
				  *tf, doflat );
    stretcher.setUdfVal( 0.f );
    if ( !uitr.execute(stretcher) )
    {
// stretcher does not give a good error message at the moment.
//	uiMSG().errorWithDetails( stretcher.errorWithDetails() );
	return false;
    }

    uiString msg = tr("Cube successfully %1. Do you want to %2 more cubes?")
			.arg(doflat ? "flattened" : "unflattened" )
			.arg(doflat ? "flattend" : "unflatten");
    const bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				      tr("No, close window") );
    return !ret;
}
