#pragma once

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uiodvw2demtreeitem.h"
#include "uistring.h"

class Vw2DHorizon2D;


mExpClass(uiODMain) uiODVw2DHor2DParentTreeItem : public uiODVw2DTreeItem
{ mODTextTranslationClass(uiODVw2DHor2DParentTreeItem)
public:

				uiODVw2DHor2DParentTreeItem();
				~uiODVw2DHor2DParentTreeItem();

    bool			showSubMenu();
    void			getHor2DVwr2DIDs(const DBKey& emid,
						 TypeSet<int>& vw2dids ) const;
    void			getLoadedHorizon2Ds(DBKeySet&) const;
    void			removeHorizon2D(const DBKey& emid);
    void			addHorizon2Ds(const DBKeySet&);
    void			addNewTrackingHorizon2D(const DBKey& emid);
    void			setupTrackingHorizon2D(const DBKey& emid);

protected:

    bool			init();
    const char*			iconName() const;
    bool			handleSubMenu(int);
    const char*			parentType() const
				{ return typeid(uiODVw2DTreeTop).name(); }
    void			getNonLoadedTrackedHor2Ds(DBKeySet&);

};


mExpClass(uiODMain)
uiODVw2DHor2DTreeItemFactory : public uiODVw2DTreeItemFactory
{
public:

    const char*		name() const	{ return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODVw2DHor2DParentTreeItem(); }
    uiTreeItem*         createForVis(const uiODViewer2D&,int visid) const;

};


mExpClass(uiODMain) uiODVw2DHor2DTreeItem : public uiODVw2DEMTreeItem
{ mODTextTranslationClass(uiODVw2DHor2DTreeItem)
public:

			uiODVw2DHor2DTreeItem(const DBKey&);
			uiODVw2DHor2DTreeItem(int dispid,bool dummy);
			~uiODVw2DHor2DTreeItem();

    bool		showSubMenu();
    bool		select();
    const Vw2DDataObject* vw2DObject() const;

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

    Vw2DHorizon2D*	horview_;

};
