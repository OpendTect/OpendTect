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

class uiODViewer2D;
namespace View2D { class Fault; }


mExpClass(uiODMain) uiODView2DFaultParentTreeItem : public uiODView2DTreeItem
{ mODTextTranslationClass(uiODView2DFaultParentTreeItem);
public:
				uiODView2DFaultParentTreeItem();
				~uiODView2DFaultParentTreeItem();

    bool			showSubMenu();
    void			getFaultVwr2DIDs(EM::ObjectID emid,
						 TypeSet<Vis2DID>&) const;
    void			getLoadedFaults(TypeSet<EM::ObjectID>&) const;
    void			removeFault(EM::ObjectID);
    void			addFaults(const TypeSet<EM::ObjectID>&);
    void			addNewTempFault(EM::ObjectID);
    void			setupNewTempFault(EM::ObjectID);

protected:

    bool			init();
    const char*			iconName() const;
    bool			handleSubMenu(int);
    const char*			parentType() const
				{ return typeid(uiODView2DTreeTop).name(); }
};


mExpClass(uiODMain) uiODView2DFaultTreeItemFactory
				: public uiODView2DTreeItemFactory
{
public:
    const char*		name() const		{ return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODView2DFaultParentTreeItem(); }
    uiTreeItem*		createForVis(const uiODViewer2D&,Vis2DID) const;
};


mExpClass(uiODMain) uiODView2DFaultTreeItem : public uiODView2DTreeItem
{ mODTextTranslationClass(uiODView2DFaultTreeItem)
public:
			uiODView2DFaultTreeItem(const EM::ObjectID&);
			uiODView2DFaultTreeItem(Vis2DID,bool dummy);
			~uiODView2DFaultTreeItem();

    bool			showSubMenu();
    bool			select();
    EM::ObjectID		emObjectID() const	{ return emid_; }
    const View2D::Fault*	vw2DObject() const	{ return faultview_; }

protected:

    bool		init();
    const char*		parentType() const
			{ return typeid(uiODView2DFaultParentTreeItem).name(); }
    bool		isSelectable() const			{ return true; }

    void		updateCS(const TrcKeyZSampling&,bool upd);
    void		deSelCB(CallBacker*);
    void		checkCB(CallBacker*);
    void		emobjAbtToDelCB(CallBacker*);
    void		displayMiniCtab();

    void		emobjChangeCB(CallBacker*);
    void		enableKnotsCB(CallBacker*);
    void		propChgCB(CallBacker*);

    EM::ObjectID	emid_;
    View2D::Fault*	faultview_;
};
