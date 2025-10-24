/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitutexternalattribrandom.h"

#include "attribdesc.h"
#include "attribsel.h"
#include "uiodapplmgr.h"
#include "uiodmain.h"
#include "uiodscenemgr.h"
#include "uivispartserv.h"
#include "vissurvobj.h"


// ExternalAttrib::uiRandomTreeItem

void ExternalAttrib::uiRandomTreeItem::initClass()
{
    uiODDataTreeItem::factory().addCreator( create );
}


ExternalAttrib::uiRandomTreeItem::uiRandomTreeItem( const char* parenttype )
    : uiODDataTreeItem( parenttype )
    , generatemenuitem_(uiStrings::sGenerate(),true)
{
    generatemenuitem_.iconfnm = "refresh";
}


ExternalAttrib::uiRandomTreeItem::~uiRandomTreeItem()
{}


bool ExternalAttrib::uiRandomTreeItem::anyButtonClick( uiTreeViewItem* item )
{
    if ( item!=uitreeviewitem_ )
	return uiTreeItem::anyButtonClick( item );

    if ( !select() )
	return false;

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    if ( !visserv || !visserv->getColTabSequence(displayID(),attribNr()) )
	return false;

    ODMainWin()->applMgr().updateColorTable( displayID(), attribNr() );

    return true;
}


uiODDataTreeItem* ExternalAttrib::uiRandomTreeItem::create(
		    const Attrib::SelSpec& as, const char* parenttype )
{
    if ( !Random::sCheckSelSpec(as) )
	return nullptr;

    return new uiRandomTreeItem( parenttype );
}


void ExternalAttrib::uiRandomTreeItem::createMenu( MenuHandler* menu,
						   bool istb )
{
    uiODDataTreeItem::createMenu( menu, istb );

    mAddMenuOrTBItem( istb, nullptr, menu, &generatemenuitem_, true, false );
}


void ExternalAttrib::uiRandomTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDataTreeItem::handleMenuCB( cb );
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);

    if ( mnuid==-1 || menu->isHandled() )
	return;

    uiVisPartServer* visserv = applMgr()->visServer();
    if ( !visserv )
	return;

    if ( mnuid==generatemenuitem_.id )
    {
	menu->setIsHandled( true );
	visserv->calculateAttrib( displayID(), attribNr(), false );
    }
}


uiString ExternalAttrib::uiRandomTreeItem::createDisplayName() const
{
    return Random::createDisplayName();
}


void ExternalAttrib::uiRandomTreeItem::updateColumnText( int col )
{
    if ( col==uiODSceneMgr::cColorColumn() )
    {
	uiVisPartServer* visserv = applMgr()->visServer();
	mDynamicCastGet( visSurvey::SurveyObject*, so,
			 visserv->getObject( displayID() ) )
	if ( !so )
	{
	    uiODDataTreeItem::updateColumnText( col );
	    return;
	}

	if ( !so->hasColor() )
	    displayMiniCtab(so->getColTabSequence(attribNr()));
    }

    uiODDataTreeItem::updateColumnText( col );
}
