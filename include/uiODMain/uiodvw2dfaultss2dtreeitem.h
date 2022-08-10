#pragma once

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uiodvw2dtreeitem.h"
#include "emposid.h"


namespace View2D { class FaultSS2D; }
class uiODViewer2D;


mExpClass(uiODMain) uiODVw2DFaultSS2DParentTreeItem : public uiODVw2DTreeItem
{ mODTextTranslationClass(uiODVw2DFaultSS2DParentTreeItem)
public:
				uiODVw2DFaultSS2DParentTreeItem();
				~uiODVw2DFaultSS2DParentTreeItem();

    bool			showSubMenu();

protected:

    bool			init();
    const char*			iconName() const;
    bool			handleSubMenu(int);
    const char*			parentType() const
				{ return typeid(uiODVw2DTreeTop).name(); }
    void			tempObjAddedCB(CallBacker*);

public:
    void			getFaultSS2DVwr2DIDs(EM::ObjectID emid,
						     TypeSet<int>& vids ) const;
    void			getLoadedFaultSS2Ds(
					TypeSet<EM::ObjectID>&) const;
    void			removeFaultSS2D(EM::ObjectID);
    void			addFaultSS2Ds(const TypeSet<EM::ObjectID>&);
    void			addNewTempFaultSS2D(EM::ObjectID emid);
    void			setupNewTempFaultSS2D(EM::ObjectID emid);

};


mExpClass(uiODMain) uiODVw2DFaultSS2DTreeItemFactory
				: public uiODVw2DTreeItemFactory
{
public:
    const char*		name() const		{ return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODVw2DFaultSS2DParentTreeItem(); }
    uiTreeItem*		createForVis(const uiODViewer2D&,int visid) const;
};


mExpClass(uiODMain) uiODVw2DFaultSS2DTreeItem : public uiODVw2DTreeItem
{ mODTextTranslationClass(uiODVw2DFaultSS2DTreeItem)
public:
    			uiODVw2DFaultSS2DTreeItem(const EM::ObjectID&);
			uiODVw2DFaultSS2DTreeItem(int dispid,bool dummy);
			~uiODVw2DFaultSS2DTreeItem();

    bool			showSubMenu();
    bool			select();
    EM::ObjectID		emObjectID() const	{ return emid_; }
    const View2D::FaultSS2D*	vw2DObject() const	{ return fssview_; }

protected:

    bool		init();
    const char*		parentType() const
			{return typeid(uiODVw2DFaultSS2DParentTreeItem).name();}
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
