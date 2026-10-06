/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseiseventsnapper.h"

#include "uigeninput.h"
#include "uihorsavefieldgrp.h"
#include "uiioobjsel.h"
#include "uiiosurface.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uiseparator.h"
#include "uitaskrunner.h"

#include "ctxtioobj.h"
#include "emhorizon3d.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "ioobj.h"
#include "seiseventsnapper.h"
#include "seis2deventsnapper.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "od_helpids.h"


uiSeisEventSnapper::uiSeisEventSnapper( uiParent* p, const IOObj* inp,
					bool is2d )
    : uiDialog(p,Setup(tr("Snap horizon to seismic event"),
		       mODHelpKey(mSnapToEventHelpID)).modal(false))
    , readyForDisplay(this)
{
    setCtrlStyle( RunAndClose );

    const uiIOObjSel::Setup su(
			tr("%1 to snap").arg(uiStrings::sHorizon()) );
    const IOObjContext ctxt = EM::Horizon::ioContext( is2d, true );
    horinfld_ = uiHorizonSel::create( this, ctxt, su );

    if ( inp )
	horinfld_->setInput( *inp );

    const Seis::GeomType gt = is2d ? Seis::Line : Seis::Vol;
    seisfld_ = new uiSeisSel( this, uiSeisSel::ioContext(gt,true),
			      uiSeisSel::Setup(gt));
    seisfld_->attach( alignedBelow, horinfld_ );

    BufferStringSet eventnms( VSEvent::TypeNames() );
    eventnms.removeSingle(0);
    eventfld_ = new uiGenInput( this, tr("Event"),
                                StringListInpSpec(eventnms));
    eventfld_->attach( alignedBelow, seisfld_ );

    uiString gatelbl = tr("Search gate %1").arg(SI().getUiZUnitString());
    gatefld_ = new uiGenInput( this, gatelbl, FloatInpIntervalSpec() );
    gatefld_->setValues( -SI().zStep() * SI().zDomain().userFactor(),
			  SI().zStep() * SI().zDomain().userFactor() );
    gatefld_->attach( alignedBelow, eventfld_ );

    undefpolicyfld_ = new uiGenInput( this, tr("If event not found"),
				      BoolInpSpec(true,tr("Erase position"),
						  tr("Keep original Horizon"),
						  true) );
    undefpolicyfld_->attach( alignedBelow, gatefld_ );

    auto* sep = new uiSeparator( this, "Hor sep" );
    sep->attach( stretchedBelow, undefpolicyfld_ );

    savefldgrp_ = new uiHorSaveFieldGrp( this, horizon_.ptr(), is2d);
    savefldgrp_->setSaveFieldName( "Save snappeded horizon" );
    savefldgrp_->attach( alignedBelow, gatefld_ );
    savefldgrp_->attach( ensureBelow, sep );
}


uiSeisEventSnapper::~uiSeisEventSnapper()
{}


#define mErrRet(msg) { uiMSG().error(msg); return false; }


bool uiSeisEventSnapper::readHorizon()
{
    if ( !horinfld_->ioobj() )
	return false;

    const MultiID& mid = horinfld_->key();
    horizon_ = savefldgrp_->readHorizon( mid ).getNonConstPtr();
    if ( !horizon_ )
	mErrRet( tr("Could not load horizon") );

    return true;
}


#define mErrRet(msg) { uiMSG().error(msg); return false; }

bool uiSeisEventSnapper::acceptOK( CallBacker* cb )
{
    const IOObj* seisioobj = seisfld_->ioobj();
    if ( !seisioobj )
	return false;

    if ( !readHorizon() )
	mErrRet( uiStrings::phrCannotRead( uiStrings::sHorizon() ) );

    if ( !savefldgrp_->acceptOK( cb ) )
	return false;

    RefMan<EM::Horizon> outputhor =
		savefldgrp_->getNewHorizon().getNonConstPtr();
    if ( !outputhor )
	outputhor = horizon_;

    Interval<float> rg = gatefld_->getFInterval();
    rg.scale( 1.f / SI().zDomain().userFactor() );
    const bool eraseundef = undefpolicyfld_->getBoolValue();
    uiTaskRunner runner( this );
    if ( horinfld_->is2D() )
    {
	ExecutorGroup execgrp( "Event Snapper" );
	mDynamicCastGet(const EM::Horizon2D*,inhor2d,horizon_.ptr())
	if ( !inhor2d )
	    return false;

	mDynamicCastGet(EM::Horizon2D*,outhor2d,outputhor.ptr())
	if ( !outhor2d )
	    return false;

	TypeSet<Pos::GeomID> geomids;
	inhor2d->geometry().getGeomIDs( geomids );
	for ( const auto& geomid : geomids )
	{
	    auto* snapper = new SeisEventSnapper2D( *seisioobj, geomid,
		    				    *inhor2d, *outhor2d,
						    rg, eraseundef );
	    execgrp.add( snapper );
	}

	if ( !runner.execute(execgrp) )
	    return false;
    }
    else
    {
	mDynamicCastGet(const EM::Horizon3D*,inhor3d,horizon_.ptr())
	if ( !inhor3d )
	    return false;

	mDynamicCastGet(EM::Horizon3D*,outhor3d,outputhor.ptr())
	if ( !outhor3d )
	    return false;

	SeisEventSnapper3D snapper( *seisioobj, *inhor3d, *outhor3d,
				    rg, eraseundef );
	snapper.setEvent( VSEvent::Type(eventfld_->getIntValue()+1) );

	if ( !runner.execute(snapper) )
	    return false;
    }

    const bool res = savefldgrp_->saveHorizon();
    if ( res && savefldgrp_->displayNewHorizon() )
	readyForDisplay.trigger();

    return false;
}
