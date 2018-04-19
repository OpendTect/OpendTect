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


uiODTutorialParentTreeItem::uiODTutorialParentTreeItem()
    : uiODSceneParentTreeItem (tr("Tutorial to Display Well Data"))
{
}


const char* uiODTutorialParentTreeItem::childObjTypeKey() const
{ 
    return typeid(*this).name();
}


const char* uiODTutorialParentTreeItem::iconName() const
{ 
    return "tree-uiODTutorialParentTreeItem"; 
}


void uiODTutorialParentTreeItem::addWells( )
{
    applMgr()->selectWells( wellids_ );
 /* you can use wellobjlist_ ( mIOObjContext( Well ) populate all well ObjIDs*/

    for ( int idx=0; idx < wellids_.size() ; idx++ )
    {
	uiODTutorialTreeItem *treeitem = new uiODTutorialTreeItem( 
				      wellids_[ idx ] );
	addChild(treeitem,true);
    }
}


static const int cAddWells    = 1;
static const int cRemoveWells = 198;
static const int cHideWells   = 199;
static const int cShowWells   = 200;


bool uiODTutorialParentTreeItem::handleSubMenu( int mnuid )
{
    switch ( mnuid )
    {
	case cAddWells:
	    addWells();
	    break;

	case cRemoveWells:  /*remove items*/
	{
	    removeAllChildren();
	    break;
	}

	case cHideWells: /*hide items*/
	{
	    ObjectSet<uiTreeItem> uitrees = this->getChildren();
	    for(int idx = 0; idx < uitrees.size(); idx++ )
	    {
		uitrees[idx]->setChecked( false );
		uiODTutorialTreeItem *t=(uiODTutorialTreeItem*)uitrees[idx];
		t->viswell_->turnOn( false );
	    }
	    break;
	}

	case cShowWells: /*show items*/
	{
	    ObjectSet<uiTreeItem> uitrees=this->getChildren();
	    for(int idx = 0; idx< uitrees.size(); idx++ )
	    {
		uitrees[idx]->setChecked( true );
		uiODTutorialTreeItem *t=(uiODTutorialTreeItem*)uitrees[idx];
		t->viswell_->turnOn( true );
	    }
	    break;
	}
    }
    return true;
}

bool uiODTutorialParentTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertAction( new uiAction( m3Dots(uiStrings::sAdd()) ), 1 );
    addStandardItems( mnu );
    
    mnu.insertSeparator();
    int mnuid=mnu.exec();

    printf("menu cliecked %d\n", mnuid );
    return mnuid<0 ? false : handleSubMenu( mnuid );
}


/*the factory is used to instantiate the object in uitutpi.cc*/
const char* uiODTutorialParentTreeItemfactory::name() const
{return typeid( *this ).name(); }


uiTreeItem* uiODTutorialParentTreeItemfactory::create() const
{ return new uiODTutorialParentTreeItem(); }

const char* uiODTutorialTreeItem::iconName() const
{ 
    return  "tree-well"; 
}

uiODTutorialTreeItem::uiODTutorialTreeItem(const DBKey key)
{
     viswell_ = new visSurvey::TutorialWellDisplay(key);
     viswell_->loadAndDisplayWell();
     displayid_ = viswell_->id();
}

