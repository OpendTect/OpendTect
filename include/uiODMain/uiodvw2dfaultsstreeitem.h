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

#include "emposid.h"

class VW2DFaultSS3D;
class uiODViewer2D;


mExpClass(uiODMain) uiODVw2DFaultSSParentTreeItem : public uiODVw2DTreeItem
{ mODTextTranslationClass(uiODVw2DFaultSSParentTreeItem);
public:
    				uiODVw2DFaultSSParentTreeItem();
				~uiODVw2DFaultSSParentTreeItem();

    bool			showSubMenu();
    void			getFaultSSVwr2DIDs(EM::ObjectID emid,
						   TypeSet<int>& vw2ids ) const;
    void			getLoadedFaultSSs(TypeSet<EM::ObjectID>&) const;
    void			removeFaultSS(EM::ObjectID);
    void			addFaultSSs(const TypeSet<EM::ObjectID>&);
    void			addNewTempFaultSS(EM::ObjectID emid);
    void			setupNewTempFaultSS(EM::ObjectID emid);

protected:

    bool			init();
    const char*			iconName() const;
    bool			handleSubMenu(int);
    const char*			parentType() const
    				{ return typeid(uiODVw2DTreeTop).name(); }
};


mExpClass(uiODMain) uiODVw2DFaultSSTreeItemFactory
				: public uiODVw2DTreeItemFactory
{
public:
    const char*         name() const            { return typeid(*this).name(); }
    uiTreeItem*         create() const
			{ return new uiODVw2DFaultSSParentTreeItem(); }
    uiTreeItem*         createForVis(const uiODViewer2D&,int visid) const;
};


mExpClass(uiODMain) uiODVw2DFaultSSTreeItem : public uiODVw2DEMTreeItem
{ mODTextTranslationClass(uiODVw2DFaultSSTreeItem);
public:
    			uiODVw2DFaultSSTreeItem(const EM::ObjectID&);
    			uiODVw2DFaultSSTreeItem(int displayid,bool dummy);
			~uiODVw2DFaultSSTreeItem();
    
    bool		showSubMenu();
    bool		select();
    EM::ObjectID	emObjectID() const	{ return emid_; }
    const Vw2DDataObject* vw2DObject() const;

protected:

    bool		init();
    const char*		parentType() const
			{ return typeid(uiODVw2DFaultSSParentTreeItem).name(); }
    bool		isSelectable() const			{ return true; }

    void		updateCS(const TrcKeyZSampling&,bool upd);
    void		deSelCB(CallBacker*);
    void		checkCB(CallBacker*);
    void		emobjAbtToDelCB(CallBacker*);
    void		displayMiniCtab();

    void		emobjChangeCB(CallBacker*);
    void		enableKnotsCB(CallBacker*);

    VW2DFaultSS3D*	fssview_;
};

