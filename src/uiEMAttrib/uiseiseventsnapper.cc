/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          September 2006
 RCS:           $Id: uiseiseventsnapper.cc,v 1.2 2006-09-19 09:46:30 cvsnanne Exp $
________________________________________________________________________

-*/


#include "uiseiseventsnapper.h"

#include "uiexecutor.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiseissel.h"

#include "ctxtioobj.h"
#include "emhorizon.h"
#include "emmanager.h"
#include "emposid.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "ioobj.h"
#include "ptrman.h"
#include "seiseventsnapper.h"
#include "seistrcsel.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "valseriesevent.h"


uiSeisEventSnapper::uiSeisEventSnapper( uiParent* p )
    : uiDialog(p,Setup("Snap horizon to seismic event","",""))
    , horinctio_(*mMkCtxtIOObj(EMHorizon))
    , horoutctio_(*mMkCtxtIOObj(EMHorizon))
    , seisctio_(*mMkCtxtIOObj(SeisTrc))
    , horizon_(0)
{
    horinfld_ = new uiIOObjSel( this, horinctio_, "Horizon to snap" );

    seisfld_ = new uiSeisSel( this, seisctio_, SeisSelSetup() );
    seisfld_->attach( alignedBelow, horinfld_ );

    eventfld_ = new uiGenInput( this, "Event",
	    			StringListInpSpec(VSEvent::TypeNames) );
    eventfld_->attach( alignedBelow, seisfld_ );

    BufferString gatelbl( "Search gate " ); gatelbl += SI().getZUnit();
    gatefld_ = new uiGenInput( this, gatelbl, FloatInpIntervalSpec() );

    savefld_ = new uiGenInput( this, "Save snapped horizon",
			       BoolInpSpec("As new","Overwrite",false) );
    savefld_->valuechanged.notify( mCB(this,uiSeisEventSnapper,saveSel) );
    savefld_->attach( alignedBelow, eventfld_ );

    horoutfld_ = new uiIOObjSel( this, horoutctio_, "Output horizon" );
    horoutfld_->attach( alignedBelow, savefld_ );
    saveSel(0);
}


uiSeisEventSnapper::~uiSeisEventSnapper()
{
    delete horoutctio_.ioobj; delete &horoutctio_;
    delete horinctio_.ioobj; delete &horinctio_;
    delete seisctio_.ioobj; delete &seisctio_;
}


void uiSeisEventSnapper::saveSel( CallBacker* )
{
    horoutfld_->display( savefld_->getBoolValue() );
}


bool uiSeisEventSnapper::readHorizon()
{
    const MultiID& mid = horinfld_->ctxtIOObj().ioobj->key();
    EM::ObjectID oid = EM::EMM().getObjectID( mid );
    EM::EMObject* emobj = EM::EMM().getObject( oid );

    Executor* reader = 0;
    if ( !emobj || !emobj->isFullyLoaded() )
    {
	reader = EM::EMM().objectLoader( mid );
	if ( !reader ) return false;

	uiExecutor dlg( this, *reader );
	if ( !dlg.go() )
	{
	    delete reader;
	    return false;
	}

	oid = EM::EMM().getObjectID( mid );
	emobj = EM::EMM().getObject( oid );
    }

    mDynamicCastGet(EM::Horizon*,hor,emobj)
    horizon_ = hor;
    horizon_->ref();
    delete reader;
    return true;
}


#define mErrRet(msg) { uiMSG().error(msg); return false; }

bool uiSeisEventSnapper::acceptOK( CallBacker* )
{
    if ( !seisctio_.ioobj )
	mErrRet( "Please select Seismics" )
    if ( !readHorizon() )
	mErrRet( "Cannot read horizon" );

// TODO: loop over all sections
    EM::SectionID sid = horizon_->sectionID( 0 );
    BinIDValueSet bivs( 1, false );
    horizon_->geometry().fillBinIDValueSet( sid, bivs );

    SeisEventSnapper snapper( *seisctio_.ioobj, bivs );
    snapper.setEvent( (VSEvent::Type)eventfld_->getIntValue() );
    snapper.setSearchGate( gatefld_->getFInterval() );
    uiExecutor dlg( this, snapper );
    dlg.go();

    return true;
}
