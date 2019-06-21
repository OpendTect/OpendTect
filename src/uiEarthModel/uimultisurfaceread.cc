/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2003
________________________________________________________________________

-*/

#include "uimultisurfaceread.h"

#include "uiioobjselgrp.h"
#include "uilistbox.h"
#include "uipossubsel.h"
#include "uimsg.h"
#include "uitaskrunnerprovider.h"

#include "ioobjctxt.h"
#include "emioobjinfo.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "emmanager.h"
#include "ioobj.h"
#include "od_helpids.h"


uiMultiSurfaceReadDlg::uiMultiSurfaceReadDlg(uiParent* p, const char* type)
   : uiDialog(p,uiDialog::Setup( uiStrings::phrSelect(tr("Input %1").arg(
				 toUiString(type))),mNoDlgTitle,
                                 mODHelpKey(mMultiSurfaceReadDlgHelpID) )
                                 .nrstatusflds(1) )
{
    surfacefld_ = new uiMultiSurfaceRead( this, type );
    surfacefld_->objselGrp()->newStatusMsg.notify(
				mCB(this,uiMultiSurfaceReadDlg,statusMsg) );
    surfacefld_->singleSurfaceSelected.notify( mCB(this,uiDialog,accept) );
}


void uiMultiSurfaceReadDlg::statusMsg( CallBacker* cb )
{
    mCBCapsuleUnpack(const char*,msg,cb);
    toStatusBar( toUiString(msg) );
}


bool uiMultiSurfaceReadDlg::acceptOK()
{
    const int nrchosen = surfacefld_->objselGrp()->nrChosen();
    if ( nrchosen==1 )
    {
	EM::SurfaceIOData sd;
	const EM::IOObjInfo info( surfacefld_->objselGrp()->chosenID() );
	uiString errmsg;
	if ( !info.getSurfaceData(sd,errmsg) )
	{
	    uiMSG().error( errmsg );
	    return false;
	}
    }

    return nrchosen > 0;
}


void uiMultiSurfaceReadDlg::selectHorizons( uiParent* prnt,
			ObjectSet<EM::Object>& objs, bool is2d )
{
    selectSurfaces( prnt, objs, is2d ? EMHorizon2DTranslatorGroup::sGroupName()
				  : EMHorizon3DTranslatorGroup::sGroupName() );
}


void uiMultiSurfaceReadDlg::selectFaults( uiParent* prnt,
					ObjectSet<EM::Object>& objs )
{
    selectSurfaces( prnt, objs, EMFault3DTranslatorGroup::sGroupName() );
}


void uiMultiSurfaceReadDlg::selectFaultStickSets( uiParent* prnt,
					ObjectSet<EM::Object>& objs )
{
    selectSurfaces( prnt, objs, EMFaultStickSetTranslatorGroup::sGroupName() );
}


void uiMultiSurfaceReadDlg::selectSurfaces( uiParent* prnt,
			    ObjectSet<EM::Object>& objs, const char* typ )
{
    uiMultiSurfaceReadDlg dlg( prnt, typ );
    DBKeySet surfaceids;
    if ( !objs.isEmpty() )
    {
	for ( int idx=0; idx<objs.size(); idx++ )
	{
	    EM::Object* emobj = objs[idx];
	    if ( emobj && emobj->dbKey().isValid() )
		surfaceids.add( emobj->dbKey() );
	}
	dlg.iogrp()->setSurfaceIds( surfaceids );
    }
    if ( !dlg.go() )
	return;

    surfaceids.setEmpty();
    dlg.iogrp()->getSurfaceIds( surfaceids );

    EM::SurfaceIOData sd;
    EM::SurfaceIODataSelection sel( sd );
    dlg.iogrp()->getSurfaceSelection( sel );

    uiTaskRunnerProvider trprov( prnt );
    const RefObjectSet<EM::Object> emobjs =
		    EM::MGR().loadObjects( surfaceids, trprov, &sel );
    if ( !emobjs.isEmpty() )
	objs.append( emobjs );
}


// ***** uiMultiSurfaceRead *****

uiMultiSurfaceRead::uiMultiSurfaceRead( uiParent* p, const char* typ )
    : uiIOSurface(p,true,typ)
    , singleSurfaceSelected(this)
{
    ioobjselgrp_ = new uiIOObjSelGrp( this, *ctio_,
			uiIOObjSelGrp::Setup(OD::ChooseAtLeastOne) );
    ioobjselgrp_->selectionChanged.notify( mCB(this,uiMultiSurfaceRead,selCB) );
    ioobjselgrp_->getListField()->doubleClicked.notify(
					mCB(this,uiMultiSurfaceRead,dClck) );

    mkRangeFld();
    rgfld_->attach( leftAlignedBelow, ioobjselgrp_ );

    const FixedString type( typ );
    if ( type == EMHorizon2DTranslatorGroup::sGroupName() ||
	 type == EMFaultStickSetTranslatorGroup::sGroupName() ||
	 type == EMFault3DTranslatorGroup::sGroupName() )
    {
	rgfld_->display( false, true );
    }

    selCB(0);
}


uiMultiSurfaceRead::~uiMultiSurfaceRead()
{
}


void uiMultiSurfaceRead::dClck( CallBacker* )
{
    singleSurfaceSelected.trigger();
}


void uiMultiSurfaceRead::selCB( CallBacker* cb )
{
    if ( !rgfld_->mainObject() || !rgfld_->mainObject()->isDisplayed() ) return;

    const int nrsel = ioobjselgrp_->nrChosen();
    if( nrsel == 0 )
	return;

    if ( nrsel > 1 )
    {
	EM::SurfaceIOData sd;
	TrcKeySampling hs( false );
	if ( !processInput() ) return;
	for ( int idx=0; idx<nrsel; idx++ )
	{
	    const DBKey& dbky = ioobjselgrp_->chosenID( idx );

	    EM::IOObjInfo eminfo( dbky );
	    if ( !eminfo.isOK() ) continue;
	    TrcKeySampling emhs;
	    emhs.set( eminfo.getInlRange(), eminfo.getCrlRange() );

	    if ( hs.isEmpty() )
		hs = emhs;
	    else if ( !emhs.isEmpty() )
		hs.include( emhs, false );
	}

	fillRangeFld( hs );
	return;
    }

    if ( processInput() )
    {
	EM::SurfaceIOData sd;
	if ( getSurfaceIOData(ioobjselgrp_->chosenID(0),sd,false) )
	    fillFields( sd );
    }
}


void uiMultiSurfaceRead::setSurfaceIds( const DBKeySet& dbkys )
{
    ioobjselgrp_->setChosen( dbkys );
}


void uiMultiSurfaceRead::getSurfaceIds( DBKeySet& dbkys ) const
{
    dbkys.erase();
    const int nrsel = ioobjselgrp_->nrChosen();
    uiStringSet errormsgstr;
    for ( int idx=0; idx<nrsel; idx++ )
    {
	const DBKey dbky = ioobjselgrp_->chosenID( idx );
	const EM::IOObjInfo info( dbky );
	EM::SurfaceIOData sd;
	uiString errmsg;
	if ( info.getSurfaceData(sd,errmsg) )
	    dbkys += dbky;
	else
	{
	    if ( !info.ioObj() )
		continue;

	    errormsgstr += toUiString("'%1': %2\n").arg(info.ioObj()->name())
					.arg( errmsg );
	}

    }

    if ( !errormsgstr.isEmpty() )
    {
	if ( nrsel == 1  )
	    uiMSG().error( errormsgstr );
	else
	    uiMSG().error(
		    tr("The following surfaces will not be loaded:\n\n%1")
		    .arg(errormsgstr.cat()) );
    }

}


void uiMultiSurfaceRead::getSurfaceSelection(
					EM::SurfaceIODataSelection& sel ) const
{
    uiIOSurface::getSelection( sel );
}
