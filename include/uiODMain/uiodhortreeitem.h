#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uiodemsurftreeitem.h"

class uiEMDataPointSetPickDlg;

mExpClass(uiODMain) uiODHorizonParentTreeItem : public uiODParentTreeItem
{ mODTextTranslationClass(uiODHorizonParentTreeItem)
public:
			uiODHorizonParentTreeItem();
			~uiODHorizonParentTreeItem();

    void		removeChild(uiTreeItem*) override;

    static CNotifier<uiODHorizonParentTreeItem,uiMenu*>& showMenuNotifier();

    CNotifier<uiODHorizonParentTreeItem,int>	handleMenu;
    MenuItem		newmenu_;

protected:
    const char*		iconName() const override;
    bool		showSubMenu() override;
    bool		addChld(uiTreeItem*,bool,bool) override;

    void		sort();
    MenuItem		trackitem_;
    MenuItem		constzitem_;
};



mExpClass(uiODMain) uiODHorizonTreeItemFactory : public uiODTreeItemFactory
{ mODTextTranslationClass(uiODHorizonTreeItemFactory)
public:
    const char*		name() const override { return typeid(*this).name(); }
    uiTreeItem*		create() const override
			{ return new uiODHorizonParentTreeItem; }
    uiTreeItem*		createForVis(VisID visid,uiTreeItem*) const override;
};


mExpClass(uiODMain) uiODHorizonTreeItem : public uiODEarthModelSurfaceTreeItem
{ mODTextTranslationClass(uiODHorizonTreeItem)
public:
			uiODHorizonTreeItem(VisID visid,bool rgba,
					    bool atsect,bool dummy);
			uiODHorizonTreeItem(const EM::ObjectID&,
					    bool rgba,bool atsect);
			~uiODHorizonTreeItem();

    VisID		reloadEMObject() override; // Return new display id.

protected:
    bool		init() override;
    void		initMenuItems();
    void		initNotify() override;
    uiString		createDisplayName() const override;
    void		dispChangeCB(CallBacker*);
    void		dataReadyCB(CallBacker*);
    const char*		parentType() const override
			{ return typeid(uiODHorizonParentTreeItem).name(); }

    void		createMenu(MenuHandler*,bool istb) override;
    void		handleMenuCB(CallBacker*) override;

    bool		askContinueAndSaveIfNeeded(bool withcancel) override;

    MenuItem		hordatamnuitem_;
    MenuItem		algomnuitem_;
    MenuItem		workflowsmnuitem_;
    MenuItem		fillholesmnuitem_;
    MenuItem		filterhormnuitem_;
    MenuItem		geom2attrmnuitem_;
    MenuItem		positionmnuitem_;
    MenuItem		snapeventmnuitem_;
    MenuItem		shiftmnuitem_;
    MenuItem		flatcubemnuitem_;
    MenuItem		isochronmnuitem_;
    MenuItem		calcvolmnuitem_;
    MenuItem		pickdatamnuitem_;

    MenuItem		parentsmnuitem_;
    MenuItem		parentsrdlmnuitem_;
    MenuItem		childrenmnuitem_;
    MenuItem		delchildrenmnuitem_;
    MenuItem		lockmnuitem_;
    MenuItem		unlockmnuitem_;
    MenuItem		addinlitm_;
    MenuItem		addcrlitm_;
    MenuItem		addzitm_;
    MenuItem		addcontouritm_;

    bool		rgba_;
    bool		atsections_;

    uiEMDataPointSetPickDlg*	dpspickdlg_		= nullptr;
};


mExpClass(uiODMain) uiODHorizon2DParentTreeItem : public uiODParentTreeItem
{ mODTextTranslationClass(uiODHorizon2DParentTreeItem)
public:
			uiODHorizon2DParentTreeItem();
			~uiODHorizon2DParentTreeItem();

    static CNotifier<uiODHorizon2DParentTreeItem,uiMenu*>& showMenuNotifier();

protected:

    const char*		iconName() const override;
    bool		showSubMenu() override;
    void		sort();
    void		removeChild(uiTreeItem*) override;
    bool		addChld(uiTreeItem* child,bool below,
				bool downwards) override;
};


mExpClass(uiODMain) uiODHorizon2DTreeItemFactory : public uiODTreeItemFactory
{ mODTextTranslationClass(uiODHorizon2DTreeItemFactory)
public:
    const char*		name() const override { return typeid(*this).name(); }
    uiTreeItem*		create() const override
			{ return new uiODHorizon2DParentTreeItem; }
    uiTreeItem*		createForVis(VisID visid,uiTreeItem*) const override;
};


mExpClass(uiODMain) uiODHorizon2DTreeItem : public uiODEarthModelSurfaceTreeItem
{ mODTextTranslationClass(uiODHorizon2DTreeItem)
public:
			uiODHorizon2DTreeItem(VisID visid,bool dummy);
			uiODHorizon2DTreeItem(const EM::ObjectID&);
			~uiODHorizon2DTreeItem();

protected:
    void		initMenuItems();
    void		initNotify() override;
    uiString		createDisplayName() const override;
    void		dispChangeCB(CallBacker*);
    const char*		parentType() const override
			{ return typeid(uiODHorizon2DParentTreeItem).name(); }

    void		createMenu(MenuHandler*,bool istb) override;
    void		handleMenuCB(CallBacker*) override;

    bool		askContinueAndSaveIfNeeded(bool withcancel) override;

    MenuItem		shiftmnuitem_;
    MenuItem		algomnuitem_;
    MenuItem		workflowsmnuitem_;
    MenuItem		derive3dhormnuitem_;
    MenuItem		snapeventmnuitem_;
    MenuItem		interpolatemnuitem_;
};
