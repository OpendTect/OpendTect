#ifndef uiodvolrentreeitem_h
#define uiodvolrentreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          4-11-2002
 RCS:           $Id: uiodvolrentreeitem.h,v 1.16 2012-09-07 22:08:02 cvsnanne Exp $
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uioddisplaytreeitem.h"

mClass(uiODMain) uiODVolrenParentTreeItem : public uiTreeItem
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


mClass(uiODMain) uiODVolrenTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const   { return getName(); }
    static const char*	getName();
    uiTreeItem*		create() const { return new uiODVolrenParentTreeItem; }
    uiTreeItem*		createForVis(int,uiTreeItem*) const;
};


mClass(uiODMain) uiODVolrenTreeItem : public uiODDisplayTreeItem
{
public:
    			uiODVolrenTreeItem(int displayid_=-1);
    bool		showSubMenu();

protected:
			~uiODVolrenTreeItem();
    bool		init();
    BufferString	createDisplayName() const;
    uiODDataTreeItem*	createAttribItem( const Attrib::SelSpec* ) const;
    virtual void	createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    bool		anyButtonClick(uiTreeViewItem*);

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
    MenuItem		savevolumemnuitem_;
};


mClass(uiODMain) uiODVolrenSubTreeItem : public uiODDisplayTreeItem
{
public:
    			uiODVolrenSubTreeItem(int displayid);

    bool		isVolume() const;
    bool		isIsoSurface() const;
    void		updateColumnText(int col);

protected:
			~uiODVolrenSubTreeItem();

    virtual void	createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    void		posChangeCB(CallBacker*);
    void		selChgCB(CallBacker*);

    bool		anyButtonClick(uiTreeViewItem*);
    bool		init();
    const char*		parentType() const;

    MenuItem		resetisosurfacemnuitem_;
    MenuItem		convertisotobodymnuitem_;
};

#endif

