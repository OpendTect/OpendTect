#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"

#include "emposid.h"
#include "uiodvw2dtreeitem.h"


namespace View2D { class FaultSS2D; }
class uiODViewer2D;


mExpClass(uiODMain) uiODView2DFaultSS2DParentTreeItem
					: public uiODView2DTreeItem
{ mODTextTranslationClass(uiODView2DFaultSS2DParentTreeItem)
public:
				uiODView2DFaultSS2DParentTreeItem();
				~uiODView2DFaultSS2DParentTreeItem();

    bool			showSubMenu() override;

private:

    bool			init() override;
    const char*			iconName() const override;
    bool			handleSubMenu(int);
    const char*			parentType() const override
				{ return typeid(uiODView2DTreeTop).name(); }
    void			tempObjAddedCB(CallBacker*);

public:
    void			getFaultSS2DVwr2DIDs(const EM::ObjectID&,
						     TypeSet<Vis2DID>&) const;
    void			getLoadedFaultSS2Ds(
					TypeSet<EM::ObjectID>&) const;
    void			removeFaultSS2D(const EM::ObjectID&);
    void			addFaultSS2Ds(const TypeSet<EM::ObjectID>&);
    void			addNewTempFaultSS2D(const EM::ObjectID&);
    void			setupNewTempFaultSS2D(const EM::ObjectID&);

};


mExpClass(uiODMain) uiODView2DFaultSS2DTreeItemFactory
				: public uiODView2DTreeItemFactory
{
public:
    const char*		name() const override	{ return typeid(*this).name(); }
    uiTreeItem*		create() const override
			{ return new uiODView2DFaultSS2DParentTreeItem(); }
    uiTreeItem*		createForVis(const uiODViewer2D&,
				     const Vis2DID&) const override;
};


mExpClass(uiODMain) uiODView2DFaultSS2DTreeItem : public uiODView2DTreeItem
{ mODTextTranslationClass(uiODView2DFaultSS2DTreeItem)
public:
			uiODView2DFaultSS2DTreeItem(const EM::ObjectID&);
			uiODView2DFaultSS2DTreeItem(const Vis2DID&,bool dummy);
			~uiODView2DFaultSS2DTreeItem();

    bool			showSubMenu() override;
    bool			select() override;
    EM::ObjectID		emObjectID() const	{ return emid_; }
    const View2D::FaultSS2D*	vw2DObject() const	{ return fssview_; }

protected:

    bool		init() override;
    const char*		parentType() const override;
    bool		isSelectable() const override		{ return true; }

    void		deSelCB(CallBacker*);
    void		checkCB(CallBacker*);
    void		emobjAbtToDelCB(CallBacker*);
    void		displayMiniCtab();

    void		emobjChangeCB(CallBacker*);
    void		enableKnotsCB(CallBacker*);
    void		propChgCB(CallBacker*);

    EM::ObjectID	emid_;
    View2D::FaultSS2D*	fssview_	= nullptr;
};
