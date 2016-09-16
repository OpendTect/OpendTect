#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uiodattribtreeitem.h"
#include "uioddisplaytreeitem.h"
#include "dbkey.h"
#include "sets.h"

class uiCreateAttribLogDlg;
class uiD2TMLogSelDlg;

mExpClass(uiODMain) uiODWellParentTreeItem : public uiODSceneTreeItem
{ mODTextTranslationClass(uiODWellParentTreeItem)
    typedef uiODTreeItem	inheritedClass;
public:
			uiODWellParentTreeItem();

protected:

			mMenuOnAnyButton
    const char*		iconName() const;
    bool		showSubMenu();
    bool		handleSubMenu(int);
    const char*		parentType() const
			{ return typeid(uiODSceneTreeTop).name(); }
    bool 		constlogsize_;
};


mExpClass(uiODMain) uiODWellTreeItemFactory : public uiODSceneTreeItemFactory
{ mODTextTranslationClass(uiODWellTreeItemFactory)
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const { return new uiODWellParentTreeItem(); }
    uiTreeItem*		createForVis(int visid,uiTreeItem*) const;
};


mExpClass(uiODMain) uiODWellTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODWellTreeItem)
public:
    			uiODWellTreeItem( int );
    			uiODWellTreeItem( const DBKey& mid );
    			~uiODWellTreeItem();

protected:
    void		initMenuItems();
    bool		init();
    bool		askContinueAndSaveIfNeeded(bool withcancel);
    virtual void	createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    const char*		parentType() const
			{ return typeid(uiODWellParentTreeItem).name(); }

    DBKey		mid_;
    MenuItem		attrmnuitem_;
    MenuItem		logcubemnuitem_;
    MenuItem		sellogmnuitem_;
    MenuItem		propertiesmnuitem_;
    MenuItem		logviewermnuitem_;
    MenuItem		nametopmnuitem_;
    MenuItem		namebotmnuitem_;
    MenuItem		markermnuitem_;
    MenuItem		markernamemnuitem_;
    MenuItem		showlogmnuitem_;
    MenuItem		showmnuitem_;
    MenuItem		editmnuitem_;
    MenuItem		storemnuitem_;
    MenuItem		gend2tmmnuitem_;
    MenuItem		amplspectrummnuitem_;
    ObjectSet<MenuItem>	logmnuitems_;
};
