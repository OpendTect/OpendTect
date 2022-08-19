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


namespace View2D { class FaultSS2D; }
class uiODViewer2D;


mExpClass(uiODMain) uiODView2DFaultSS2DParentTreeItem
					: public uiODView2DTreeItem
{ mODTextTranslationClass(uiODView2DFaultSS2DParentTreeItem)
public:
				uiODView2DFaultSS2DParentTreeItem();
				~uiODView2DFaultSS2DParentTreeItem();

    bool			showSubMenu();

protected:

    bool			init();
    const char*			iconName() const;
    bool			handleSubMenu(int);
    const char*			parentType() const
				{ return typeid(uiODView2DTreeTop).name(); }
    void			tempObjAddedCB(CallBacker*);

public:
    void			getFaultSS2DVwr2DIDs(EM::ObjectID emid,
						     TypeSet<Vis2DID>&) const;
    void			getLoadedFaultSS2Ds(
					TypeSet<EM::ObjectID>&) const;
    void			removeFaultSS2D(EM::ObjectID);
    void			addFaultSS2Ds(const TypeSet<EM::ObjectID>&);
    void			addNewTempFaultSS2D(EM::ObjectID emid);
    void			setupNewTempFaultSS2D(EM::ObjectID emid);

};


mExpClass(uiODMain) uiODView2DFaultSS2DTreeItemFactory
				: public uiODView2DTreeItemFactory
{
public:
    const char*		name() const		{ return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODView2DFaultSS2DParentTreeItem(); }
    uiTreeItem*		createForVis(const uiODViewer2D&,Vis2DID visid) const;
};


mExpClass(uiODMain) uiODView2DFaultSS2DTreeItem : public uiODView2DTreeItem
{ mODTextTranslationClass(uiODView2DFaultSS2DTreeItem)
public:
			uiODView2DFaultSS2DTreeItem(const EM::ObjectID&);
			uiODView2DFaultSS2DTreeItem(Vis2DID dispid,bool dummy);
			~uiODView2DFaultSS2DTreeItem();

    bool			showSubMenu();
    bool			select();
    EM::ObjectID		emObjectID() const	{ return emid_; }
    const View2D::FaultSS2D*	vw2DObject() const	{ return fssview_; }

protected:

    bool		init();
    const char*		parentType() const;
    bool		isSelectable() const			{ return true; }

    void		deSelCB(CallBacker*);
    void		checkCB(CallBacker*);
    void		emobjAbtToDelCB(CallBacker*);
    void		displayMiniCtab();

    void		emobjChangeCB(CallBacker*);
    void		enableKnotsCB(CallBacker*);
    void		propChgCB(CallBacker*);

    EM::ObjectID	emid_;
    View2D::FaultSS2D*	fssview_;
};
