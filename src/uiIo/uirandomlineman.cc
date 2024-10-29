/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uirandomlineman.h"

#include "ctxtioobj.h"
#include "ioman.h"
#include "randomlinegeom.h"
#include "randomlinetr.h"
#include "od_helpids.h"

#include "uigisexp.h"
#include "uigisexpdlgs.h"
#include "uimsg.h"
#include "uipickpartserv.h"


mDefineInstanceCreatedNotifierAccess(uiRandomLineMan)

uiRandomLineMan::uiRandomLineMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup(
             uiStrings::phrManage( uiStrings::sRandomLine(mPlural)),mNoDlgTitle,
                 mODHelpKey(mRandomLineManHelpID) )
                 .nrstatusflds(1).modal(false),
		   RandomLineSetTranslatorGroup::ioContext())
{
    createDefaultUI();
    addExtraButton( uiGISExpStdFld::strIcon(),
		    uiGISExpStdFld::sToolTipTxt(),
		    mCB(this,uiRandomLineMan,exportToGISCB) );
    mTriggerInstanceCreatedNotifier();
}


uiRandomLineMan::~uiRandomLineMan()
{}


void uiRandomLineMan::exportToGISCB( CallBacker* )
{
    TypeSet<MultiID> ids;
    getChosen( ids );
    if ( ids.isEmpty() )
    {
	uiMSG().error( tr("Please select at least one Random line") );
	return;
    }

    PtrMan<Geometry::RandomLineSet> rdlset = new Geometry::RandomLineSet;
    uiString errmsg;
    for ( const auto& id : ids )
    {
	PtrMan<IOObj> obj = IOM().get( id );
	if ( !obj )
	    continue;

	Geometry::RandomLineSet rdls;
	const bool res =
		RandomLineSetTranslator::retrieve( rdls, obj.ptr(), errmsg );
	if ( !res )
	    continue;

	rdlset->addLine( *rdls.getRandomLine(0) );
    }

    if ( rdlset->isEmpty() )
    {
	uiMSG().error( tr("Cannot export Random lines"), errmsg );
	return;
    }

    RefObjectSet<const Pick::Set> gisdata;
    for ( int idx=0; idx<rdlset->size(); idx++ )
    {
	ConstRefMan<Geometry::RandomLine> rl = rdlset->getRandomLine( idx );
	if ( !rl || rl->size() < 2 )
	    continue;

	RefMan<Pick::Set> pickset = new Pick::Set();
	uiPickPartServer::convert( *rl.ptr(), *pickset.ptr() );
	if ( pickset->isEmpty() )
	    continue;

	gisdata.add( pickset.ptr() );
    }

    if ( gisdata.isEmpty() )
    {
	uiMSG().error( tr("Cannot export Random lines"), errmsg );
	return;
    }

    uiGISExportDlg dlg( this, uiGISExportDlg::Type::RandomLine, gisdata);
    dlg.go();
}


void uiRandomLineMan::mkFileInfo()
{
    if ( !curioobj_ )
    {
	setInfo( "" );
	return;
    }

    setInfo( getFileInfo() );
}
