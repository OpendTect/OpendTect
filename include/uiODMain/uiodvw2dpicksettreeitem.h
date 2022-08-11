#pragma once

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Ranojay Sen
 Date:		Mar 2011
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

    bool			showSubMenu();
    void			getPickSetVwr2DIDs(const MultiID& mid,
						   TypeSet<Vis2DID>&) const;
    void			getLoadedPickSets(TypeSet<MultiID>&) const;
    void			removePickSet(const MultiID&);
    void			addPickSets(const TypeSet<MultiID>&);

protected:

    bool			init();
    const char*			iconName() const;
    bool			handleSubMenu(int);
    const char*			parentType() const
				{ return typeid(uiODView2DTreeTop).name(); }
    Pick::SetMgr&		picksetmgr_;

public:
    void			setupNewPickSet(const MultiID&);
};


mExpClass(uiODMain)
uiODView2DPickSetTreeItemFactory : public uiODView2DTreeItemFactory
{
public:
    const char*		name() const		{ return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODView2DPickSetParentTreeItem(); }
    uiTreeItem*		createForVis(const uiODViewer2D&,Vis2DID) const;
};


mExpClass(uiODMain) uiODView2DPickSetTreeItem : public uiODView2DTreeItem
{ mODTextTranslationClass(uiODView2DPickSetTreeItem)
public:
			uiODView2DPickSetTreeItem(int picksetid);
			uiODView2DPickSetTreeItem(Vis2DID id,bool dummy);
			~uiODView2DPickSetTreeItem();

    bool			showSubMenu();
    bool			select();
    const MultiID&		pickMultiID() const;
    const View2D::PickSet*	vw2DObject() const	{ return vw2dpickset_; }
    void			keyPressedCB(CallBacker*);

protected:

    bool		init();
    const char*		parentType() const
			{return typeid(uiODView2DPickSetParentTreeItem).name();}
    bool		isSelectable() const			{ return true; }

    void		deSelCB(CallBacker*);
    void		checkCB(CallBacker*);
    void		displayChangedCB(CallBacker*);
    void		displayMiniCtab();
    void		removePickSetCB(CallBacker*);

    Pick::SetMgr&	picksetmgr_;
    RefMan<Pick::Set>	pickset_;
    View2D::PickSet*	vw2dpickset_		= nullptr;
};
