/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		August 2016
___________________________________________________________________

-*/

#include "uiodsceneparenttreeitem.h"
#include "uiodscenetreeitem.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uivispartserv.h"


uiODSceneParentTreeItem::uiODSceneParentTreeItem( const uiString& nm )
    : uiPresManagedParentTreeItem(nm)
{
}


uiODSceneParentTreeItem::~uiODSceneParentTreeItem()
{
    detachAllNotifiers();
}


uiODApplMgr* uiODSceneParentTreeItem::applMgr() const
{
    return &ODMainWin()->applMgr();
}


int uiODSceneParentTreeItem::sceneID() const
{
    mDynamicCastGet(uiODSceneTreeTop*,treetop,parent_);
    return treetop ? treetop->sceneID() : -1;
}


bool uiODSceneParentTreeItem::init()
{
    uiODScene* scene = applMgr()->sceneMgr().getScene( sceneID() );
    if ( !scene )
	return false;

    setPrManagedViewer( *scene );
    return uiODTreeItem::init();
}


bool uiODSceneParentTreeItem::anyButtonClick( uiTreeViewItem* item )
{
    if ( item!=uitreeviewitem_ )
	return uiTreeItem::anyButtonClick( item );

    if ( !select() ) return false;

    applMgr()->updateColorTable( -1, -1 );
    return true;
}


void uiODSceneParentTreeItem::setMoreObjectsToDoHint( bool yn )
{
    applMgr()->visServer()->setMoreObjectsToDoHint( sceneID(), yn );
}


Presentation::ViewerID uiODSceneParentTreeItem::viewerID() const
{
    return ViewerID( uiODSceneMgr::theViewerTypeID(),
		     Presentation::ViewerObjID::get(sceneID()) );
}
