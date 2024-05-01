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
#include "pickset.h"


namespace Pick { class Set; class SetMgr; }
namespace View2D { class PickSet; }



mExpClass(uiODMain) uiODView2DPickSetParentTreeItem : public uiODView2DTreeItem
{ mODTextTranslationClass(uiODView2DPickSetParentTreeItem)
public:
				uiODView2DPickSetParentTreeItem();
				~uiODView2DPickSetParentTreeItem();

    bool			showSubMenu() override;
    void			getPickSetVwr2DIDs(const MultiID& mid,
						   TypeSet<Vis2DID>&) const;
    void			getLoadedPickSets(TypeSet<MultiID>&) const;
    void			removePickSet(const MultiID&);
    void			addPickSets(const TypeSet<MultiID>&);

protected:

    bool			init() override;
    const char*			iconName() const override;
    bool			handleSubMenu(int);
    const char*			parentType() const override
				{ return typeid(uiODView2DTreeTop).name(); }
    Pick::SetMgr&		picksetmgr_;

public:
    void			setupNewPickSet(const MultiID&);
};


mExpClass(uiODMain)
uiODView2DPickSetTreeItemFactory : public uiODView2DTreeItemFactory
{
public:
    const char*		name() const override	{ return typeid(*this).name(); }
    uiTreeItem*		create() const override
			{ return new uiODView2DPickSetParentTreeItem(); }
    uiTreeItem*		createForVis(const uiODViewer2D&,
				     const Vis2DID&) const override;
};


mExpClass(uiODMain) uiODView2DPickSetTreeItem : public uiODView2DTreeItem
{ mODTextTranslationClass(uiODView2DPickSetTreeItem)
public:
			uiODView2DPickSetTreeItem(int picksetid);
			uiODView2DPickSetTreeItem(const Vis2DID&,bool dummy);
			~uiODView2DPickSetTreeItem();

    bool			showSubMenu() override;
    bool			select() override;
    const MultiID&		pickMultiID() const;
    const View2D::PickSet*	vw2DObject() const	{ return vw2dpickset_; }
    void			keyPressedCB(CallBacker*);

protected:

    bool		init() override;
    const char*		parentType() const override
			{return typeid(uiODView2DPickSetParentTreeItem).name();}
    bool		isSelectable() const override		{ return true; }

    void		deSelCB(CallBacker*);
    void		checkCB(CallBacker*);
    void		displayChangedCB(CallBacker*);
    void		displayMiniCtab();
    void		removePickSetCB(CallBacker*);

    Pick::SetMgr&	picksetmgr_;
    RefMan<Pick::Set>	pickset_;
    View2D::PickSet*	vw2dpickset_		= nullptr;
};
