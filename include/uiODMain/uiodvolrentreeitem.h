#ifndef uiodvolrentreeitem_h
#define uiodvolrentreeitem_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          4-11-2002
 RCS:           $Id: uiodvolrentreeitem.h,v 1.7 2008-12-04 19:51:00 cvskris Exp $
________________________________________________________________________


-*/

#include "uioddisplaytreeitem.h"

class uiODVolrenParentTreeItem : public uiTreeItem
{
    typedef uiTreeItem	inheritedClass;
public:
			uiODVolrenParentTreeItem();
			~uiODVolrenParentTreeItem();

			mMenuOnAnyButton;

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
    MenuItem            colsettingsmnuitem_;
    MenuItem		positionmnuitem_;
    MenuItem            statisticsmnuitem_;
    MenuItem            amplspectrummnuitem_;
    MenuItem		addmnuitem_;
    MenuItem		addlinlslicemnuitem_;
    MenuItem		addlcrlslicemnuitem_;
    MenuItem		addltimeslicemnuitem_;
    MenuItem		addvolumemnuitem_;
    MenuItem		addisosurfacemnuitem_;
};


class uiODVolrenSubTreeItem : public uiODDisplayTreeItem
{
public:
    			uiODVolrenSubTreeItem(int displayid);

    bool		isVolume() const;
    bool		isIsoSurface() const;
    void		updateColumnText(int col);

protected:
			~uiODVolrenSubTreeItem();

    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);

    bool		anyButtonClick( uiListViewItem* item );
    bool		init();
    const char*		parentType() const;

    MenuItem		setisovaluemnuitem_;
    MenuItem		convertisotobodymnuitem_;
};

#endif
