/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2011
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uihorizonmergedlg.cc,v 1.2 2011-05-17 11:58:43 cvsnanne Exp $";


#include "uihorizonmergedlg.h"

#include "uigeninput.h"
#include "uiiosurface.h"
#include "uimsg.h"
#include "uisurfacesel.h"
#include "uitaskrunner.h"

#include "bufstringset.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "executor.h"
#include "horizonmerger.h"
#include "ioobj.h"
#include "ptrman.h"


uiHorizonMergeDlg::uiHorizonMergeDlg( uiParent* p, bool is2d )
    : uiDialog(p,Setup("Merge 3D Horizons","",mTODOHelpID))
{
    setCtrlStyle( DoAndStay );

    horselfld_ = new uiHorizon3DSel( this );

    BufferStringSet options;
    options.add( "Take average" ).add( "Use top" ).add( "Use base" );
    duplicatefld_ = new uiGenInput( this, "Duplicate positions",
	    StringListInpSpec(options) );
    duplicatefld_->attach( alignedBelow, horselfld_ );

    const char* typestr = is2d ? EM::Horizon2D::typeStr()
			       : EM::Horizon3D::typeStr();
    uiSurfaceWrite::Setup ssu( typestr );
    ssu.withcolorfld(true).withstratfld(true).withdisplayfld(true);
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
    PtrMan<IOObj> ioobj = outfld_->selIOObj();
    if ( !ioobj )
	return false;

    PtrMan<Executor> loader = EM::EMM().objectLoader( mids );
    if ( !loader || !uitr.execute(*loader) )
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
    if ( !uitr.execute(merger) )
    {
	uiMSG().error( "Cannot merge horizons" );
	return false;
    }

    uiMSG().error( "Save not implemented yet" );
    return false;
}
