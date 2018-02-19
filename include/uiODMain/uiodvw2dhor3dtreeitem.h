#pragma once

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uiodvw2demtreeitem.h"

class Vw2DHorizon3D;
class uiODViewer2D;


mExpClass(uiODMain) uiODVw2DHor3DParentTreeItem : public uiODVw2DTreeItem
{ mODTextTranslationClass(uiODVw2DHor3DParentTreeItem);
public:
				uiODVw2DHor3DParentTreeItem();
				~uiODVw2DHor3DParentTreeItem();

    bool			showSubMenu();
    void			getHor3DVwr2DIDs(const DBKey& emid,
						 TypeSet<int>& vw2dids) const;
    void			getLoadedHorizon3Ds(DBKeySet&) const;
    void			removeHorizon3D(const DBKey& emid);
    void			addHorizon3Ds(const DBKeySet&);
    void			addNewTrackingHorizon3D(const DBKey& emid);
    void			setupTrackingHorizon3D(const DBKey& emid);

protected:

    bool			init();
    const char*			iconName() const;
    bool			handleSubMenu(int);
    const char*			parentType() const
				{ return typeid(uiODVw2DTreeTop).name(); }
    void			getNonLoadedTrackedHor3Ds(DBKeySet&);
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


mExpClass(uiODMain) uiODVw2DHor3DTreeItem : public uiODVw2DEMTreeItem
{ mODTextTranslationClass(uiODVw2DHor3DTreeItem)
public:
			uiODVw2DHor3DTreeItem(const DBKey&);
			uiODVw2DHor3DTreeItem(int id,bool dummy);
			~uiODVw2DHor3DTreeItem();

    bool		select();
    bool		showSubMenu();
    const Vw2DDataObject* vw2DObject() const;

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

    Vw2DHorizon3D*	horview_;
    void		emobjAbtToDelCB(CallBacker*);

};
