#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uiodtreeitem.h"

class uiODDataTreeItem;
class uiVisPartServer;
namespace Attrib { class SelSpec; }


mExpClass(uiODMain) uiODDisplayTreeItem : public uiODTreeItem
{ mODTextTranslationClass(uiODDisplayTreeItem)
public:

    static bool		create(uiTreeItem*,uiODApplMgr*,VisID displayid);
			//!< Creates an instance (if possible)
			//!< and adds it to the tree

			uiODDisplayTreeItem();
    virtual		~uiODDisplayTreeItem();

    void		updateColumnText(int);
    bool		showSubMenu();
    virtual bool	actModeWhenSelected() const	{ return false; }
    void		updateCheckStatus();
    void		show(bool);

    VisID		displayID() const		{ return displayid_; }
    bool		isDisplayID(int) const;

    uiODDataTreeItem*	addAttribItem();
    void		prepareForShutdown();
    void		handleAddAttrib();

    virtual void	setOnlyAtSectionsDisplay(bool);
    virtual bool	displayedOnlyAtSections() const;

protected:

    virtual uiODDataTreeItem* createAttribItem(const Attrib::SelSpec*) const;

    bool		shouldSelect(int selkey) const;
    int			selectionKey() const;
    int			uiTreeViewItemType() const;
    virtual void	checkCB(CallBacker*);
    virtual void	keyPressCB(CallBacker*);
    virtual bool	doubleClick(uiTreeViewItem*);
    virtual bool	init();

    bool		isSelectable() const		{ return true; }
    bool		isExpandable() const		{ return false; }
    uiString		getLockMenuText() const;

    virtual uiString	createDisplayName() const;
    void		updateLockPixmap(bool islocked);
    void		selectRGBA(const Pos::GeomID&);
    virtual void	colorChgCB(CallBacker*);

    virtual bool	askContinueAndSaveIfNeeded(bool withcancel)
			{ return true; }

    void		addToToolBarCB(CallBacker*);
    void		createMenuCB(CallBacker*);
    virtual void	createMenu(MenuHandler*,bool istb);
    virtual void	handleMenuCB(CallBacker*);
    virtual void	deleteObject();

    uiVisPartServer*	visserv_;
    VisID		displayid_;

    MenuItem		addmnuitem_;
    MenuItem		displaymnuitem_;
    MenuItem		addattribmnuitem_;
    MenuItem		addvolprocmnuitem_;
    MenuItem		duplicatemnuitem_;
    MenuItem		removemnuitem_;
    MenuItem		lockmnuitem_;
    MenuItem		hidemnuitem_;
    MenuItem		histogrammnuitem_;

public:
    virtual bool	isOnlyAtSections() const
			{ return displayedOnlyAtSections(); }
};

