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

class VW2DPickSet;

namespace Pick { class Set; class SetMgr; }



mExpClass(uiODMain) uiODVw2DPickSetParentTreeItem : public uiODVw2DTreeItem
{ mODTextTranslationClass(uiODVw2DPickSetParentTreeItem)
public:
				uiODVw2DPickSetParentTreeItem();
				~uiODVw2DPickSetParentTreeItem();

    bool			showSubMenu();
    void			getPickSetVwr2DIDs(const MultiID& mid,
						   TypeSet<int>& vw2ids) const;
    void			getLoadedPickSets(TypeSet<MultiID>&) const;
    void			removePickSet(const MultiID&);
    void			addPickSets(const TypeSet<MultiID>&);

protected:

    bool			init();
    const char*			iconName() const;
    bool			handleSubMenu(int);
    const char*			parentType() const
				{ return typeid(uiODVw2DTreeTop).name(); }
    Pick::SetMgr&		picksetmgr_;

public:
    void			setupNewPickSet(const MultiID&);
};


mExpClass(uiODMain)
uiODVw2DPickSetTreeItemFactory : public uiODVw2DTreeItemFactory
{
public:
    const char*		name() const		{ return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODVw2DPickSetParentTreeItem(); }
    uiTreeItem*		createForVis(const uiODViewer2D&,int visid) const;
};


mExpClass(uiODMain) uiODVw2DPickSetTreeItem : public uiODVw2DTreeItem
{ mODTextTranslationClass(uiODVw2DPickSetTreeItem)
public:
			uiODVw2DPickSetTreeItem(int picksetid);
			uiODVw2DPickSetTreeItem(int id,bool dummy);
			~uiODVw2DPickSetTreeItem();

    bool		showSubMenu();
    bool		select();
    const MultiID&	pickMultiID() const;
    const VW2DPickSet*	vw2DObject() const	{ return vw2dpickset_; }
    void		keyPressedCB(CallBacker*);

protected:

    bool		init();
    const char*		parentType() const
			{ return typeid(uiODVw2DPickSetParentTreeItem).name(); }
    bool		isSelectable() const			{ return true; }

    void		deSelCB(CallBacker*);
    void		checkCB(CallBacker*);
    void		displayChangedCB(CallBacker*);
    void		displayMiniCtab();
    void		removePickSetCB(CallBacker*);

    Pick::SetMgr&	picksetmgr_;
    RefMan<Pick::Set>	pickset_;
    VW2DPickSet*	vw2dpickset_		= nullptr;
};
