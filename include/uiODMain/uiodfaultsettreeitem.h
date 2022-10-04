#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"

#include "uioddisplaytreeitem.h"
#include "uiodattribtreeitem.h"

#include "emposid.h"

class DataPointSet;

namespace visSurvey { class FaultSetDisplay; }


mExpClass(uiODMain) uiODFaultSetParentTreeItem : public uiODParentTreeItem
{ mODTextTranslationClass(uiODFaultSetParentTreeItem)
public:
			uiODFaultSetParentTreeItem();
			~uiODFaultSetParentTreeItem();

protected:
    const char*		iconName() const override;
    bool		showSubMenu() override;
};


mExpClass(uiODMain) uiODFaultSetTreeItemFactory : public uiODTreeItemFactory
{ mODTextTranslationClass(uiODFaultSetTreeItemFactory)
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODFaultSetParentTreeItem; }
    uiTreeItem*		createForVis(VisID visid,uiTreeItem*) const;
};


mExpClass(uiODMain) uiODFaultSetTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODFaultSetTreeItem)
public:
			uiODFaultSetTreeItem(VisID,bool dummy);
			uiODFaultSetTreeItem(const EM::ObjectID&);
			~uiODFaultSetTreeItem();

    EM::ObjectID	emObjectID() const	{ return emid_; }

    void		setOnlyAtSectionsDisplay(bool);
    bool		isOnlyAtSections() const;

    static uiString	sFaultPlanes() { return tr("Fault Planes" ); }
    static uiString	sOnlyAtSections() { return tr( "Only at Sections" ); }
    static uiString	sOnlyAtHorizons() { return tr( "Only at Horizons" ); }

protected:
    bool		askContinueAndSaveIfNeeded(bool withcancel);
    void		prepareForShutdown();
    virtual void	createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    void		colorChCB(CallBacker*);

    uiODDataTreeItem*	createAttribItem(const Attrib::SelSpec*) const;

    bool		init();
    const char*		parentType() const
			{ return typeid(uiODFaultSetParentTreeItem).name(); }

    EM::ObjectID		emid_;

    MenuItem			savemnuitem_;
    MenuItem			saveasmnuitem_;
    MenuItem			displayplanemnuitem_;
    MenuItem			displayintersectionmnuitem_;
    MenuItem			displayintersecthorizonmnuitem_;
    MenuItem			singlecolmnuitem_;
    visSurvey::FaultSetDisplay*	faultsetdisplay_	= nullptr;

private:
			uiODFaultSetTreeItem();
};


mExpClass(uiODMain) uiODFaultSetDataTreeItem : public uiODAttribTreeItem
{ mODTextTranslationClass(uiODFaultSetDataTreeItem)
public:
			uiODFaultSetDataTreeItem(EM::ObjectID,
				const char* parenttype);
			~uiODFaultSetDataTreeItem();

    void		setDataPointSet(const DataPointSet&);

protected:

    void		createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    uiString		createDisplayName() const;

    MenuItem		depthattribmnuitem_;

    bool		changed_;
    EM::ObjectID	emid_;
};
