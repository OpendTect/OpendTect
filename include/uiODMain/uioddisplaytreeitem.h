#ifndef uioddisplaytreeitem_h
#define uioddisplaytreeitem_h

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
{ mODTextTranslationClass(uiODDisplayTreeItem);
public:

    static bool		create(uiTreeItem*,uiODApplMgr*,int displayid);
			//!< Creates an instance (if possible)
			//!< and adds it to the tree

			uiODDisplayTreeItem();
			~uiODDisplayTreeItem();
    void		updateColumnText(int);
    bool		showSubMenu();
    virtual bool	actModeWhenSelected() const	{ return false; }
    void		updateCheckStatus();

    int			displayID() const		{ return displayid_; }

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
    virtual bool	init();

    bool		isSelectable() const		{ return true; }
    bool		isExpandable() const		{ return false; }
    uiString		getLockMenuText() const;

    virtual uiString	createDisplayName() const;
    void		updateLockPixmap(bool islocked);

    virtual bool	askContinueAndSaveIfNeeded(bool withcancel)
			{ return true; }

    void		addToToolBarCB(CallBacker*);
    void		createMenuCB(CallBacker*);
    virtual void	createMenu(MenuHandler*,bool istb);
    virtual void	handleMenuCB(CallBacker*);
    virtual void	deleteObject();

    uiVisPartServer*	visserv_;
    int			displayid_;

    MenuItem		addmnuitem_;
    MenuItem		displaymnuitem_;
    MenuItem		addattribmnuitem_;
    MenuItem		addvolprocmnuitem_;
    MenuItem		duplicatemnuitem_;
    MenuItem		removemnuitem_;
    MenuItem		lockmnuitem_;
    MenuItem		hidemnuitem_;
    MenuItem		histogrammnuitem_;
};


#endif
