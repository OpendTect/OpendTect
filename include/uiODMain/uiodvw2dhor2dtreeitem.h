#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uiodvw2dtreeitem.h"
#include "emposid.h"

namespace View2D { class Horizon2D; }

mExpClass(uiODMain) uiODView2DHor2DParentTreeItem : public uiODView2DTreeItem
{ mODTextTranslationClass(uiODView2DHor2DParentTreeItem);
public:
				uiODView2DHor2DParentTreeItem();
				~uiODView2DHor2DParentTreeItem();

    bool			showSubMenu() override;
    void			getHor2DVwr2DIDs(EM::ObjectID,
						 TypeSet<Vis2DID>&) const;
    void			getLoadedHorizon2Ds(
					TypeSet<EM::ObjectID>&) const;
    void			removeHorizon2D(EM::ObjectID);
    void			addHorizon2Ds(const TypeSet<EM::ObjectID>&);
    void			addNewTrackingHorizon2D(EM::ObjectID);
    void			setupTrackingHorizon2D(EM::ObjectID);

protected:

    bool			init() override;
    const char*			iconName() const override;
    bool			handleSubMenu(int);
    const char*			parentType() const override
				{ return typeid(uiODView2DTreeTop).name(); }
    void			getNonLoadedTrackedHor2Ds(
					TypeSet<EM::ObjectID>&);
};


mExpClass(uiODMain)
uiODView2DHor2DTreeItemFactory : public uiODView2DTreeItemFactory
{
public:
    const char*		name() const override	{ return typeid(*this).name(); }
    uiTreeItem*		create() const override
			{ return new uiODView2DHor2DParentTreeItem(); }
    uiTreeItem*		createForVis(const uiODViewer2D&,
				     const Vis2DID&) const override;
};


mExpClass(uiODMain) uiODView2DHor2DTreeItem : public uiODView2DTreeItem
{ mODTextTranslationClass(uiODView2DHor2DTreeItem)
public:
			uiODView2DHor2DTreeItem(const EM::ObjectID&);
			uiODView2DHor2DTreeItem(const Vis2DID&,bool dummy);
			~uiODView2DHor2DTreeItem();

    bool			showSubMenu() override;
    bool			select() override;
    EM::ObjectID		emObjectID() const	{ return emid_; }
    const View2D::Horizon2D*	vw2DObject() const	{ return horview_; }

protected:

    bool		init() override;
    const char*		parentType() const override
			{ return typeid(uiODView2DHor2DParentTreeItem).name(); }
    bool		isSelectable() const override		{ return true; }

    void		updateSelSpec(const Attrib::SelSpec*,bool wva) override;
    void		deSelCB(CallBacker*);
    void		checkCB(CallBacker*);
    void		emobjAbtToDelCB(CallBacker*);
    void		mousePressInVwrCB(CallBacker*);
    void		mouseReleaseInVwrCB(CallBacker*);
    void		displayMiniCtab();

    void		emobjChangeCB(CallBacker*);
    void		propChgCB(CallBacker*);

    EM::ObjectID	emid_;
    View2D::Horizon2D*	horview_;
    bool		trackerefed_;
};
