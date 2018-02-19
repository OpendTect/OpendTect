#pragma once

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Mar 2009
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uiodvw2demtreeitem.h"

class VW2DFault;
class uiODViewer2D;


mExpClass(uiODMain) uiODVw2DFaultParentTreeItem : public uiODVw2DTreeItem
{ mODTextTranslationClass(uiODVw2DFaultParentTreeItem)
public:

				uiODVw2DFaultParentTreeItem();
				~uiODVw2DFaultParentTreeItem();

    bool			showSubMenu();
    void			getFaultVwr2DIDs(const DBKey& emid,
						 TypeSet<int>& vw2dids ) const;
    void			getLoadedFaults(DBKeySet& emids) const;
    void			removeFault(const DBKey&);
    void			addFaults(const DBKeySet&);
    void			addNewTempFault(const DBKey& emid);
    void			setupNewTempFault(const DBKey& emid);

protected:

    bool			init();
    const char*			iconName() const;
    bool			handleSubMenu(int);
    const char*			parentType() const
				{ return typeid(uiODVw2DTreeTop).name(); }
};


mExpClass(uiODMain) uiODVw2DFaultTreeItemFactory
				: public uiODVw2DTreeItemFactory
{
public:
    const char*		name() const		{ return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODVw2DFaultParentTreeItem(); }
    uiTreeItem*		createForVis(const uiODViewer2D&,int visid) const;
};


mExpClass(uiODMain) uiODVw2DFaultTreeItem : public uiODVw2DEMTreeItem
{ mODTextTranslationClass(uiODVw2DFaultTreeItem)
public:
			uiODVw2DFaultTreeItem(const DBKey&);
			uiODVw2DFaultTreeItem(int dispid,bool dummy);
			~uiODVw2DFaultTreeItem();

    bool		showSubMenu();
    bool		select();
    const Vw2DDataObject* vw2DObject() const;

protected:

    bool		init();
    const char*		parentType() const
			{ return typeid(uiODVw2DFaultParentTreeItem).name(); }
    bool		isSelectable() const			{ return true; }

    void		updateCS(const TrcKeyZSampling&,bool upd);
    void		deSelCB(CallBacker*);
    void		checkCB(CallBacker*);
    void		emobjAbtToDelCB(CallBacker*);

    void		enableKnotsCB(CallBacker*);
    VW2DFault*		faultview_;
};
