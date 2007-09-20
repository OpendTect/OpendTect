#ifndef uioddisplaytreeitem_h
#define uioddisplaytreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: uioddisplaytreeitem.h,v 1.5 2007-09-20 06:15:17 cvssulochana Exp $
________________________________________________________________________


-*/

#include "uiodtreeitem.h"
class uiVisPartServer;
class uiODDataTreeItem;
namespace Attrib { class SelSpec; }


class uiODDisplayTreeItem : public uiODTreeItem
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

    virtual bool	askContinueAndSaveIfNeeded()	{ return true; }

    virtual void	createMenuCB(CallBacker*);
    virtual void	handleMenuCB(CallBacker*);
    
    uiVisPartServer*	visserv_;
    int			displayid_;

    MenuItem		selattrmnuitem_;
    MenuItem		addattribmnuitem_;
    MenuItem		duplicatemnuitem_;
    MenuItem		removemnuitem_;
    MenuItem            lockmnuitem_;
};


#endif
