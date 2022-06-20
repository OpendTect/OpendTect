#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
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

    virtual void	removeChild(uiTreeItem*);

    static CNotifier<uiODHorizonParentTreeItem,uiMenu*>& showMenuNotifier();

    CNotifier<uiODHorizonParentTreeItem,int>	handleMenu;
    MenuItem		newmenu_;

protected:
    const char*		iconName() const;
    bool		showSubMenu();
    virtual bool	addChld(uiTreeItem*,bool,bool);

    void		sort();
    MenuItem		trackitem_;
    MenuItem		constzitem_;
};



mExpClass(uiODMain) uiODHorizonTreeItemFactory : public uiODTreeItemFactory
{ mODTextTranslationClass(uiODHorizonTreeItemFactory)
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODHorizonParentTreeItem; }
    uiTreeItem*		createForVis(int visid,uiTreeItem*) const;
};


mExpClass(uiODMain) uiODHorizonTreeItem : public uiODEarthModelSurfaceTreeItem
{ mODTextTranslationClass(uiODHorizonTreeItem)
public:
			uiODHorizonTreeItem(int visid,bool rgba,
					    bool atsect,bool dummy);
			uiODHorizonTreeItem(const EM::ObjectID&,
					    bool rgba,bool atsect);
			~uiODHorizonTreeItem();

    virtual int		reloadEMObject();	// Return new display id.

protected:
    bool		init();
    void		initMenuItems();
    void		initNotify();
    uiString		createDisplayName() const;
    void		dispChangeCB(CallBacker*);
    void		dataReadyCB(CallBacker*);
    const char*		parentType() const
			{ return typeid(uiODHorizonParentTreeItem).name(); }

    virtual void	createMenu(MenuHandler*,bool istb);
    virtual void	handleMenuCB(CallBacker*);

    bool		askContinueAndSaveIfNeeded(bool withcancel);

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
    virtual void	removeChild(uiTreeItem*) override;
    bool		addChld(uiTreeItem* child,bool below,
				bool downwards) override;
};


mExpClass(uiODMain) uiODHorizon2DTreeItemFactory : public uiODTreeItemFactory
{ mODTextTranslationClass(uiODHorizon2DTreeItemFactory)
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODHorizon2DParentTreeItem; }
    uiTreeItem*		createForVis(int visid,uiTreeItem*) const;
};


mExpClass(uiODMain) uiODHorizon2DTreeItem : public uiODEarthModelSurfaceTreeItem
{ mODTextTranslationClass(uiODHorizon2DTreeItem)
public:
			uiODHorizon2DTreeItem(int visid,bool dummy);
			uiODHorizon2DTreeItem(const EM::ObjectID&);
			~uiODHorizon2DTreeItem();

protected:
    void		initMenuItems();
    void		initNotify();
    uiString		createDisplayName() const;
    void		dispChangeCB(CallBacker*);
    const char*		parentType() const
			{ return typeid(uiODHorizon2DParentTreeItem).name(); }

    virtual void	createMenu(MenuHandler*,bool istb);
    virtual void	handleMenuCB(CallBacker*);

    bool		askContinueAndSaveIfNeeded(bool withcancel);

    MenuItem		shiftmnuitem_;
    MenuItem		algomnuitem_;
    MenuItem		workflowsmnuitem_;
    MenuItem		derive3dhormnuitem_;
    MenuItem		snapeventmnuitem_;
    MenuItem		interpolatemnuitem_;
};

