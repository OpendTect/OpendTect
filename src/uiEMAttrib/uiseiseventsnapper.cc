/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseiseventsnapper.cc,v 1.24 2009-11-05 19:49:48 cvsyuancheng Exp $";


#include "uiseiseventsnapper.h"

#include "uitaskrunner.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiseissel.h"

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
    , horinctio_(*mMkCtxtIOObj(EMHorizon3D))
    , horoutctio_(*mMkCtxtIOObj(EMHorizon3D))
    , seisctio_(*uiSeisSel::mkCtxtIOObj(Seis::Vol,true))
    , horizon_(0)
{
    if ( inp )
	horinctio_.setObj( inp->clone() );
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

    savefld_ = new uiGenInput( this, "Save snapped horizon",
			       BoolInpSpec(false,"As new","Overwrite") );
    savefld_->valuechanged.notify( mCB(this,uiSeisEventSnapper,saveSel) );
    savefld_->attach( alignedBelow, gatefld_ );

    horoutctio_.ctxt.forread = false;
    horoutfld_ = new uiIOObjSel( this, horoutctio_, "Output horizon" );
    horoutfld_->attach( alignedBelow, savefld_ );
    saveSel(0);
}


uiSeisEventSnapper::~uiSeisEventSnapper()
{
    delete horoutctio_.ioobj; delete &horoutctio_;
    delete horinctio_.ioobj; delete &horinctio_;
    delete seisctio_.ioobj; delete &seisctio_;
    if ( horizon_ ) horizon_->unRef();
}


bool uiSeisEventSnapper::overwriteHorizon() const
{ return !savefld_->getBoolValue(); }


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

	uiTaskRunner dlg( this );
	if ( !dlg.execute(*reader) )
	{
	    delete reader;
	    return false;
	}

	oid = EM::EMM().getObjectID( mid );
	emobj = EM::EMM().getObject( oid );
    }

    mDynamicCastGet(EM::Horizon3D*,hor,emobj)
    horizon_ = hor;
    horizon_->ref();
    delete reader;
    return true;
}


#define mErrRet(msg) { uiMSG().error(msg); return false; }

bool uiSeisEventSnapper::saveHorizon()
{
    PtrMan<Executor> exec = 0;
    const bool saveas = savefld_ && savefld_->getBoolValue();
    if ( !saveas )
	exec = horizon_->saver();
    else if ( !horoutfld_->commitInput() )
	mErrRet( "Cannot continue: write permission problem" )
    else
    {
	const MultiID& mid = horoutfld_->ctxtIOObj().ioobj->key();
	horizon_->setMultiID( mid );
	exec = horizon_->geometry().saver( 0, &mid );
    }

    if ( !exec ) return false;

    uiTaskRunner dlg( this );
    return dlg.execute( *exec );
}


bool uiSeisEventSnapper::acceptOK( CallBacker* )
{
    if ( !seisfld_->commitInput() )
	mErrRet( "Please select the seismics" )
    if ( !readHorizon() )
	mErrRet( "Cannot read horizon" );

// TODO: loop over all sections
    EM::SectionID sid = horizon_->sectionID( 0 );
    BinIDValueSet bivs( 1, false );
    horizon_->geometry().fillBinIDValueSet( sid, bivs );

    Interval<float> rg = gatefld_->getFInterval();
    rg.scale( 1 / SI().zFactor() );

    SeisEventSnapper snapper( *seisctio_.ioobj, bivs, rg );
    snapper.setEvent( VSEvent::Type(eventfld_->getIntValue()+1) );
   
    uiTaskRunner dlg( this );
    if ( !dlg.execute(snapper) )
	return false;

    horizon_->setBurstAlert( true );
    MouseCursorManager::setOverride( MouseCursor::Wait );
    BinIDValueSet::Pos pos;
    while ( bivs.next(pos) )
    {
	BinID bid; float z;
	bivs.get( pos, bid, z );
	horizon_->setPos( sid, bid.getSerialized(), Coord3(0,0,z), false );
    }
    horizon_->setBurstAlert( false );
    MouseCursorManager::restoreOverride();

    if ( !saveHorizon() )
	mErrRet( "Cannot save horizon" )

    return true;
}



