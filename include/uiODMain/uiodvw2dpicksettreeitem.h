#ifndef uiodvw2dpicksettreeitem_h
#define uiodvw2dpicksettreeitem_h

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

class VW2DPickSet;

namespace Pick{ class Set; }



mExpClass(uiODMain) uiODVw2DPickSetParentTreeItem
			: public uiODVw2DParentTreeItem
{ mODTextTranslationClass(uiODVw2DPickSetParentTreeItem);
public:
				uiODVw2DPickSetParentTreeItem();
				~uiODVw2DPickSetParentTreeItem();

    bool			showSubMenu();
    void			getPickSetVwr2DIDs(const MultiID& mid,
						   TypeSet<int>& vw2ids) const;
    void			getLoadedPickSets(TypeSet<MultiID>&) const;
    void			showHidePickSet(const MultiID&,bool);
    void			removePickSet(const MultiID&);
    void			addPickSets(const TypeSet<MultiID>&);
    void			setupNewPickSet(const MultiID&);

protected:

    bool			init();
    const char*			iconName() const;
    bool			handleSubMenu(int);
    const char*			parentType() const
				{ return typeid(uiODVw2DTreeTop).name(); }
    void			setActive(const MultiID&);

    void			objAddedCB(CallBacker*);
    void			objVanishedCB(CallBacker*);
    void			objShownCB(CallBacker*);
    void			objHiddenCB(CallBacker*);
    void			objOrphanedCB(CallBacker*);

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
			uiODVw2DPickSetTreeItem(Pick::Set&);
			~uiODVw2DPickSetTreeItem();

    bool		showSubMenu();
    bool		select();
    MultiID		pickSetID() const;
    const VW2DPickSet*	vw2DObject() const	{ return vw2dpickset_; }
    virtual void	doSave();
    virtual void	doSaveAs();

    void		enableDisplay(bool);


protected:

    const char*		parentType() const
			{ return typeid(uiODVw2DPickSetParentTreeItem).name(); }
    bool		init();
    bool		isSelectable() const			{ return true; }

    void		deSelCB(CallBacker*);
    void		checkCB(CallBacker*);
    void		setChangedCB(CallBacker*);
    void		displayMiniCtab();

    Pick::Set&		pickset_;
    VW2DPickSet*	vw2dpickset_;

};


#endif
