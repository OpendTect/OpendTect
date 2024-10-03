#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"

#include "multiid.h"
#include "sets.h"
#include "uiodattribtreeitem.h"
#include "uioddisplaytreeitem.h"
#include "viswelldisplay.h"

class uiCreateAttribLogDlg;
class uiD2TMLogSelDlg;

mExpClass(uiODMain) uiODWellParentTreeItem : public uiODParentTreeItem
{ mODTextTranslationClass(uiODWellParentTreeItem)
public:
			uiODWellParentTreeItem();

    static CNotifier<uiODWellParentTreeItem,uiMenu*>& showMenuNotifier();

protected:
			~uiODWellParentTreeItem();

    const char*		iconName() const override;
    bool		showSubMenu() override;
    bool		handleSubMenu(int);
    bool		constlogsize_;
};


mExpClass(uiODMain) uiODWellTreeItemFactory : public uiODTreeItemFactory
{ mODTextTranslationClass(uiODWellTreeItemFactory)
public:
    const char*		name() const override { return typeid(*this).name(); }
    uiTreeItem*		create() const override
			{ return new uiODWellParentTreeItem(); }
    uiTreeItem*		createForVis(const VisID&,uiTreeItem*) const override;
};


mExpClass(uiODMain) uiODWellTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODWellTreeItem)
public:
			uiODWellTreeItem(const VisID&);
			uiODWellTreeItem(const MultiID&);

    MultiID		getMid() const		{ return mid_; }

protected:
			~uiODWellTreeItem();

    void		initMenuItems();
    bool		init() override;
    void		askSaveCB(CallBacker*);
    void		saveCB(CallBacker*);
    bool		askContinueAndSaveIfNeeded(bool withcancel) override;
    void		createMenu(MenuHandler*,bool istb) override;
    void		handleMenuCB(CallBacker*) override;
    bool		doubleClick(uiTreeViewItem*) override;
    const char*		parentType() const override
			{ return typeid(uiODWellParentTreeItem).name(); }

    MultiID		mid_;
    MenuItem		attrmnuitem_;
    MenuItem		logcubemnuitem_;
    MenuItem		editmnuitem_;
    MenuItem		storemnuitem_;
    MenuItem		sellogmnuitem_;
    MenuItem		propertiesmnuitem_;
    MenuItem		logviewermnuitem_;
    MenuItem		nametopmnuitem_;
    MenuItem		namebotmnuitem_;
    MenuItem		markermnuitem_;
    MenuItem		markernamemnuitem_;
    MenuItem		showlogmnuitem_;
    MenuItem		showmnuitem_;
    MenuItem		gend2tmmnuitem_;
    MenuItem		amplspectrummnuitem_;
    ObjectSet<MenuItem>	logmnuitems_;

    ConstRefMan<visSurvey::WellDisplay> getDisplay() const;
    RefMan<visSurvey::WellDisplay> getDisplay();

    WeakPtr<visSurvey::WellDisplay> welldisplay_;
};
