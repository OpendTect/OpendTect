/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Asif
 * DATE     : April 2018
-*/

#include "uitutorialtreeitem.h"

#include "uimenu.h"
#include "uiodmain.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uistrings.h"


uiODTutorialParentTreeItem::uiODTutorialParentTreeItem()
    : uiODSceneParentTreeItem(tr("Tutorial"))
{
}


const char* uiODTutorialParentTreeItem::childObjTypeKey() const
{
    return typeid(*this).name();
}


const char* uiODTutorialParentTreeItem::iconName() const
{
    return "tutorial";
}


void uiODTutorialParentTreeItem::addWells( )
{
    applMgr()->selectWells( wellids_ );

    for ( int idx=0; idx < wellids_.size() ; idx++ )
    {
	uiODTutorialTreeItem *treeitem =
			new uiODTutorialTreeItem( wellids_[idx] );
	addChild( treeitem, true );
    }
}


static const int cAddWells    = 1;


bool uiODTutorialParentTreeItem::handleSubMenu( int mnuid )
{
    if ( mnuid == cAddWells )
	addWells();
    else
	handleStandardItems( mnuid );

    return true;
}

bool uiODTutorialParentTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertAction( new uiAction(
	m3Dots(uiStrings::phrAdd(uiStrings::sWell(mPlural)))), cAddWells );
    addStandardItems( mnu );

    mnu.insertSeparator();
    int mnuid = mnu.exec();

    return mnuid<0 ? false : handleSubMenu( mnuid );
}


/*the factory is used to instantiate the object in uitutpi.cc*/
const char* uiODTutorialParentTreeItemfactory::name() const
{ return typeid(*this).name(); }

uiTreeItem* uiODTutorialParentTreeItemfactory::create() const
{ return new uiODTutorialParentTreeItem(); }

const char* uiODTutorialTreeItem::iconName() const
{ return "tree-well"; }


uiODTutorialTreeItem::uiODTutorialTreeItem( const DBKey& key )
    : key_(key)
{}

bool uiODTutorialTreeItem::init()
{
     auto viswell = new visSurvey::TutorialWellDisplay;
     displayid_ = viswell->id();
     visserv_->addObject( viswell, sceneID(), true );
     viswell->loadAndDisplayWell( key_ );
     return true;
}

