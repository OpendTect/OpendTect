/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	DZH
 Date:		Apr 2016
________________________________________________________________________

-*/

#include "uiodvw2demtreeitem.h"

#include "uitreeitemmanager.h"
#include "uiodapplmgr.h"
#include "uiodviewer2dmgr.h"
#include "uiempartserv.h"
#include "uimpepartserv.h"
#include "uivispartserv.h"

#include "uiodvw2dhor2dtreeitem.h"
#include "uiodvw2dhor3dtreeitem.h"

#include "mpeengine.h"
#include "commondefs.h"
#include "emmanager.h"
#include "emobject.h"
#include "ptrman.h"
#include "ioobj.h"


uiODVw2DEMTreeItem::uiODVw2DEMTreeItem( const DBKey& emid )
    : uiODVw2DTreeItem(uiString::emptyString())
    , emid_( emid )
{
}


uiODVw2DEMTreeItem::~uiODVw2DEMTreeItem()
{}


void uiODVw2DEMTreeItem::doSave()
{
    bool savewithname = false;
    if ( emid_.isValid() )
    {
	PtrMan<IOObj> ioobj = DBM().get( emid_ );
	savewithname = !ioobj;
    }
    doStoreObject( savewithname );

    if ( MPE::engine().hasTracker(emid_) )
    {
	uiMPEPartServer* mps = applMgr()->mpeServer();
	if ( mps )
	    mps->saveSetup( emid_ );
    }

}


void uiODVw2DEMTreeItem::doSaveAs()
{
    doStoreObject( true );

    if ( MPE::engine().hasTracker(emid_) )
    {
	uiMPEPartServer* mps = applMgr()->mpeServer();
	if ( mps )
	{
	   mps->prepareSaveSetupAs( emid_ );
	   mps->saveSetupAs( emid_ );
	}
    }
}


void uiODVw2DEMTreeItem::doStoreObject( bool saveas )
{
    applMgr()->EMServer()->storeObject( emid_, saveas );
    renameVisObj();
}


void uiODVw2DEMTreeItem::renameVisObj()
{
    TypeSet<int> visobjids;
    applMgr()->visServer()->findObject( emid_, visobjids );
    name_ = ::toUiString( DBM().nameOf(emid_) );
    for ( int idx = 0; idx<visobjids.size(); idx++ )
	applMgr()->visServer()->setObjectName( visobjids[idx], name_ );
    uiTreeItem::updateColumnText(uiODViewer2DMgr::cNameColumn());
    applMgr()->visServer()->triggerTreeUpdate();
}
