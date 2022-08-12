/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2006
________________________________________________________________________

-*/


#include "uiseiseventsnapper.h"

#include "uigeninput.h"
#include "uihorsavefieldgrp.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uiseparator.h"
#include "uitaskrunner.h"

#include "binidvalset.h"
#include "ctxtioobj.h"
#include "emhorizon3d.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "emposid.h"
#include "emsurfacetr.h"
#include "ioobj.h"
#include "mousecursor.h"
#include "seiseventsnapper.h"
#include "seis2deventsnapper.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "od_helpids.h"


uiSeisEventSnapper::uiSeisEventSnapper( uiParent* p, const IOObj* inp,
					bool is2d )
    : uiDialog(p,Setup(tr("Snap horizon to seismic event"),mNoDlgTitle,
		       mODHelpKey(mSnapToEventHelpID) ).modal(false))
    , horizon_(0)
    , is2d_(is2d)
    , readyForDisplay(this)
{
    setCtrlStyle( RunAndClose );

    horinfld_ = new uiIOObjSel( this, is2d ? mIOObjContext(EMHorizon2D)
					   : mIOObjContext(EMHorizon3D),
			        tr("%1 to snap").arg(uiStrings::sHorizon()) );
    if ( inp ) horinfld_->setInput( *inp );

    const Seis::GeomType gt = is2d_ ? Seis::Line : Seis::Vol;
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

    uiSeparator* sep = new uiSeparator( this, "Hor sep" );
    sep->attach( stretchedBelow, gatefld_ );

    savefldgrp_ = new uiHorSaveFieldGrp( this, horizon_, is2d );
    savefldgrp_->setSaveFieldName( "Save snappeded horizon" );
    savefldgrp_->attach( alignedBelow, gatefld_ );
    savefldgrp_->attach( ensureBelow, sep );
}


uiSeisEventSnapper::~uiSeisEventSnapper()
{
    if ( horizon_ ) horizon_->unRef();
}


#define mErrRet(msg) { uiMSG().error(msg); return false; }


bool uiSeisEventSnapper::readHorizon()
{
    if ( !horinfld_->ioobj() )
	return false;

    const MultiID& mid = horinfld_->key();
    EM::Horizon* hor = savefldgrp_->readHorizon( mid );
    if ( !hor ) mErrRet( tr("Could not load horizon") );

    if ( horizon_ ) horizon_->unRef();
    horizon_ = hor;
    horizon_->ref();
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

    EM::Horizon* usedhor = savefldgrp_->getNewHorizon() ?
	savefldgrp_->getNewHorizon() : horizon_;
    usedhor->setBurstAlert( true );

    Interval<float> rg = gatefld_->getFInterval();
    rg.scale( 1.f / SI().zDomain().userFactor() );

    for ( int idx=0; idx<horizon_->geometry().nrSections(); idx++ )
    {
	if ( !is2d_ )
	{
	    mDynamicCastGet(EM::Horizon3D*,hor3d,horizon_)
	    if ( !hor3d )
		return false;

	    mDynamicCastGet(EM::Horizon3D*,newhor3d,usedhor)
	    if ( !newhor3d )
		return false;

	    BinIDValueSet bivs( 1, false );
	    hor3d->geometry().fillBinIDValueSet( bivs );

	    SeisEventSnapper3D snapper( *seisioobj, bivs, rg );
	    snapper.setEvent( VSEvent::Type(eventfld_->getIntValue()+1) );

	    uiTaskRunner dlg( this );
	    if ( !TaskRunner::execute(&dlg,snapper) )
		return false;

	    hor3d->setBurstAlert( true );
	    MouseCursorManager::setOverride( MouseCursor::Wait );
	    BinIDValueSet::SPos pos;
	    while ( bivs.next(pos) )
	    {
		BinID bid; float z;
		bivs.get( pos, bid, z );
		newhor3d->setPos( bid.toInt64(), Coord3(0,0,z),
				  false );
	    }

	    hor3d->setBurstAlert( false );
	    MouseCursorManager::restoreOverride();
	}
	else
	{
	    mDynamicCastGet(EM::Horizon2D*,hor2d,horizon_)
	    if ( !hor2d )
		return false;

	    mDynamicCastGet(EM::Horizon2D*,newhor2d,usedhor)
	    if ( !newhor2d )
		return false;

	    SeisEventSnapper2D::Setup su(
		    seisioobj, eventfld_->getIntValue()+1, rg );
	    SeisEventSnapper2D snapper( hor2d, newhor2d, su );

	    uiTaskRunner dlg( this );
	    if ( !TaskRunner::execute( &dlg, snapper ) )
		return false;
	}

	usedhor->setBurstAlert( false );
	MouseCursorManager::restoreOverride();
    }

    const bool res = savefldgrp_->saveHorizon();
    if ( res && savefldgrp_->displayNewHorizon() )
	readyForDisplay.trigger();

    return false;
}
