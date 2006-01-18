#ifndef uiodtreeitem_h
#define uiodtreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: uiodtreeitem.h,v 1.12 2006-01-18 22:58:59 cvskris Exp $
________________________________________________________________________


-*/

#include "uitreeitemmanager.h"
#include "menuhandler.h"

class uiListView;
class uiListViewItem;
class uiMenuHandler;
class uiODApplMgr;
class uiParent;
class uiPopupMenu;
class uiSoViewer;

class uiODTreeItem : public uiTreeItem
{
public:
    			uiODTreeItem(const char*);

protected:

    uiODApplMgr*	applMgr();
    uiSoViewer*		viewer();
    int			sceneID() const;

    void		addStandardItems(uiPopupMenu&);
    void		handleStandardItems(int mnuid);
};


class uiODTreeTop : public uiTreeTopItem
{
public:
			uiODTreeTop(uiSoViewer*,uiListView*,
				    uiODApplMgr*,uiTreeFactorySet*);
			~uiODTreeTop();

    static const char*	sceneidkey;
    static const char*	viewerptr;
    static const char*	applmgrstr;
    static const char*	scenestr;

    int			sceneID() const;
    bool		select(int selkey);
    TypeSet<int>	getDisplayIds(int&, bool);
    void		loopOverChildrenIds( TypeSet<int>&, int&, bool, 
	    				     const ObjectSet<uiTreeItem>& );

protected:

    void		addFactoryCB(CallBacker*);
    void		removeFactoryCB(CallBacker*);

    virtual const char*	parentType() const { return 0; } 
    uiODApplMgr*	applMgr();

    uiTreeFactorySet*	tfs;
};



class uiODTreeItemFactory : public uiTreeItemFactory
{
public:

    virtual uiTreeItem*	create(int visid,uiTreeItem*) const { return 0; }
    virtual uiTreeItem*	create(const MultiID&,uiTreeItem*) const { return 0; }

};


class uiODDataTreeItem : public uiTreeItem
{
public:
    			uiODDataTreeItem( const char* parenttype );
			~uiODDataTreeItem();

    static const int	cPixmapWidth() { return 16; }
    static const int	cPixmapHeight() { return 10; }

protected:

    uiODApplMgr*	applMgr();
    uiSoViewer*		viewer();
    int			sceneID() const;
    bool		anyButtonClick(uiListViewItem*);
    bool		isSelectable() const { return true; }
    bool		isExpandable() const { return false; }
    const char*		parentType() const { return parenttype_; }
    int			displayID() const;
    bool		showSubMenu();

    virtual void	createMenuCB( CallBacker* );
    virtual void	handleMenuCB( CallBacker* );
    void		updateColumnText( int col );
    void		updateSiblings();
    BufferString	createDisplayName() const;

    uiMenuHandler*	menu;
    MenuItem		selattrmnuitem;
    MenuItem		settingsmnuitem;

    MenuItem		movemnuitem;
    MenuItem		movetotopmnuitem;
    MenuItem		movetobottommnuitem;
    MenuItem		moveupmnuitem;
    MenuItem		movedownmnuitem;
    const char*		parenttype_;
};

#endif

