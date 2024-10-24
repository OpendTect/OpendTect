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

namespace View2D { class FaultSS3D; }
class uiODViewer2D;


mExpClass(uiODMain) uiODView2DFaultSSParentTreeItem : public uiODView2DTreeItem
{ mODTextTranslationClass(uiODView2DFaultSSParentTreeItem)
public:
				uiODView2DFaultSSParentTreeItem();
				~uiODView2DFaultSSParentTreeItem();

    bool			showSubMenu() override;
    void			getFaultSSVwr2DIDs(const EM::ObjectID&,
						   TypeSet<Vis2DID>&) const;
    void			getLoadedFaultSSs(TypeSet<EM::ObjectID>&) const;
    void			removeFaultSS(const EM::ObjectID&);
    void			addFaultSSs(const TypeSet<EM::ObjectID>&);
    void			addNewTempFaultSS(const EM::ObjectID&);
    void			setupNewTempFaultSS(const EM::ObjectID&);

private:

    bool			init() override;
    const char*			iconName() const override;
    bool			handleSubMenu(int);
    const char*			parentType() const override
				{ return typeid(uiODView2DTreeTop).name(); }
};


mExpClass(uiODMain) uiODView2DFaultSSTreeItemFactory
				: public uiODView2DTreeItemFactory
{
public:
    const char*		name() const override	{ return typeid(*this).name(); }
    uiTreeItem*		create() const override
			{ return new uiODView2DFaultSSParentTreeItem(); }
    uiTreeItem*		createForVis(const uiODViewer2D&,
				     const Vis2DID&) const override;
};


mExpClass(uiODMain) uiODView2DFaultSSTreeItem : public uiODView2DTreeItem
{ mODTextTranslationClass(uiODView2DFaultSSTreeItem)
public:
			uiODView2DFaultSSTreeItem(const EM::ObjectID&);
			uiODView2DFaultSSTreeItem(const Vis2DID&,bool dummy);
			~uiODView2DFaultSSTreeItem();

    bool		showSubMenu() override;
    bool		select() override;
    EM::ObjectID	emObjectID() const	{ return emid_; }
    const View2D::FaultSS3D* vw2DObject() const	{ return fssview_; }

private:

    bool		init() override;
    const char*		parentType() const override
			{return typeid(uiODView2DFaultSSParentTreeItem).name();}
    bool		isSelectable() const override		{ return true; }

    void		updateCS(const TrcKeyZSampling&,bool upd) override;
    void		deSelCB(CallBacker*);
    void		checkCB(CallBacker*);
    void		emobjAbtToDelCB(CallBacker*);
    void		displayMiniCtab();

    void		emobjChangeCB(CallBacker*);
    void		enableKnotsCB(CallBacker*);
    void		propChgCB(CallBacker*);

    EM::ObjectID	emid_;
    View2D::FaultSS3D*	fssview_	= nullptr;
};
