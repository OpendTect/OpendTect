#pragma once

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uiodvw2demtreeitem.h"
#include "uistring.h"


class VW2DFaultSS2D;
class uiODViewer2D;


mExpClass(uiODMain) uiODVw2DFaultSS2DParentTreeItem : public uiODVw2DTreeItem
{ mODTextTranslationClass(uiODVw2DFaultSS2DParentTreeItem);
public:
				uiODVw2DFaultSS2DParentTreeItem();
				~uiODVw2DFaultSS2DParentTreeItem();

    bool			showSubMenu();
    void			getFaultSS2DVwr2DIDs(const DBKey& emid,
						     TypeSet<int>& vids ) const;
    void			getLoadedFaultSS2Ds(DBKeySet&) const;
    void			removeFaultSS2D(const DBKey&);
    void			addFaultSS2Ds(const DBKeySet&);
    void			addNewTempFaultSS2D(const DBKey& emid);
    void			setupNewTempFaultSS2D(const DBKey& emid);

protected:

    bool			init();
    const char*			iconName() const;
    bool			handleSubMenu(int);
    const char*			parentType() const
				{ return typeid(uiODVw2DTreeTop).name(); }
    void			tempObjAddedCB(CallBacker*);

};


mExpClass(uiODMain) uiODVw2DFaultSS2DTreeItemFactory
				: public uiODVw2DTreeItemFactory
{
public:
    const char*		name() const		{ return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODVw2DFaultSS2DParentTreeItem(); }
    uiTreeItem*         createForVis(const uiODViewer2D&,int visid) const;
};


mExpClass(uiODMain) uiODVw2DFaultSS2DTreeItem : public uiODVw2DEMTreeItem
{ mODTextTranslationClass(uiODVw2DFaultSS2DTreeItem);
public:
			uiODVw2DFaultSS2DTreeItem(const DBKey&);
			uiODVw2DFaultSS2DTreeItem(int dispid,bool dummy);
			~uiODVw2DFaultSS2DTreeItem();

    bool		showSubMenu();
    bool		select();
    const Vw2DDataObject* vw2DObject() const;

protected:

    bool		init();
    const char*		parentType() const
			{return typeid(uiODVw2DFaultSS2DParentTreeItem).name();}
    bool		isSelectable() const			{ return true; }

    void		deSelCB(CallBacker*);
    void		checkCB(CallBacker*);
    void		emobjAbtToDelCB(CallBacker*);

    void		enableKnotsCB(CallBacker*);

    VW2DFaultSS2D*	fssview_;
};
