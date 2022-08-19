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

namespace View2D { class Horizon3D; }
class uiODViewer2D;


mExpClass(uiODMain) uiODView2DHor3DParentTreeItem : public uiODView2DTreeItem
{ mODTextTranslationClass(uiODView2DHor3DParentTreeItem);
public:
				uiODView2DHor3DParentTreeItem();
				~uiODView2DHor3DParentTreeItem();

    bool			showSubMenu();
    void			getHor3DVwr2DIDs(EM::ObjectID emid,
					TypeSet<Vis2DID>&) const;
    void			getLoadedHorizon3Ds(
					TypeSet<EM::ObjectID>&) const;
    void			removeHorizon3D(EM::ObjectID emid);
    void			addHorizon3Ds(const TypeSet<EM::ObjectID>&);
    void			addNewTrackingHorizon3D(EM::ObjectID emid);

protected:

    bool			init();
    const char*			iconName() const;
    bool			handleSubMenu(int);
    const char*			parentType() const
				{ return typeid(uiODView2DTreeTop).name(); }
    void			getNonLoadedTrackedHor3Ds(
					TypeSet<EM::ObjectID>&);
public:
    void			setupTrackingHorizon3D(EM::ObjectID emid);
};


mExpClass(uiODMain) uiODView2DHor3DTreeItemFactory
    : public uiODView2DTreeItemFactory
{
public:
    const char*		name() const		{ return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODView2DHor3DParentTreeItem(); }
    uiTreeItem*		createForVis(const uiODViewer2D&,Vis2DID) const;
};


mExpClass(uiODMain) uiODView2DHor3DTreeItem : public uiODView2DTreeItem
{ mODTextTranslationClass(uiODView2DHor3DTreeItem)
public:
			uiODView2DHor3DTreeItem(const EM::ObjectID&);
			uiODView2DHor3DTreeItem(Vis2DID id,bool dummy);
			~uiODView2DHor3DTreeItem();

    bool			select();
    bool			showSubMenu();
    EM::ObjectID		emObjectID() const	{ return emid_; }
    const View2D::Horizon3D*	vw2DObject() const	{ return horview_; }

protected:

    bool		init();
    const char*		parentType() const
			{ return typeid(uiODView2DHor3DParentTreeItem).name(); }
    bool		isSelectable() const			{ return true; }


    void		updateSelSpec(const Attrib::SelSpec*,bool wva);
    void		updateCS(const TrcKeyZSampling&,bool upd);
    void		checkCB(CallBacker*);
    void		deSelCB(CallBacker*);
    void		mousePressInVwrCB(CallBacker*);
    void		mouseReleaseInVwrCB(CallBacker*);
    void		displayMiniCtab();

    void		emobjChangeCB(CallBacker*);
    void		propChgCB(CallBacker*);

    EM::ObjectID	emid_;
    View2D::Horizon3D*	horview_;
    bool		oldactivevolupdated_;
    bool		trackerefed_;
    void		emobjAbtToDelCB(CallBacker*);
};
