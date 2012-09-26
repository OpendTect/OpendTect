/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          November 2011
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiprestacktreeitemmgr.h"

#include "bufstringset.h"
#include "uipseventstreeitem.h"
#include "uiodscenemgr.h"
#include "uiseispartserv.h"


uiPreStackTreeItemManager::uiPreStackTreeItemManager( uiODMain& appl )
    : appl_(appl)
    , treeitem_(0)
{
    IOM().surveyChanged.notify(
	mCB(this,uiPreStackTreeItemManager,surveyChangedCB) );
    surveyChangedCB( 0 );
}


uiPreStackTreeItemManager::~uiPreStackTreeItemManager()
{
}


void uiPreStackTreeItemManager::surveyChangedCB( CallBacker* cb )
{
    uiSeisPartServer* seisserv = appl_.applMgr().seisServer();
    BufferStringSet gnms; seisserv->getStoredGathersList( true, gnms );
    uiTreeFactorySet* factoryset = appl_.sceneMgr().treeItemFactorySet();
    if ( gnms.size() )
    {
	if ( !treeitem_ )
	{
	    treeitem_ = new PSEventsTreeItemFactory;
	    factoryset->addFactory( treeitem_, 8500 );
	}
    }
    else if ( treeitem_ )
    {
	factoryset->remove( treeitem_->name() );
	treeitem_ = 0;
    }
}
