/*+
___________________________________________________________________

 CopyRight: 	(C) dGB Beheer B.V.
 Author: 	K. Tingdahl
 Date: 		Jul 2003
___________________________________________________________________

-*/
static const char* rcsID = "$Id: uiodattribtreeitem.cc,v 1.30 2009-05-20 16:10:36 cvshelene Exp $";

#include "uiodattribtreeitem.h"

#include "attribsel.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "filepath.h"
#include "ioobj.h"
#include "ioman.h"
#include "keystrs.h"
#include "ptrman.h"
#include "survinfo.h"

#include "uiattribpartserv.h"
#include "uilistview.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uiviscoltabed.h"
#include "uivispartserv.h"
#include "vissurvobj.h"
#include "vissurvscene.h"



const char* uiODAttribTreeItem::sKeySelAttribMenuTxt()
{ return "Select &Attribute"; }


const char* uiODAttribTreeItem::sKeyMultCompMenuTxt()
{ return "Display &Multi-components stored data"; }


const char* uiODAttribTreeItem::sKeyColSettingsMenuTxt()
{ return "Save &Color Settings"; }


uiODAttribTreeItem::uiODAttribTreeItem( const char* parenttype )
    : uiODDataTreeItem( parenttype )
    , selattrmnuitem_( sKeySelAttribMenuTxt() )
    , multcompmnuitem_( sKeyMultCompMenuTxt() )
    , colsettingsmnuitem_( sKeyColSettingsMenuTxt() )
{}


uiODAttribTreeItem::~uiODAttribTreeItem()
{}


bool uiODAttribTreeItem::anyButtonClick( uiListViewItem* item )
{
    if ( item!=uilistviewitem_ )
	return uiTreeItem::anyButtonClick( item );

    if ( !select() ) return false;

    uiVisPartServer* visserv = applMgr()->visServer();
    if ( !visserv->canSetColTabSequence( displayID() ) &&
	 !visserv->getColTabMapperSetup( displayID(), attribNr() ) )
	return false;
//  if ( !visserv->isClassification( displayID(), attribNr() ) )
    applMgr()->updateColorTable( displayID(), attribNr() );

    return true;
}


// TODO: get depthdomain key from scene
#define mCreateDepthDomMnuItemIfNeeded( is2d, needext ) \
{\
    if ( scene && scene->getDataTransform() )\
    {\
	subitem = attrserv->zDomainAttribMenuItem( *as,\
				scene->getZDomainString(), is2d, needext );\
	mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked );\
    }\
}


#define mCreateItemsList( is2d, needext ) \
{ \
    subitem = attrserv->storedAttribMenuItem( *as, is2d, false ); \
    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked ); \
    subitem = attrserv->calcAttribMenuItem( *as, is2d, needext ); \
    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked ); \
    subitem = attrserv->nlaAttribMenuItem( *as, is2d, needext ); \
    if ( subitem && subitem->nrItems() ) \
	mAddMenuItem( &mnu, subitem, true, subitem->checked ); \
    subitem = attrserv->storedAttribMenuItem( *as, is2d, true ); \
    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked ); \
    mCreateDepthDomMnuItemIfNeeded( is2d, needext ); \
}


void uiODAttribTreeItem::createSelMenu( MenuItem& mnu, int visid, int attrib,
					int sceneid, bool ismulticomp )
{
    const uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( visid, attrib );
    if ( as && visserv->hasAttrib(visid) )
    {
	uiAttribPartServer* attrserv = ODMainWin()->applMgr().attrServer();
	mDynamicCastGet(visSurvey::SurveyObject*,so,visserv->getObject(visid));
	if ( !so ) return;

	Pol2D3D p2d3d = so->getAllowedDataType();
	mDynamicCastGet(visSurvey::Scene*,scene,visserv->getObject(sceneid))

	bool need2dlist = SI().has2D() && p2d3d != Only3D;
	bool need3dlist = SI().has3D() && p2d3d != Only2D;

	if ( ismulticomp )
	{
	    if ( need3dlist )
		attrserv->fillInStoredAttribMenuItem(
				&mnu, false, false, *as, true, need2dlist );
	    if ( need2dlist )
		attrserv->fillInStoredAttribMenuItem(
				&mnu, true, false, *as, true, !need2dlist );

	    return;
	}

	MenuItem* subitem;
	attrserv->resetMenuItems();
	if ( need3dlist )
	    mCreateItemsList( false, need2dlist );
	if ( need2dlist && p2d3d != Only2D )
	    mCreateItemsList( true, need3dlist );
    }
}


void uiODAttribTreeItem::createMenuCB( CallBacker* cb )
{
    const uiVisPartServer* visserv = applMgr()->visServer();

    mDynamicCastGet(uiMenuHandler*,menu,cb);

    bool isonly2d = false;
    mDynamicCastGet(visSurvey::SurveyObject*,so,visserv->getObject(sceneID()));
    if ( so ) isonly2d = so->getAllowedDataType() == Only2D;

    selattrmnuitem_.removeItems();
    createSelMenu( selattrmnuitem_, displayID(), attribNr(), sceneID() );

    if ( selattrmnuitem_.nrItems() || Only2D )
	mAddMenuItem( menu, &selattrmnuitem_,
		      !visserv->isLocked(displayID()), false );

    multcompmnuitem_.removeItems();
    createSelMenu( multcompmnuitem_, displayID(), attribNr(), sceneID(), true );
    if ( multcompmnuitem_.nrItems() )
	mAddMenuItem( menu, &multcompmnuitem_,
		      !visserv->isLocked(displayID()), false );

    const uiAttribPartServer* attrserv = applMgr()->attrServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( displayID(), attribNr() );
    if ( as && attrserv->getIOObj(*as) )
	mAddMenuItem( menu, &colsettingsmnuitem_, true, false );
    
    uiODDataTreeItem::createMenuCB( cb );
}


void uiODAttribTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDataTreeItem::handleMenuCB( cb );

    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    if ( mnuid == colsettingsmnuitem_.id )
    {
	menu->setIsHandled(true);
	applMgr()->saveDefColTab( displayID(), attribNr() );
    }
    else if( handleMultCompSelMenu( mnuid, displayID(), attribNr() ) )
    {
	menu->setIsHandled(true);
	updateColumnText( uiODSceneMgr::cNameColumn() );
    }
    else if ( handleSelMenu( mnuid, displayID(), attribNr()) )
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


bool uiODAttribTreeItem::handleMultCompSelMenu( int mnuid, int visid,
						int attrib )
{
    if ( !multcompmnuitem_.findItem(mnuid) ) return false;
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    if ( mnuid==-1 || visserv->isLocked(visid) )
	return false;

    bool isonly2d = false;
    mDynamicCastGet(visSurvey::SurveyObject*,so,visserv->getObject(sceneID()));
    if ( so ) isonly2d = so->getAllowedDataType() == Only2D;

    const MenuItem* item = multcompmnuitem_.findItem( mnuid );
    uiAttribPartServer* attrserv = ODMainWin()->applMgr().attrServer();
    if ( attrserv->handleMultiCompSubMenu( mnuid, isonly2d, item->text ) )
    {
	Attrib::SelSpec as( "Multi-Textures", Attrib::SelSpec::cOtherAttrib() );
	if ( !applMgr()->calcMultipleAttribs( as ) )
	    return false;
    }

    return true;
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
	
	displayMiniCtab( so->getColTabSequence(attribNr()) );
    }

    uiODDataTreeItem::updateColumnText( col );
}
