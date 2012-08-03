#ifndef uioddisplaytreeitem_h
#define uioddisplaytreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: uioddisplaytreeitem.h,v 1.21 2012-08-03 13:01:03 cvskris Exp $
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uiodtreeitem.h"

class uiODDataTreeItem;
class uiVisPartServer;
namespace Attrib { class SelSpec; }


mClass(uiODMain) uiODDisplayTreeItem : public uiODTreeItem
{
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

protected:

    virtual uiODDataTreeItem* createAttribItem(const Attrib::SelSpec*) const;

    bool		shouldSelect(int selkey) const;
    int			selectionKey() const;
    int			uiListViewItemType() const;
    virtual void	checkCB(CallBacker*);
    virtual bool	init();

    bool		isSelectable() const		{ return true; }
    bool		isExpandable() const		{ return false; }
    const char*		getLockMenuText();

    virtual BufferString createDisplayName() const;
    void		updateLockPixmap(bool islocked);

    virtual bool	askContinueAndSaveIfNeeded(bool withcancel)	
			{ return true; }

    void		addToToolBarCB(CallBacker*);
    void		createMenuCB(CallBacker*);
    virtual void	createMenu(MenuHandler*,bool istb);
    virtual void	handleMenuCB(CallBacker*);
    
    uiVisPartServer*	visserv_;
    int			displayid_;

    MenuItem		addmnuitem_;
    MenuItem		displaymnuitem_;
    MenuItem		addattribmnuitem_;
    MenuItem		addvolprocmnuitem_;
    MenuItem		duplicatemnuitem_;
    MenuItem		removemnuitem_;
    MenuItem            lockmnuitem_;
    MenuItem		hidemnuitem_;
    MenuItem		histogrammnuitem_;
};


#endif

