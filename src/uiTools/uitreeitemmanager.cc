/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: uitreeitemmanager.cc,v 1.6 2003-12-17 12:31:51 kristofer Exp $";


#include "uitreeitemmanager.h"
#include "uimenu.h"
#include "errh.h"
#include "uilistview.h"


uiTreeItem::uiTreeItem( const char* name__ )
    : parent( 0 )
    , name_( name__ )
    , uilistviewitem( 0 )
{
}


uiTreeItem::~uiTreeItem()
{
    while ( children.size() )
	removeChild( children[0] );
}


const char* uiTreeItem::name() const
{
    return name_.buf();
}



bool uiTreeItem::rightClick( uiListViewItem* item )
{
    if ( item==uilistviewitem )
    {
	showSubMenu();
	return true;
    }

    for ( int idx=0; idx<children.size(); idx++ )
    {
	if ( children[idx]->rightClick(item) )
	    return true;
    }

    return false;
}


bool uiTreeItem::anyButtonClick( uiListViewItem* item )
{
    if ( item==uilistviewitem )
	return select();

    for ( int idx=0; idx<children.size(); idx++ )
    {
	if ( children[idx]->anyButtonClick(item) )
	    return true;
    }

    return false;
}


void uiTreeItem::updateSelection( int selid, bool downward )
{
    if ( uilistviewitem )
	uilistviewitem->setSelected( selid!=-1 && selid==selectionKey() );

    if ( downward )
    {
	for ( int idx=0; idx<children.size(); idx++ )
	    children[idx]->updateSelection(selid,downward);
    }
    else if ( parent )
    {
	parent->updateSelection( selid, false );
    }
}


bool uiTreeItem::select(int selkey)
{
    return parent ? parent->select(selkey) : false;
}


bool uiTreeItem::select()
{
    return select(selectionKey());
}




void uiTreeItem::updateColumnText( int col )
{
    for ( int idx=0; idx<children.size(); idx++ )
	children[idx]->updateColumnText(col);

    if ( !uilistviewitem ) return;

    if ( !col )
    {
	uilistviewitem->setText( name_, col );
    }
}


const uiTreeItem* uiTreeItem::findChild( const char* nm ) const
{
    if ( !strcmp( nm, name_ ) )
	return this;

    for ( int idx=0; idx<children.size(); idx++ )
    {
	const uiTreeItem* res = children[idx]->findChild(nm);
	if ( res )
	    return res;
    }

    return 0;
}
	

int uiTreeItem::uiListViewItemType() const
{
    return uiListViewItem::Standard;
}


uiParent* uiTreeItem::getUiParent()
{
    return parent ? parent->getUiParent() : 0;
}


void uiTreeItem::setListViewItem( uiListViewItem* item )
{
    uilistviewitem=item;
    uilistviewitem->setExpandable(isExpandable());
    uilistviewitem->setSelectable(isSelectable());
}


#define mAddChildImpl( uiparentlist ) \
if ( newitem->parentType()==typeid(*this).name() ) \
{ \
    children += newitem; \
    newitem->parent = this; \
 \
    newitem->setListViewItem( new uiListViewItem(uiparentlist, \
		  uiListViewItem::Setup(newitem->name(), \
		  (uiListViewItem::Type)newitem->uiListViewItemType())) ); \
 \
    if ( !newitem->init() ) \
    { \
	removeChild( newitem ); \
	return true; \
    } \
 \
    if ( uilistviewitem ) uilistviewitem->setOpen( true ); \
    updateColumnText(0); updateColumnText(1); \
    updateSelection(selectionKey(),false);  \
    return true; \
} \
 \
if ( downwards ) \
{ \
    for ( int idx=0; idx<children.size(); idx++ ) \
    { \
	if ( children[idx]->addChild( newitem, downwards ) ) \
	    return true; \
    } \
} \
else if ( parent ) \
    return parent->addChild( newitem, downwards ); \
 \
return false; 


bool uiTreeItem::addChild( uiTreeItem* newitem )
{
    return addChild( newitem, false );
}


bool uiTreeItem::addChild( uiTreeItem* newitem, bool downwards )
{
    mAddChildImpl( uilistviewitem );
}


void uiTreeItem::removeChild( uiTreeItem* treeitem )
{
    const int idx=children.indexOf( treeitem );
    if ( idx<0 )
	return;

    if ( uilistviewitem ) uilistviewitem->removeItem( treeitem->getItem() );
    delete children[idx];
    children.remove( idx );
}


uiTreeTopItem::uiTreeTopItem( uiListView* listview_ )
    : uiTreeItem( listview_->name() )
    , listview( listview_ )
    , disabrightclick(false)
{
    listview->rightButtonClicked.notify(
	    		mCB(this,uiTreeTopItem,rightClickCB) );
    listview->mouseButtonClicked.notify(
	    		mCB(this,uiTreeTopItem,anyButtonClickCB) );
}


uiTreeTopItem::~uiTreeTopItem()
{
    listview->rightButtonClicked.remove(
	    		mCB(this,uiTreeTopItem,rightClickCB) );
    listview->mouseButtonClicked.remove(
	    		mCB(this,uiTreeTopItem,anyButtonClickCB) );
}


bool uiTreeTopItem::addChild( uiTreeItem* newitem )
{
    return addChild( newitem, true );
}



bool uiTreeTopItem::addChild( uiTreeItem* newitem, bool downwards )
{
    downwards = true;		//We are at the top, so we should go downwards
    mAddChildImpl( listview );
}


void uiTreeTopItem::rightClickCB( CallBacker* )
{
    if ( disabrightclick ) return;
    rightClick( listview->itemNotified() );
}


void uiTreeTopItem::anyButtonClickCB( CallBacker* cb )
{
    anyButtonClick( listview->itemNotified() );
}


void uiTreeTopItem::updateSelection( int selectionkey, bool dw )
{
    uiTreeItem::updateSelection( selectionkey, true );
    listview->triggerUpdate();
}


void uiTreeTopItem::updateColumnText(int col)
{
    //Is only impl to have it nicely together with updateSelection at the
    //public methods in the headerfile.
    uiTreeItem::updateColumnText(col);
}


uiParent* uiTreeTopItem::getUiParent()
{
    return listview->parent();
}


uiTreeFactorySet::uiTreeFactorySet()
    : addnotifier( this )
    , removenotifier( this )
{}


uiTreeFactorySet::~uiTreeFactorySet()
{
    deepErase( factories );
}


void uiTreeFactorySet::addFactory(uiTreeItemFactory* ptr)
{
    factories += ptr;
    addnotifier.trigger(factories.size()-1);
}


void uiTreeFactorySet::remove( const char* nm )
{
    int index = -1;
    for ( int idx=0; idx<factories.size(); idx++ )
    {
	if ( !strcmp(nm,factories[idx]->name()) )
	{
	    index=idx;
	    break;
	}
    }

    if ( index<0 )
	return;

    removenotifier.trigger(index);
    delete factories[index];
    factories.remove(index);
}


int uiTreeFactorySet::nrFactories() const
{
    return factories.size();
}


const uiTreeItemFactory* uiTreeFactorySet::getFactory(int idx) const
{
    return idx<nrFactories() ? factories[idx] : 0;
}
