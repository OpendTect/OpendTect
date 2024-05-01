#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"

#include "emposid.h"
#include "uioddisplaytreeitem.h"
#include "uiodattribtreeitem.h"
#include "visfaultsetdisplay.h"

class DataPointSet;

namespace visSurvey { class FaultSetDisplay; }


mExpClass(uiODMain) uiODFaultSetParentTreeItem : public uiODParentTreeItem
{ mODTextTranslationClass(uiODFaultSetParentTreeItem)
public:
			uiODFaultSetParentTreeItem();

protected:
			~uiODFaultSetParentTreeItem();

    const char*		iconName() const override;
    bool		showSubMenu() override;
};


mExpClass(uiODMain) uiODFaultSetTreeItemFactory : public uiODTreeItemFactory
{ mODTextTranslationClass(uiODFaultSetTreeItemFactory)
public:
    const char*		name() const override { return typeid(*this).name(); }
    uiTreeItem*		create() const override
			{ return new uiODFaultSetParentTreeItem; }
    uiTreeItem*		createForVis(const VisID&,uiTreeItem*) const override;
};


mExpClass(uiODMain) uiODFaultSetTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODFaultSetTreeItem)
public:
			uiODFaultSetTreeItem(const VisID&,bool dummy);
			uiODFaultSetTreeItem(const EM::ObjectID&);

    EM::ObjectID	emObjectID() const	{ return emid_; }

    void		setOnlyAtSectionsDisplay(bool) override;
    bool		isOnlyAtSections() const override;

    static uiString	sFaultPlanes() { return tr("Fault Planes" ); }
    static uiString	sOnlyAtSections() { return tr( "Only at Sections" ); }
    static uiString	sOnlyAtHorizons() { return tr( "Only at Horizons" ); }

protected:
			~uiODFaultSetTreeItem();

    bool		askContinueAndSaveIfNeeded(bool withcancel) override;
    void		createMenu(MenuHandler*,bool istb) override;
    void		handleMenuCB(CallBacker*) override;
    void		colorChCB(CallBacker*);

    uiODDataTreeItem*	createAttribItem(const Attrib::SelSpec*) const override;

    bool		init() override;
    const char*		parentType() const override
			{ return typeid(uiODFaultSetParentTreeItem).name(); }

    EM::ObjectID		emid_;

    MenuItem			savemnuitem_;
    MenuItem			saveasmnuitem_;
    MenuItem			displayplanemnuitem_;
    MenuItem			displayintersectionmnuitem_;
    MenuItem			displayintersecthorizonmnuitem_;
    MenuItem			singlecolmnuitem_;

    ConstRefMan<visSurvey::FaultSetDisplay> getDisplay() const;
    RefMan<visSurvey::FaultSetDisplay> getDisplay();

    WeakPtr<visSurvey::FaultSetDisplay> faultsetdisplay_;

private:
			uiODFaultSetTreeItem();
};


mExpClass(uiODMain) uiODFaultSetDataTreeItem : public uiODAttribTreeItem
{ mODTextTranslationClass(uiODFaultSetDataTreeItem)
public:
			uiODFaultSetDataTreeItem(EM::ObjectID,
				const char* parenttype);

    void		setDataPointSet(const DataPointSet&);

protected:
			~uiODFaultSetDataTreeItem();

    void		createMenu(MenuHandler*,bool istb) override;
    void		handleMenuCB(CallBacker*) override;
    uiString		createDisplayName() const override;

    MenuItem		depthattribmnuitem_;

    bool		changed_ = false;
    EM::ObjectID	emid_;
};
