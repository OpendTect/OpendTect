#ifndef uiodwelltreeitem_h
#define uiodwelltreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id: uiodwelltreeitem.h,v 1.3 2008-04-10 05:24:11 cvssatyaki Exp $
________________________________________________________________________


-*/

#include "uiodattribtreeitem.h"
#include "uioddisplaytreeitem.h"
#include "multiid.h"
#include "sets.h"

class uiCreateAttribLogDlg;

class uiODWellParentTreeItem : public uiODTreeItem
{
public:
    			uiODWellParentTreeItem();
    bool		showSubMenu();
    bool		handleSubMenu(int);

protected:
    const char*		parentType() const
			    { return typeid(uiODTreeTop).name(); }
};


class uiODWellTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const { return new uiODWellParentTreeItem; }
    uiTreeItem*		create(int visid,uiTreeItem*) const;
};


class uiODWellTreeItem : public uiODDisplayTreeItem
{
public:
    			uiODWellTreeItem( int );
    			uiODWellTreeItem( const MultiID& mid );
    			~uiODWellTreeItem();

protected:
    void		initMenuItems();
    bool		init();
    bool		askContinueAndSaveIfNeeded();
    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);
    const char*		parentType() const
			{ return typeid(uiODWellParentTreeItem).name(); }

    MultiID		mid;
    MenuItem		attrmnuitem_;
    MenuItem		sellogmnuitem_;
    MenuItem		propertiesmnuitem_;
    MenuItem		nametopmnuitem_;
    MenuItem		namebotmnuitem_;
    MenuItem		markermnuitem_;
    MenuItem		markernamemnuitem_;
    MenuItem		showlogmnuitem_;
    MenuItem		showmnuitem_;
    MenuItem		editmnuitem_;
    MenuItem		storemnuitem_;
};


#endif
