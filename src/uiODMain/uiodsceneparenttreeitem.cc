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
    : uiODPrManagedParentTreeItem(nm)
{
}


uiODSceneParentTreeItem::~uiODSceneParentTreeItem()
{
    detachAllNotifiers();
}


uiODApplMgr* uiODSceneParentTreeItem::applMgr()
{
    void* res = 0;
    getPropertyPtr( uiODSceneTreeTop::applmgrstr(), res );
    return reinterpret_cast<uiODApplMgr*>( res );
}


int uiODSceneParentTreeItem::sceneID() const
{
    int sceneid=-1;
    getProperty<int>( uiODSceneTreeTop::sceneidkey(), sceneid );
    return sceneid;
}


bool uiODSceneParentTreeItem::init()
{
    uiODScene* scene = applMgr()->sceneMgr().getScene( sceneID() );
    if ( !scene )
	return false;

    setPRManagedViewer( *scene );
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
{ applMgr()->visServer()->setMoreObjectsToDoHint( sceneID(), yn ); }
