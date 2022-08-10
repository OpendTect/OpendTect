#pragma once

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uiodvw2dtreeitem.h"
#include "emposid.h"

namespace View2D { class Horizon3D; }
class uiODViewer2D;


mExpClass(uiODMain) uiODVw2DHor3DParentTreeItem : public uiODVw2DTreeItem
{ mODTextTranslationClass(uiODVw2DHor3DParentTreeItem);
public:
    				uiODVw2DHor3DParentTreeItem();
				~uiODVw2DHor3DParentTreeItem();

    bool			showSubMenu();
    void			getHor3DVwr2DIDs(EM::ObjectID emid,
						 TypeSet<int>& vw2dids) const;
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
				{ return typeid(uiODVw2DTreeTop).name(); }
    void			getNonLoadedTrackedHor3Ds(
					TypeSet<EM::ObjectID>&);
public:
    void			setupTrackingHorizon3D(EM::ObjectID emid);
};


mExpClass(uiODMain) uiODVw2DHor3DTreeItemFactory
    : public uiODVw2DTreeItemFactory
{
public:
    const char*		name() const		{ return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODVw2DHor3DParentTreeItem(); }
    uiTreeItem*		createForVis(const uiODViewer2D&,int visid) const;
};


mExpClass(uiODMain) uiODVw2DHor3DTreeItem : public uiODVw2DTreeItem
{ mODTextTranslationClass(uiODVw2DHor3DTreeItem)
public:
    			uiODVw2DHor3DTreeItem(const EM::ObjectID&);
    			uiODVw2DHor3DTreeItem(int id,bool dummy);
			~uiODVw2DHor3DTreeItem();

    bool			select();
    bool			showSubMenu();
    EM::ObjectID		emObjectID() const	{ return emid_; }
    const View2D::Horizon3D*	vw2DObject() const	{ return horview_; }

protected:

    bool		init();
    const char*		parentType() const
    			{ return typeid(uiODVw2DHor3DParentTreeItem).name(); }
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
