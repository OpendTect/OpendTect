/*+
___________________________________________________________________

 CopyRight: 	(C) dGB Beheer B.V.
 Author: 	K. Tingdahl
 Date: 		Jul 2003
 RCS:		$Id: uiodattribtreeitem.cc,v 1.6 2006-07-26 15:36:02 cvsnanne Exp $
___________________________________________________________________

-*/

#include "uiodattribtreeitem.h"

#include "uimenu.h"
#include "uiodapplmgr.h"
#include "uivispartserv.h"

#include "attribsel.h"
#include "ptrman.h"
#include "ioobj.h"
#include "ioman.h"
#include "keystrs.h"

#include "uiattribpartserv.h"
#include "uilistview.h"
#include "uimenuhandler.h"
#include "uiodscenemgr.h"
#include "vissurvscene.h"
#include "viscolortab.h"
#include "vissurvobj.h"
#include "visdataman.h"


uiODAttribTreeItem::uiODAttribTreeItem( const char* parenttype )
    : uiODDataTreeItem( parenttype )
    , selattrmnuitem_( sKeySelAttribMenuTxt() )
{}


uiODAttribTreeItem::~uiODAttribTreeItem()
{}


bool uiODAttribTreeItem::anyButtonClick( uiListViewItem* item )
{
    if ( item!=uilistviewitem_ )
	return uiTreeItem::anyButtonClick( item );

    if ( !select() ) return false;

    uiVisPartServer* visserv = applMgr()->visServer();
    if ( visserv->getColTabId(displayID(),attribNr()) < 0 )
	return false;
//  if ( !visserv->isClassification( displayID(), attribNr() ) )
    applMgr()->modifyColorTable( displayID(), attribNr() );

    return true;
}


void uiODAttribTreeItem::createSelMenu( MenuItem& mnu, int visid, int attrib,
					int sceneid)
{
    const uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( visid, attrib );
    if ( as && visserv->hasAttrib(visid) )
    {
	uiAttribPartServer* attrserv = ODMainWin()->applMgr().attrServer();
	MenuItem* subitem = attrserv->storedAttribMenuItem( *as );
	mAddMenuItem( &mnu, subitem, subitem->nrItems(),
		       subitem->checked );

	subitem = attrserv->calcAttribMenuItem( *as );
	mAddMenuItem( &mnu, subitem, subitem->nrItems(),
			 subitem->checked );

	subitem = attrserv->nlaAttribMenuItem( *as );
	if ( subitem && subitem->nrItems() )
	    mAddMenuItem( &mnu, subitem, true, subitem->checked );

	mDynamicCastGet(visSurvey::Scene*,scene,visserv->getObject(sceneid))
	if ( scene && scene->getDataTransform() )
	{
	    // TODO: get depthdomain key from scene
	    subitem = attrserv->depthdomainAttribMenuItem( *as,
		    				scene->getDepthDomainKey() );
	    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked );
	}
    }
}


const char* uiODAttribTreeItem::sKeySelAttribMenuTxt()
{ return "Select Attribute"; }


void uiODAttribTreeItem::createMenuCB( CallBacker* cb )
{
    const uiVisPartServer* visserv = applMgr()->visServer();

    mDynamicCastGet(uiMenuHandler*,menu,cb);

    selattrmnuitem_.removeItems();
    createSelMenu( selattrmnuitem_, displayID(), attribNr(), sceneID() );

    if ( selattrmnuitem_.nrItems() )
	mAddMenuItem( menu, &selattrmnuitem_,
		!visserv->isLocked(displayID()), false );

    uiODDataTreeItem::createMenuCB( cb );
}


void uiODAttribTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDataTreeItem::handleMenuCB( cb );

    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    if ( handleSelMenu( mnuid, displayID(), attribNr()) )
    {
	menu->setIsHandled(true);
	updateColumnText( uiODSceneMgr::cNameColumn() );
    }
}


bool uiODAttribTreeItem::handleSelMenu( int mnuid, int visid, int attrib )
{
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    if ( mnuid==-1 || visserv->isLocked(visid) )
	return false;

    const Attrib::SelSpec* as = visserv->getSelSpec( visid, attrib );
    if ( !as ) return false;

    uiAttribPartServer* attrserv = ODMainWin()->applMgr().attrServer();

    Attrib::SelSpec myas( *as );
    if ( attrserv->handleAttribSubMenu(mnuid,myas) )
    {
	visserv->setSelSpec( visid, attrib, myas );
	visserv->calculateAttrib( visid, attrib, false );
	return true;
    }

    return false;
}


BufferString uiODAttribTreeItem::createDisplayName( int visid, int attrib )
{
    const uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( visid, attrib );
    BufferString dispname( as ? as->userRef() : 0 );
    if ( as && as->isNLA() )
    {
	dispname = as->objectRef();
	const char* nodenm = as->userRef();
	if ( IOObj::isKey(as->userRef()) )
	    nodenm = IOM().nameOf( as->userRef(), false );
	dispname += " ("; dispname += nodenm; dispname += ")";
    }

    if ( as && as->id()==Attrib::SelSpec::cAttribNotSel() )
	dispname = "<right-click>";
    else if ( !as )
	dispname = visserv->getObjectName( visid );
    else if ( as->id() == Attrib::SelSpec::cNoAttrib() )
	dispname="";

    return dispname;
}


BufferString uiODAttribTreeItem::createDisplayName() const
{
    return createDisplayName( displayID(), attribNr() );
}


void uiODAttribTreeItem::updateColumnText( int col )
{
    if ( col==uiODSceneMgr::cColorColumn() )
    {
	uiVisPartServer* visserv = applMgr()->visServer();
	mDynamicCastGet(visSurvey::SurveyObject*,so,
			visserv->getObject( displayID() ))
	if ( !so )
	{
	    uiODDataTreeItem::updateColumnText( col );
	    return;
	}
	
	if ( !so->hasColor() ) displayMiniCtab( so->getColTabID( attribNr()) );
    }

    uiODDataTreeItem::updateColumnText( col );
}
