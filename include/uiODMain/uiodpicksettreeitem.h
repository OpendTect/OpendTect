#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uioddisplaytreeitem.h"

namespace Pick		{ class Set; }
class uiSeedPainterDlg;


mExpClass(uiODMain) uiODPickSetParentTreeItem : public uiODParentTreeItem
{ mODTextTranslationClass(uiODPickSetParentTreeItem)
public:
			uiODPickSetParentTreeItem();
			~uiODPickSetParentTreeItem();

    static CNotifier<uiODPickSetParentTreeItem,uiMenu*>& showMenuNotifier();

protected:
    const char*		iconName() const override;
    bool		showSubMenu() override;
    void		addPickSet(Pick::Set*);
    void		setRemovedCB(CallBacker*);
};


mExpClass(uiODMain) uiODPickSetTreeItemFactory : public uiODTreeItemFactory
{ mODTextTranslationClass(uiODPickSetTreeItemFactory)
public:

    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const { return new uiODPickSetParentTreeItem; }
    uiTreeItem*		createForVis(VisID visid,uiTreeItem*) const;

};


mExpClass(uiODMain) uiODPickSetTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODPickSetTreeItem)
public:
			uiODPickSetTreeItem(VisID dispid,Pick::Set&);
    virtual		~uiODPickSetTreeItem();

    virtual bool	actModeWhenSelected() const;
    void		showAllPicks(bool yn);
    RefMan<Pick::Set>		getSet()		{ return set_; }
    ConstRefMan<Pick::Set>	getSet() const		{ return set_; }

protected:

    bool		init();
    void		prepareForShutdown();
    bool		askContinueAndSaveIfNeeded(bool withcancel);
    void		setChg(CallBacker*);
    virtual void	createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    bool		doubleClick(uiTreeViewItem*);
    virtual const char*	parentType() const
    			{ return typeid(uiODPickSetParentTreeItem).name(); }

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

    void		selChangedCB(CallBacker*);
    void		paintDlgClosedCB(CallBacker*);
    void		enablePainting(bool);
    void		askSaveCB(CallBacker*);
    void		saveCB(CallBacker*);
};


mExpClass(uiODMain) uiODPolygonParentTreeItem : public uiODParentTreeItem
{ mODTextTranslationClass(uiODPolygonParentTreeItem)
public:
			uiODPolygonParentTreeItem();
			~uiODPolygonParentTreeItem();

    static CNotifier<uiODPolygonParentTreeItem,uiMenu*>& showMenuNotifier();

protected:
    const char*		iconName() const override;
    bool		showSubMenu() override;
    void		addPolygon(Pick::Set*);
    void		setRemovedCB(CallBacker*);

};


mExpClass(uiODMain) uiODPolygonTreeItemFactory : public uiODTreeItemFactory
{ mODTextTranslationClass(uiODPolygonTreeItemFactory)
public:

    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const { return new uiODPolygonParentTreeItem; }
    uiTreeItem*		createForVis(VisID visid,uiTreeItem*) const;

};


mExpClass(uiODMain) uiODPolygonTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODPolygonTreeItem)
public:
			uiODPolygonTreeItem(VisID dispid,Pick::Set&);
    virtual		~uiODPolygonTreeItem();

    virtual bool	actModeWhenSelected() const;
    void		showAllPicks(bool yn);
    RefMan<Pick::Set>	getSet()			{ return set_; }
    ConstRefMan<Pick::Set> getSet() const		{ return set_; }

protected:

    bool		init();
    void		prepareForShutdown();
    bool		askContinueAndSaveIfNeeded(bool withcancel);
    void		setChg(CallBacker*);
    virtual void	createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    bool		doubleClick(uiTreeViewItem*);
    virtual const char*	parentType() const
			{ return typeid(uiODPolygonParentTreeItem).name(); }

    RefMan<Pick::Set>	set_;

    MenuItem		storemnuitem_;
    MenuItem		storeasmnuitem_;
    MenuItem		onlyatsectmnuitem_;
    MenuItem		propertymnuitem_;
    MenuItem		closepolyitem_;
    MenuItem		changezmnuitem_;
    MenuItem		workareaitem_;
    MenuItem		calcvolmnuitem_;

    void		selChangedCB(CallBacker*);
    void		askSaveCB(CallBacker*);
    void		saveCB(CallBacker*);
};
