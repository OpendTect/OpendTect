/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseiseventsnapper.cc,v 1.25 2009-11-12 21:34:40 cvsyuancheng Exp $";


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
#include "emmanager.h"
#include "emposid.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "ioobj.h"
#include "mousecursor.h"
#include "ptrman.h"
#include "seiseventsnapper.h"
#include "seisselection.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "binidvalset.h"
#include "valseriesevent.h"


uiSeisEventSnapper::uiSeisEventSnapper( uiParent* p, const IOObj* inp )
    : uiDialog(p,Setup("Snap horizon to seismic event",mNoDlgTitle,"104.0.11"))
    , seisctio_( *uiSeisSel::mkCtxtIOObj(Seis::Vol,true) )
    , horizon_( 0 )
    , horinctio_( *mMkCtxtIOObj(EMHorizon3D) )
{
    if ( inp ) horinctio_.setObj( inp->clone() );
    horinfld_ = new uiIOObjSel( this, horinctio_, "Horizon to snap" );

    seisfld_ = new uiSeisSel( this, seisctio_, uiSeisSel::Setup(Seis::Vol) );
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
    
    savefldgrp_ = new uiHorSaveFieldGrp( this, horizon_ );
    savefldgrp_->setSaveFieldName( "Save snappeded horizon" );
    savefldgrp_->attach( alignedBelow, gatefld_ );
    savefldgrp_->attach( ensureBelow, sep );
}


uiSeisEventSnapper::~uiSeisEventSnapper()
{
    delete seisctio_.ioobj; delete &seisctio_;
    delete horinctio_.ioobj; delete &horinctio_;
    if ( horizon_ ) horizon_->unRef();
}


#define mErrRet(msg) { uiMSG().error(msg); return false; }


bool uiSeisEventSnapper::readHorizon()
{
    if ( !horinfld_->ctxtIOObj(false).ioobj )
	return false;
    
    const MultiID& mid = horinfld_->ctxtIOObj(false).ioobj->key();
    EM::Horizon3D* hor = savefldgrp_->readHorizon( mid );
    if ( !hor ) mErrRet( "Could not load horizon" );
    
    if ( horizon_ ) horizon_->unRef();
    horizon_ = hor;
    horizon_->ref();
    
    return true;
}


bool uiSeisEventSnapper::acceptOK( CallBacker* cb )
{
    if ( !seisfld_->commitInput() )
	mErrRet( "Please select the seismics" )

    if ( !readHorizon() )
	mErrRet( "Cannot read horizon" );

    if ( !savefldgrp_->acceptOK( cb ) )
	return false;

    EM::Horizon3D* usedhor = savefldgrp_->getNewHorizon() ?
	savefldgrp_->getNewHorizon() : horizon_;
    usedhor->setBurstAlert( true );
    
    Interval<float> rg = gatefld_->getFInterval();
    rg.scale( 1 / SI().zFactor() );

    for ( int idx=0; idx<horizon_->geometry().nrSections(); idx++ )
    {
	const EM::SectionID sid = horizon_->sectionID( idx );
	BinIDValueSet bivs( 1, false );
	horizon_->geometry().fillBinIDValueSet( sid, bivs );

	SeisEventSnapper snapper( *seisctio_.ioobj, bivs, rg );
	snapper.setEvent( VSEvent::Type(eventfld_->getIntValue()+1) );
       
	uiTaskRunner dlg( this );
	if ( !dlg.execute(snapper) )
	    return false;

	const EM::SectionID usedsid = usedhor->sectionID( idx );
	MouseCursorManager::setOverride( MouseCursor::Wait );
	BinIDValueSet::Pos pos;
	while ( bivs.next(pos) )
	{
	    BinID bid; float z;
	    bivs.get( pos, bid, z );
	    usedhor->setPos(usedsid,bid.getSerialized(),Coord3(0,0,z),false);
	}
    }

    usedhor->setBurstAlert( false );
    MouseCursorManager::restoreOverride();

    return savefldgrp_->saveHorizon();
}



