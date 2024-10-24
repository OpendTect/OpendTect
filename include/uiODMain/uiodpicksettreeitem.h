#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uioddisplaytreeitem.h"
#include "vispicksetdisplay.h"

namespace Pick		{ class Set; }
class uiSeedPainterDlg;


mExpClass(uiODMain) uiODPickSetParentTreeItem : public uiODParentTreeItem
{ mODTextTranslationClass(uiODPickSetParentTreeItem)
public:
			uiODPickSetParentTreeItem();

    static CNotifier<uiODPickSetParentTreeItem,uiMenu*>& showMenuNotifier();

protected:
			~uiODPickSetParentTreeItem();

    const char*		iconName() const override;
    bool		showSubMenu() override;
    void		addPickSet(Pick::Set*);
    void		setRemovedCB(CallBacker*);
};


mExpClass(uiODMain) uiODPickSetTreeItemFactory : public uiODTreeItemFactory
{ mODTextTranslationClass(uiODPickSetTreeItemFactory)
public:

    const char*		name() const override { return typeid(*this).name(); }
    uiTreeItem*		create() const override
			{ return new uiODPickSetParentTreeItem; }
    uiTreeItem*		createForVis(const VisID&,uiTreeItem*) const override;

};


mExpClass(uiODMain) uiODPickSetTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODPickSetTreeItem)
public:
			uiODPickSetTreeItem(const VisID&,Pick::Set&);

    bool		actModeWhenSelected() const override;
    void		showAllPicks(bool yn);
    RefMan<Pick::Set>		getSet()		{ return set_; }
    ConstRefMan<Pick::Set>	getSet() const		{ return set_; }

    ConstRefMan<visSurvey::PickSetDisplay> getDisplay() const;

protected:
			~uiODPickSetTreeItem();

    bool		init() override;
    bool		askContinueAndSaveIfNeeded(bool withcancel) override;
    void		setChg(CallBacker*);
    void		createMenu(MenuHandler*,bool istb) override;
    void		handleMenuCB(CallBacker*) override;
    bool		doubleClick(uiTreeViewItem*) override;
    const char*		parentType() const override
			{ return typeid(uiODPickSetParentTreeItem).name(); }

    void		selChangedCB(CallBacker*);
    void		paintDlgClosedCB(CallBacker*);
    void		enablePainting(bool);
    void		askSaveCB(CallBacker*);
    void		saveCB(CallBacker*);

    RefMan<Pick::Set>	set_;
    MenuItem		storemnuitem_;
    MenuItem		storeasmnuitem_;
    MenuItem		dirmnuitem_;
    MenuItem		onlyatsectmnuitem_;
    MenuItem		convertbodymnuitem_;
    MenuItem		propertymnuitem_;
    MenuItem		paintingmnuitem_;

    uiSeedPainterDlg*	paintdlg_ = nullptr;
    bool		paintingenabled_ = false;

    RefMan<visSurvey::PickSetDisplay> getDisplay();

    WeakPtr<visSurvey::PickSetDisplay> pointsetdisplay_;

};


mExpClass(uiODMain) uiODPolygonParentTreeItem : public uiODParentTreeItem
{ mODTextTranslationClass(uiODPolygonParentTreeItem)
public:
			uiODPolygonParentTreeItem();

    static CNotifier<uiODPolygonParentTreeItem,uiMenu*>& showMenuNotifier();

protected:
			~uiODPolygonParentTreeItem();

    const char*		iconName() const override;
    bool		showSubMenu() override;
    void		addPolygon(Pick::Set*);
    void		setRemovedCB(CallBacker*);

};


mExpClass(uiODMain) uiODPolygonTreeItemFactory : public uiODTreeItemFactory
{ mODTextTranslationClass(uiODPolygonTreeItemFactory)
public:

    const char*		name() const override { return typeid(*this).name(); }
    uiTreeItem*		create() const override
			{ return new uiODPolygonParentTreeItem; }
    uiTreeItem*		createForVis(const VisID&,uiTreeItem*) const override;

};


mExpClass(uiODMain) uiODPolygonTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODPolygonTreeItem)
public:
			uiODPolygonTreeItem(const VisID&,Pick::Set&);

    bool		actModeWhenSelected() const override;
    void		showAllPicks(bool yn);
    RefMan<Pick::Set>	getSet()			{ return set_; }
    ConstRefMan<Pick::Set> getSet() const		{ return set_; }

    ConstRefMan<visSurvey::PickSetDisplay> getDisplay() const;

protected:
			~uiODPolygonTreeItem();

    bool		init() override;
    bool		askContinueAndSaveIfNeeded(bool withcancel) override;
    void		setChg(CallBacker*);
    void		createMenu(MenuHandler*,bool istb) override;
    void		handleMenuCB(CallBacker*) override;
    bool		doubleClick(uiTreeViewItem*) override;
    const char*		parentType() const override
			{ return typeid(uiODPolygonParentTreeItem).name(); }
    void		selChangedCB(CallBacker*);
    void		askSaveCB(CallBacker*);
    void		saveCB(CallBacker*);

    RefMan<Pick::Set>	set_;

    MenuItem		storemnuitem_;
    MenuItem		storeasmnuitem_;
    MenuItem		onlyatsectmnuitem_;
    MenuItem		propertymnuitem_;
    MenuItem		closepolyitem_;
    MenuItem		changezmnuitem_;
    MenuItem		workareaitem_;
    MenuItem		calcvolmnuitem_;

    RefMan<visSurvey::PickSetDisplay> getDisplay();

    WeakPtr<visSurvey::PickSetDisplay> polygondisplay_;
};
