#ifndef treeitem_h
#define treeitem_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          4-11-2002
 RCS:           $Id: uiodvolrentreeitem.h,v 1.1 2007-01-03 18:29:06 cvskris Exp $
________________________________________________________________________


-*/

#include "uioddisplaytreeitem.h"

class uiODVolrenParentTreeItem : public uiTreeItem
{
public:
			uiODVolrenParentTreeItem();
			~uiODVolrenParentTreeItem();

    bool		showSubMenu();
    int			sceneID() const;

protected:
    bool		init();
    const char*		parentType() const;
};


class uiODVolrenTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const   { return getName(); }
    static const char*	getName();
    uiTreeItem*		create() const { return new uiODVolrenParentTreeItem; }
    uiTreeItem*		create(int,uiTreeItem*) const;
};


class uiODVolrenTreeItem : public uiODDisplayTreeItem
{
public:
    			uiODVolrenTreeItem(int displayid_=-1);
    bool		showSubMenu();

protected:
			~uiODVolrenTreeItem();
    bool		init();
    BufferString	createDisplayName() const;
    uiODDataTreeItem*	createAttribItem( const Attrib::SelSpec* ) const;
    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);
    bool		anyButtonClick( uiListViewItem* item );

    bool		isExpandable() const		{ return true; }
    const char*		parentType() const;

    bool		hasVolume() const;

    MenuItem		selattrmnuitem_;
    MenuItem		positionmnuid_;
    MenuItem		addmnuid_;
    MenuItem		addlinlslicemnuid_;
    MenuItem		addlcrlslicemnuid_;
    MenuItem		addltimeslicemnuid_;
    MenuItem		addlvolumemnuid_;
};


class uiODVolrenSubTreeItem : public uiODDisplayTreeItem
{
public:
    			uiODVolrenSubTreeItem(int displayid);

    bool		isVolume() const;
    void		updateColumnText(int col);

protected:
			~uiODVolrenSubTreeItem();
    bool		anyButtonClick( uiListViewItem* item );
    bool		init();
    const char*		parentType() const;
};

#endif
