/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2011
________________________________________________________________________

-*/



#include "uihorizonmergedlg.h"

#include "uigeninput.h"
#include "uiioobjsel.h"
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
#include "od_helpids.h"


uiHorizonMergeDlg::uiHorizonMergeDlg( uiParent* p, bool is2d )
    : uiDialog(p,Setup(tr("Merge 3D Horizons"),uiString::empty(),
                       mODHelpKey(mHorizonMergeDlgHelpID) ))
{
    horselfld_ = new uiHorizon3DSel( this );

    duplicatefld_ = new uiGenInput( this, tr("Duplicate positions"),
	    StringListInpSpec(EM::HorizonMerger::ModeDef()) );
    duplicatefld_->attach( alignedBelow, horselfld_ );

    const char* typestr = is2d ? EM::Horizon2D::typeStr()
			       : EM::Horizon3D::typeStr();
    const uiString typelabel = is2d ? EM::Horizon2D::userTypeStr()
				    : EM::Horizon3D::userTypeStr();
    uiSurfaceWrite::Setup ssu( typestr, typelabel );
    ssu.withcolorfld(true).withstratfld(true);
    outfld_ = new uiSurfaceWrite( this, ssu );
    outfld_->attach( alignedBelow, duplicatefld_ );
}


uiHorizonMergeDlg::~uiHorizonMergeDlg()
{}


void uiHorizonMergeDlg::setInputHors( const DBKeySet& mids )
{
     horselfld_->setSelSurfaceIds( mids );
}


DBKey uiHorizonMergeDlg::getNewHorMid() const
{
    return outfld_->getObjSel()->key();
}


bool uiHorizonMergeDlg::acceptOK()
{
    uiTaskRunnerProvider trprov( this );

    DBKeySet objids;
    horselfld_->getSelSurfaceIds( objids );
    if ( objids.size() < 2 )
    {
	uiMSG().error( uiStrings::phrSelect(tr("at least 2 horizons")) );
	return false;
    }

    outfld_->processInput();
    const IOObj* ioobj = outfld_->selIOObj();
    if ( !ioobj )
	return false;

    RefObjectSet<EM::Object> objs = EM::Hor3DMan().loadObjects( objids, trprov);
    if ( objs.size() != objids.size() )
    {
	uiMSG().error( tr("Cannot load selected input horizons") );
	return false;
    }

    EM::Horizon3DMerger merger( objids );
    merger.setMode( (EM::HorizonMerger::Mode)duplicatefld_->getIntValue() );
    if ( !trprov.execute(merger) )
    {
	uiMSG().error( tr("Cannot merge horizons") );
	return false;
    }

    EM::Horizon3D* hor3d = merger.getOutputHorizon();
    if ( !hor3d )
    {
	uiMSG().error( tr("No output horizon generated") );
	return false;
    }

    hor3d->setPreferredColor( outfld_->getColor() );
    hor3d->setStratLevelID( outfld_->getStratLevelID() );
    hor3d->setDBKey( ioobj->key() );
    uiRetVal ret = EM::Hor3DMan().save( ioobj->key(), trprov );
    if ( !ret.isOK() )
    {
	uiMSG().errorWithDetails( ret, tr("Cannot save output horizon") );
	return false;
    }

    return true;
}
