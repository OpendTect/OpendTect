/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jul 2003
___________________________________________________________________

-*/

#include "uiodtreeitem.h"

#include "keyboardevent.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uitreeview.h"



uiODTreeItem::uiODTreeItem( const uiString& nm )
    : uiTreeItem(nm)
    , showallitems_(tr("Show All Items"),cShowAllItems)
    , hideallitems_(tr("Hide All Items"),cHideAllItems)
    , removeallitems_(tr("Remove All Items from Tree"),cRemoveAllItems)
    , expandallitems_(tr("Expand All Items"),cExpandAllItems)
    , collapseallitems_(tr("Collapse All Items"),cCollapseAllItems)
{}


bool uiODTreeItem::anyButtonClick( uiTreeViewItem* item )
{
    if ( item!=uitreeviewitem_ )
	return uiTreeItem::anyButtonClick( item );

    return select();
}


bool uiODTreeItem::init()
{
    if ( !uiTreeItem::init() )
	return false;

    const char* iconnm = iconName();
    if ( iconnm )
	uitreeviewitem_->setIcon( 0, iconnm );

    return true;
}


void uiODTreeItem::addStandardItems( uiMenu& mnu )
{
    if ( children_.size() < 2 )
	return;

    mnu.insertSeparator();
    uiAction* action = new uiAction( tr("Show All Items") );
    action->setEnabled( !allChildrenChecked() );
    mnu.insertAction( action, cShowAllItems );

    action = new uiAction( tr("Hide All Items") );
    action->setEnabled( !allChildrenUnchecked() );
    mnu.insertAction( action, cHideAllItems );

    mnu.insertAction( new uiAction(tr("Remove All Items from Tree")),
		      cRemoveAllItems );

    if ( !children_.first() || children_.first()->nrChildren()==0 )
	return;

    mnu.insertSeparator();
    action = new uiAction( tr("Expand All Items") );
    action->setEnabled( !allChildrenExpanded() );
    mnu.insertAction( action, cExpandAllItems );

    action = new uiAction( tr("Collapse All Items") );
    action->setEnabled( !allChildrenCollapsed() );
    mnu.insertAction( action, cCollapseAllItems );
}


void uiODTreeItem::addStandardItems( MenuHandler* menu )
{
    if ( children_.size() < 2 )
	return;

    mAddMenuItem( menu, &showallitems_, !allChildrenChecked(), false );
    mAddMenuItem( menu, &hideallitems_, !allChildrenUnchecked(), false );
    mAddMenuItem( menu, &removeallitems_, true, false );

    if ( !children_.first() || children_.first()->nrChildren()==0 )
	return;

    mAddMenuItem( menu, &expandallitems_, !allChildrenExpanded(), false );
    mAddMenuItem( menu, &collapseallitems_, !allChildrenCollapsed(), false );
}


void uiODTreeItem::handleStandardItems( int menuid )
{
    if ( menuid == cRemoveAllItems )
	removeAllItems();
    else
    {
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    if ( menuid == cShowAllItems )
		children_[idx]->setChecked( true, true );
	    else if ( menuid == cHideAllItems )
		children_[idx]->setChecked( false, true );
	    else if ( menuid == cExpandAllItems )
		children_[idx]->expand();
	    else if ( menuid == cCollapseAllItems )
		children_[idx]->collapse();
	}
    }
}


void uiODTreeItem::handleStandardMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, menuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( !menu || menu->isHandled() || menuid==-1 )
	return;

    for ( int idx=0; idx<children_.size(); idx++ )
    {
	if ( menuid == showallitems_.id )
	    children_[idx]->setChecked( true, true );
	else if ( menuid == hideallitems_.id )
	    children_[idx]->setChecked( false, true );
	else if ( menuid == expandallitems_.id )
	    children_[idx]->expand();
	else if ( menuid == collapseallitems_.id )
	    children_[idx]->collapse();
    }

    if ( menuid == removeallitems_.id )
	removeAllItems();
}


void uiODTreeItem::removeAllItems( bool showmsg )
{
    if ( showmsg )
    {
	const uiString msg = tr("All %1 items will be removed from tree.\n"
				"Do you want to continue?").arg(name());
	if ( !gUiMsg().askRemove(msg) )
	    return;
    }

    while ( children_.size() )
    {
	if ( !children_[0] )
	    continue;
	children_[0]->prepareForShutdown();
	mDynamicCastGet(uiODTreeItem*,odtreeitem,children_[0]);
	if ( odtreeitem && odtreeitem->nrChildren() )
	    odtreeitem->removeAllItems( false );
	removeChild( children_[0] );
    }
}
