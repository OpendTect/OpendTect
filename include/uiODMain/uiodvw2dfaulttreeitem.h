#pragma once

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Mar 2009
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uiodvw2dtreeitem.h"
#include "emposid.h"

class uiODViewer2D;
namespace View2D { class Fault; }


mExpClass(uiODMain) uiODVw2DFaultParentTreeItem : public uiODVw2DTreeItem
{ mODTextTranslationClass(uiODVw2DFaultParentTreeItem);
public:
    				uiODVw2DFaultParentTreeItem();
				~uiODVw2DFaultParentTreeItem();

    bool			showSubMenu();
    void			getFaultVwr2DIDs(EM::ObjectID emid,
						 TypeSet<int>& vw2dids ) const;
    void			getLoadedFaults(
					TypeSet<EM::ObjectID>& emids) const;
    void			removeFault(EM::ObjectID);
    void			addFaults(const TypeSet<EM::ObjectID>&);
    void			addNewTempFault(EM::ObjectID emid);
    void			setupNewTempFault(EM::ObjectID emid);

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


mExpClass(uiODMain) uiODVw2DFaultTreeItem : public uiODVw2DTreeItem
{ mODTextTranslationClass(uiODVw2DFaultTreeItem);
public:
    			uiODVw2DFaultTreeItem(const EM::ObjectID&);
    			uiODVw2DFaultTreeItem(int dispid,bool dummy);
			~uiODVw2DFaultTreeItem();

    bool			showSubMenu();
    bool			select();
    EM::ObjectID		emObjectID() const	{ return emid_; }
    const View2D::Fault*	vw2DObject() const	{ return faultview_; }

protected:

    bool		init();
    const char*		parentType() const
			{ return typeid(uiODVw2DFaultParentTreeItem).name(); }
    bool		isSelectable() const			{ return true; }

    void		updateCS(const TrcKeyZSampling&,bool upd);
    void		deSelCB(CallBacker*);
    void		checkCB(CallBacker*);
    void		emobjAbtToDelCB(CallBacker*);
    void		displayMiniCtab();

    void		emobjChangeCB(CallBacker*);
    void		enableKnotsCB(CallBacker*);
    void		propChgCB(CallBacker*);

    EM::ObjectID	emid_;
    View2D::Fault*	faultview_;
};
