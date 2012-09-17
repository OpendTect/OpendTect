/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseiseventsnapper.cc,v 1.30 2011/09/16 10:59:21 cvskris Exp $";


#include "uiseiseventsnapper.h"

#include "uigeninput.h"
#include "uihorsavefieldgrp.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uiseparator.h"
#include "uitaskrunner.h"

#include "arraynd.h"
#include "ctxtioobj.h"
#include "emhorizon3d.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "emposid.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "ioobj.h"
#include "mousecursor.h"
#include "ptrman.h"
#include "seiseventsnapper.h"
#include "seis2deventsnapper.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "binidvalset.h"
#include "valseriesevent.h"


uiSeisEventSnapper::uiSeisEventSnapper( uiParent* p, const IOObj* inp,
					bool is2d )
    : uiDialog(p,Setup("Snap horizon to seismic event",mNoDlgTitle,"104.0.11"))
    , seisctio_(*uiSeisSel::mkCtxtIOObj(is2d ? Seis::Line : Seis::Vol,true))
    , horizon_(0)
    , is2d_(is2d)
{
    horinfld_ = new uiIOObjSel( this, is2d ? mIOObjContext(EMHorizon2D)
	    				   : mIOObjContext(EMHorizon3D),
			        "Horizon to snap" );
    if ( inp ) horinfld_->setInput( *inp );

    seisfld_ = new uiSeisSel( this, seisctio_,
	    		      uiSeisSel::Setup(is2d ? Seis::Line : Seis::Vol ));
    seisfld_->attach( alignedBelow, horinfld_ );

    BufferStringSet eventnms( VSEvent::TypeNames() );
    eventnms.remove(0);
    eventfld_ = new uiGenInput( this, "Event", StringListInpSpec(eventnms) );
    eventfld_->attach( alignedBelow, seisfld_ );

    BufferString gatelbl( "Search gate " ); gatelbl += SI().getZUnitString();
    gatefld_ = new uiGenInput( this, gatelbl, FloatInpIntervalSpec() );
    gatefld_->setValues( -SI().zStep() * SI().zFactor(), 
	    		  SI().zStep() * SI().zFactor() );
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
    delete seisctio_.ioobj; delete &seisctio_;
    if ( horizon_ ) horizon_->unRef();
}


#define mErrRet(msg) { uiMSG().error(msg); return false; }


bool uiSeisEventSnapper::readHorizon()
{
    if ( !horinfld_->ioobj() )
	return false;
    
    const MultiID& mid = horinfld_->key();
    EM::Horizon* hor = savefldgrp_->readHorizon( mid );
    if ( !hor ) mErrRet( "Could not load horizon" );
    
    if ( horizon_ ) horizon_->unRef();
    horizon_ = hor;
    horizon_->ref();
    return true;
}


#define mErrRet(msg) { uiMSG().error(msg); return false; }

bool uiSeisEventSnapper::acceptOK( CallBacker* cb )
{
    if ( !seisfld_->commitInput() )
	mErrRet( "Please select the seismics" )

    if ( !readHorizon() )
	mErrRet( "Cannot read horizon" );

    if ( !savefldgrp_->acceptOK( cb ) )
	return false;

    EM::Horizon* usedhor = savefldgrp_->getNewHorizon() ?
	savefldgrp_->getNewHorizon() : horizon_;
    usedhor->setBurstAlert( true );
    
    Interval<float> rg = gatefld_->getFInterval();
    rg.scale( 1. / SI().zFactor() );

    for ( int idx=0; idx<horizon_->geometry().nrSections(); idx++ )
    {
	const EM::SectionID sid = horizon_->sectionID( idx );
	if ( !is2d_ )
	{
	    mDynamicCastGet(EM::Horizon3D*,hor3d,horizon_)
	    if ( !hor3d )
		return false;
	    
	    mDynamicCastGet(EM::Horizon3D*,newhor3d,usedhor)
	    if ( !newhor3d )
		return false;

	    EM::SectionID sid0 = hor3d->sectionID( 0 );
	    BinIDValueSet bivs( 1, false );
	    hor3d->geometry().fillBinIDValueSet( sid0, bivs );
	    
	    SeisEventSnapper3D snapper( *seisctio_.ioobj, bivs, rg );
	    snapper.setEvent( VSEvent::Type(eventfld_->getIntValue()+1) );
	   
	    uiTaskRunner dlg( this );
	    if ( !dlg.execute(snapper) )
		return false;

	    hor3d->setBurstAlert( true );
	    MouseCursorManager::setOverride( MouseCursor::Wait );
	    BinIDValueSet::Pos pos;
	    while ( bivs.next(pos) )
	    {
		BinID bid; float z;
		bivs.get( pos, bid, z );
		newhor3d->setPos( sid0, bid.toInt64(), Coord3(0,0,z),
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

	    Seis2DLineSetEventSnapper snapper( hor2d, newhor2d,
		    Seis2DLineSetEventSnapper::Setup(seisfld_->attrNm(), 
						     eventfld_->getIntValue()+1,
						     rg) );
	   
	    uiTaskRunner dlg( this );
	    if ( !dlg.execute(snapper) )
		return false;
	}

	usedhor->setBurstAlert( false );
	MouseCursorManager::restoreOverride();
    }

    return savefldgrp_->saveHorizon();
}



