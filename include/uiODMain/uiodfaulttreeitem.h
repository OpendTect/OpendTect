#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"

#include "emposid.h"
#include "uiodattribtreeitem.h"
#include "uioddisplaytreeitem.h"
#include "visfaultdisplay.h"
#include "visfaultsticksetdisplay.h"

class DataPointSet;


namespace visSurvey { class FaultDisplay; class FaultStickSetDisplay; }


mExpClass(uiODMain) uiODFaultParentTreeItem : public uiODParentTreeItem
{ mODTextTranslationClass(uiODFaultParentTreeItem)
public:
			uiODFaultParentTreeItem();

    static CNotifier<uiODFaultParentTreeItem,uiMenu*>& showMenuNotifier();

protected:
			~uiODFaultParentTreeItem();

    const char*		iconName() const override;
    bool		showSubMenu() override;
};


mExpClass(uiODMain) uiODFaultTreeItemFactory : public uiODTreeItemFactory
{ mODTextTranslationClass(uiODFaultTreeItemFactory)
public:
    const char*		name() const override { return typeid(*this).name(); }
    uiTreeItem*		create() const override
			{ return new uiODFaultParentTreeItem; }
    uiTreeItem*		createForVis(const VisID&,uiTreeItem*) const override;
};


mExpClass(uiODMain) uiODFaultTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODFaultTreeItem)
public:
			uiODFaultTreeItem(const VisID&,bool dummy);
			uiODFaultTreeItem(const EM::ObjectID&);

    EM::ObjectID	emObjectID() const	{ return emid_; }

    void		setOnlyAtSectionsDisplay(bool) override;
    bool		isOnlyAtSections() const override;

    static uiString	sFaultPlanes() { return tr("Fault Planes" ); }
    static uiString	sFaultSticks() { return tr("Fault Sticks" ); }
    static uiString	sOnlyAtSections() { return tr( "Only at Sections" ); }
    static uiString	sOnlyAtHorizons() { return tr( "Only at Horizons" ); }

protected:
			~uiODFaultTreeItem();

    bool		askContinueAndSaveIfNeeded(bool withcancel) override;
    void		createMenu(MenuHandler*,bool istb) override;
    void		handleMenuCB(CallBacker*) override;
    void		askSaveCB(CallBacker*);
    void		saveCB(CallBacker*);

    uiODDataTreeItem*	createAttribItem(const Attrib::SelSpec*) const override;

    bool		init() override;
    const char*		parentType() const override
			{return typeid(uiODFaultParentTreeItem).name();}

    EM::ObjectID		emid_;

    MenuItem			savemnuitem_;
    MenuItem			saveasmnuitem_;
    MenuItem			displayplanemnuitem_;
    MenuItem			displaystickmnuitem_;
    MenuItem			displayintersectionmnuitem_;
    MenuItem			displayintersecthorizonmnuitem_;
    MenuItem			singlecolmnuitem_;

    ConstRefMan<visSurvey::FaultDisplay> getDisplay() const;
    RefMan<visSurvey::FaultDisplay> getDisplay();

    WeakPtr<visSurvey::FaultDisplay>	faultdisplay_;
};


mExpClass(uiODMain) uiODFaultStickSetParentTreeItem : public uiODParentTreeItem
{ mODTextTranslationClass(uiODFaultStickSetParentTreeItem)
public:
			uiODFaultStickSetParentTreeItem();
			~uiODFaultStickSetParentTreeItem();

    static CNotifier<uiODFaultStickSetParentTreeItem,uiMenu*>&
			showMenuNotifier();

protected:
    const char*		iconName() const override;
    bool		showSubMenu() override;
};


mExpClass(uiODMain) uiODFaultStickSetTreeItemFactory
    : public uiODTreeItemFactory
{ mODTextTranslationClass(uiODFaultStickSetTreeItemFactory)
public:
    const char*		name() const override { return typeid(*this).name(); }
    uiTreeItem*		create() const override
			{ return new uiODFaultStickSetParentTreeItem; }
    uiTreeItem*		createForVis(const VisID&,uiTreeItem*) const override;
};


mExpClass(uiODMain) uiODFaultStickSetTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODFaultStickSetTreeItem)
public:
			uiODFaultStickSetTreeItem(const VisID&,bool dummy);
			uiODFaultStickSetTreeItem(const EM::ObjectID&);
			~uiODFaultStickSetTreeItem();

    EM::ObjectID	emObjectID() const	{ return emid_; }

protected:
    bool		askContinueAndSaveIfNeeded( bool withcancel ) override;
    void		createMenu(MenuHandler*,bool istb) override;
    void		handleMenuCB(CallBacker*) override;
    void		colorChCB(CallBacker*);
    void		askSaveCB(CallBacker*);
    void		saveCB(CallBacker*);

    bool		init() override;
    const char*		parentType() const override
			{return typeid(uiODFaultStickSetParentTreeItem).name();}


    EM::ObjectID			emid_;
    MenuItem				onlyatsectmnuitem_;
    MenuItem				savemnuitem_;
    MenuItem				saveasmnuitem_;

    ConstRefMan<visSurvey::FaultStickSetDisplay> getDisplay() const;
    RefMan<visSurvey::FaultStickSetDisplay> getDisplay();

    WeakPtr<visSurvey::FaultStickSetDisplay>	faultsticksetdisplay_;
};


mExpClass(uiODMain) uiODFaultSurfaceDataTreeItem : public uiODAttribTreeItem
{ mODTextTranslationClass(uiODFaultSurfaceDataTreeItem)
public:
			uiODFaultSurfaceDataTreeItem(EM::ObjectID,
				const char* parenttype);
			~uiODFaultSurfaceDataTreeItem();

    void		setDataPointSet(const DataPointSet&);

protected:

    void		createMenu(MenuHandler*,bool istb) override;
    void		handleMenuCB(CallBacker*) override;
    uiString		createDisplayName() const override;

    MenuItem		depthattribmnuitem_;
    MenuItem		savesurfacedatamnuitem_;
    MenuItem		loadsurfacedatamnuitem_;
    MenuItem		algomnuitem_;

    bool		changed_;
    EM::ObjectID	emid_;
};
