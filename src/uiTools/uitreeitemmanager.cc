/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: uitreeitemmanager.cc,v 1.13 2004-09-15 06:13:16 kristofer Exp $";


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


void uiTreeItem::prepareForShutdown()
{
    for ( int idx=0; idx<children.size(); idx++ )
	children[idx]->prepareForShutdown();
}


void uiTreeItem::setChecked( bool yn )
{
    if ( uilistviewitem )
	uilistviewitem->setChecked( yn );
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
	

const uiTreeItem* uiTreeItem::findChild( int selkey ) const
{
    if ( selectionKey()==selkey )
	return this;

    for ( int idx=0; idx<children.size(); idx++ )
    {
	const uiTreeItem* res = children[idx]->findChild(selkey);
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
if ( !strcmp(newitem->parentType(), typeid(*this).name()) ) \
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
    uiTreeItem* child = children[idx];
    children.remove( idx );
    delete child;
}


uiTreeTopItem::uiTreeTopItem( uiListView* listview_ )
    : uiTreeItem( listview_->name() )
    , listview( listview_ )
    , disabrightclick(false)
    , disabanyclick(false)
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
    if ( disabanyclick || disabrightclick ) return;
    rightClick( listview->itemNotified() );
}


void uiTreeTopItem::anyButtonClickCB( CallBacker* cb )
{
    if ( disabanyclick ) return;
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


uiTreeCreaterSet::uiTreeCreaterSet()
    : addnotifier( this )
    , removenotifier( this )
{}


uiTreeCreaterSet::~uiTreeCreaterSet()
{
    deepErase( creaters );
}


void uiTreeCreaterSet::addCreater(uiTreeItemCreater* ptr, int placement)
{
    creaters += ptr;
    placementidxs += placement;
    addnotifier.trigger(creaters.size()-1);
}


void uiTreeCreaterSet::remove( const char* nm )
{
    int index = -1;
    for ( int idx=0; idx<creaters.size(); idx++ )
    {
	if ( !strcmp(nm,creaters[idx]->name()) )
	{
	    index=idx;
	    break;
	}
    }

    if ( index<0 )
	return;

    removenotifier.trigger(index);
    delete creaters[index];
    creaters.remove(index);
    placementidxs.remove(index);
}


int uiTreeCreaterSet::nrCreaters() const
{
    return creaters.size();
}


const uiTreeItemCreater* uiTreeCreaterSet::getCreater(int idx) const
{
    return idx<nrCreaters() ? creaters[idx] : 0;
}


int uiTreeCreaterSet::getPlacementIdx(int idx) const
{ return placementidxs[idx]; }
