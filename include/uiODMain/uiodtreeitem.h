#ifndef uiodtreeitem_h
#define uiodtreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: uiodtreeitem.h,v 1.20 2006-03-03 22:03:42 cvskris Exp $
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

namespace Attrib { class SelSpec; };

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


class uiODDataTreeItem;

typedef uiODDataTreeItem*(uiDataTreeItemCrator)(const Attrib::SelSpec&,
						const char*);


/*!Base class for the attribs on a treeitem. */


class uiODDataTreeItem : public uiTreeItem
{
public:
    			uiODDataTreeItem(const char* parenttype);
			~uiODDataTreeItem();

    static const int	cPixmapWidth() { return 16; }
    static const int	cPixmapHeight() { return 10; }

    static uiODDataTreeItem*	create(const Attrib::SelSpec&,
	    			       const char* parenttype);
    				/*!<Creates an item based on the selspec. This
				    is used to create custom items like
				    the overlay item. */
    static void			addFactory(uiDataTreeItemCrator);
    				/*!<Adds custom create function for create
				    function. */

    bool		select();

protected:
    int			uiListViewItemType() const;
    bool		init();

    void		checkCB(CallBacker*);
    bool		shouldSelect(int) const;

    uiODApplMgr*	applMgr() const;
    uiSoViewer*		viewer() const;
    int			sceneID() const;
    bool		isSelectable() const { return true; }
    bool		isExpandable() const { return false; }
    const char*		parentType() const { return parenttype_; }
    int			displayID() const;
    bool		showSubMenu();

    virtual void	createMenuCB(CallBacker*);
    virtual void	handleMenuCB(CallBacker*);
    void		updateColumnText(int col);
    virtual BufferString createDisplayName() const			= 0;

    uiMenuHandler*	menu_;
    MenuItem		movemnuitem_;
    MenuItem		movetotopmnuitem_;
    MenuItem		movetobottommnuitem_;
    MenuItem		moveupmnuitem_;
    MenuItem		movedownmnuitem_;

    MenuItem		removemnuitem_;
    const char*		parenttype_;

    static ObjectSet<uiDataTreeItemCrator>	creators_;
};


/*! Implemntation of uiODDataTreeItem for standard attribute displays. */


class uiODAttribTreeItem : public uiODDataTreeItem
{
public:
    			uiODAttribTreeItem( const char* parenttype );
			~uiODAttribTreeItem();
    static BufferString	createDisplayName( int visid, int attrib );
    static void		createSelMenu(MenuItem&,int visid,int attrib,
	    			      int sceneid);
    static bool		handleSelMenu(int mnuid,int visid,int attrib);
    static const char*	sKeySelAttribMenuTxt();
protected:

    bool		anyButtonClick(uiListViewItem*);

    void		createMenuCB( CallBacker* );
    void		handleMenuCB( CallBacker* );
    void		updateColumnText( int col );
    BufferString	createDisplayName() const;

    MenuItem		selattrmnuitem_;
};

#endif

