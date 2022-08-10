#pragma once

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uiodvw2dtreeitem.h"
#include "emposid.h"

namespace View2D { class Horizon2D; }

mExpClass(uiODMain) uiODVw2DHor2DParentTreeItem : public uiODVw2DTreeItem
{ mODTextTranslationClass(uiODVw2DHor2DParentTreeItem);
public:
				uiODVw2DHor2DParentTreeItem();
				~uiODVw2DHor2DParentTreeItem();

    bool			showSubMenu();
    void			getHor2DVwr2DIDs(EM::ObjectID emid,
						 TypeSet<int>& vw2dids ) const;
    void			getLoadedHorizon2Ds(
					TypeSet<EM::ObjectID>&) const;
    void			removeHorizon2D(EM::ObjectID emid);
    void			addHorizon2Ds(const TypeSet<EM::ObjectID>&);
    void			addNewTrackingHorizon2D(EM::ObjectID emid);
    void			setupTrackingHorizon2D(EM::ObjectID emid);

protected:

    bool			init();
    const char*			iconName() const;
    bool			handleSubMenu(int);
    const char*			parentType() const
				{ return typeid(uiODVw2DTreeTop).name(); }
    void			getNonLoadedTrackedHor2Ds(
					TypeSet<EM::ObjectID>&);
};


mExpClass(uiODMain)
uiODVw2DHor2DTreeItemFactory : public uiODVw2DTreeItemFactory
{
public:
    const char*		name() const 	{ return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODVw2DHor2DParentTreeItem(); }
    uiTreeItem*		createForVis(const uiODViewer2D&,int visid) const;
};


mExpClass(uiODMain) uiODVw2DHor2DTreeItem : public uiODVw2DTreeItem
{ mODTextTranslationClass(uiODVw2DHor2DTreeItem)
public:
    			uiODVw2DHor2DTreeItem(const EM::ObjectID&);
    			uiODVw2DHor2DTreeItem(int dispid,bool dummy);
			~uiODVw2DHor2DTreeItem();

    bool			showSubMenu();
    bool			select();
    EM::ObjectID		emObjectID() const	{ return emid_; }
    const View2D::Horizon2D*	vw2DObject() const	{ return horview_; }

protected:

    bool		init();
    const char*		parentType() const
			{ return typeid(uiODVw2DHor2DParentTreeItem).name(); }
    bool		isSelectable() const			{ return true; }

    void		updateSelSpec(const Attrib::SelSpec*,bool wva);
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
