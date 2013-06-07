/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2011
________________________________________________________________________

-*/

static const char* rcsID = "$Id$";


#include "uihorizonmergedlg.h"

#include "uigeninput.h"
#include "uiiosurface.h"
#include "uimsg.h"
#include "uisurfacesel.h"
#include "uitaskrunner.h"

#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "executor.h"
#include "horizonmerger.h"
#include "ioobj.h"
#include "ptrman.h"


uiHorizonMergeDlg::uiHorizonMergeDlg( uiParent* p, bool is2d )
    : uiDialog(p,Setup("Merge 3D Horizons","","104.2.10"))
{
    horselfld_ = new uiHorizon3DSel( this );

    duplicatefld_ = new uiGenInput( this, "Duplicate positions",
	    StringListInpSpec(EM::HorizonMerger::ModeNames()) );
    duplicatefld_->attach( alignedBelow, horselfld_ );

    const char* typestr = is2d ? EM::Horizon2D::typeStr()
			       : EM::Horizon3D::typeStr();
    uiSurfaceWrite::Setup ssu( typestr );
    ssu.withcolorfld(true).withstratfld(true);
    outfld_ = new uiSurfaceWrite( this, ssu );
    outfld_->attach( alignedBelow, duplicatefld_ );
}


uiHorizonMergeDlg::~uiHorizonMergeDlg()
{}


bool uiHorizonMergeDlg::acceptOK( CallBacker* )
{
    uiTaskRunner uitr( this );

    TypeSet<MultiID> mids;
    horselfld_->getSelSurfaceIds( mids );
    if ( mids.size() < 2 )
    {
	uiMSG().error( "Please select at least 2 horizons" );
	return false;
    }

    outfld_->processInput();
    IOObj* ioobj = outfld_->selIOObj();
    if ( !ioobj )
	return false;

    PtrMan<Executor> loader = EM::EMM().objectLoader( mids );
    if ( loader && !uitr.execute(*loader) )
    {
	uiMSG().error( "Cannot load selected input horizons" );
	return false;
    }

    TypeSet<EM::ObjectID> objids;
    for ( int idx=0; idx<mids.size(); idx++ )
    {
	EM::ObjectID objid = EM::EMM().getObjectID( mids[idx] );
	if ( EM::EMM().getObject(objid) )
	    objids += objid;
    }

    EM::Horizon3DMerger merger( objids );
    merger.setMode( (EM::HorizonMerger::Mode)duplicatefld_->getIntValue() );
    if ( !uitr.execute(merger) )
    {
	uiMSG().error( "Cannot merge horizons" );
	return false;
    }

    EM::Horizon3D* hor3d = merger.getOutputHorizon();
    if ( !hor3d )
    {
	uiMSG().error( "No output horizon generated" );
	return false;
    }

    hor3d->setPreferredColor( outfld_->getColor() );
    hor3d->setStratLevelID( outfld_->getStratLevelID() );
    hor3d->setMultiID( ioobj->key() );
    PtrMan<Executor> saver = hor3d->saver();
    if ( !saver || !uitr.execute(*saver) )
    {
	uiMSG().error( "Cannot save output horizon" );
	return false;
    }

    return true;
}
