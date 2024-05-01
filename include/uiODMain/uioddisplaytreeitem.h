#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

    static bool		create(uiTreeItem*,uiODApplMgr*,const VisID&);
			//!< Creates an instance (if possible)
			//!< and adds it to the tree

    void		updateColumnText(int) override;
    bool		showSubMenu() override;
    virtual bool	actModeWhenSelected() const	{ return false; }
    void		updateCheckStatus() override;
    void		show(bool);

    VisID		displayID() const		{ return displayid_; }
    bool		isDisplayID(int) const;

    uiODDataTreeItem*	addAttribItem();
    void		prepareForShutdown() override;
    void		handleAddAttrib();

    virtual void	setOnlyAtSectionsDisplay(bool);
    virtual bool	displayedOnlyAtSections() const;

protected:
			uiODDisplayTreeItem();
			~uiODDisplayTreeItem();

    virtual uiODDataTreeItem* createAttribItem(const Attrib::SelSpec*) const;

    bool		shouldSelect(int selkey) const override;
    int			selectionKey() const override;
    int			uiTreeViewItemType() const override;
    virtual void	checkCB(CallBacker*);
    virtual void	keyPressCB(CallBacker*);
    bool		doubleClick(uiTreeViewItem*) override;
    bool		init() override;

    bool		isSelectable() const override	{ return true; }
    bool		isExpandable() const override	{ return false; }
    uiString		getLockMenuText() const;

    virtual uiString	createDisplayName() const;
    void		updateLockPixmap(bool islocked);
    void		selectRGBA(const Pos::GeomID&);
    virtual void	colorChgCB(CallBacker*);

    bool		askContinueAndSaveIfNeeded(bool withcancel) override
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
