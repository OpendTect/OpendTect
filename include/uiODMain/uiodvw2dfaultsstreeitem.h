#ifndef uiodvw2dfaultsstreeitem_h
#define uiodvw2dfaultsstreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uiodvw2dtreeitem.h"

#include "emposid.h"

class VW2DFaultSS3D;
class uiODViewer2D;


mClass(uiODMain) uiODVw2DFaultSSParentTreeItem : public uiODVw2DTreeItem
{
public:
    				uiODVw2DFaultSSParentTreeItem();
				~uiODVw2DFaultSSParentTreeItem();

    bool			showSubMenu();

protected:

    bool			init();
    bool			handleSubMenu(int);
    const char*			parentType() const
    				{ return typeid(uiODVw2DTreeTop).name(); }
    void			tempObjAddedCB(CallBacker*);
};


mClass(uiODMain) uiODVw2DFaultSSTreeItemFactory : public uiODVw2DTreeItemFactory
{
public:
    const char*         name() const            { return typeid(*this).name(); }
    uiTreeItem*         create() const
			{ return new uiODVw2DFaultSSParentTreeItem(); }
    uiTreeItem*         createForVis(const uiODViewer2D&,int visid) const;
};


mClass(uiODMain) uiODVw2DFaultSSTreeItem : public uiODVw2DTreeItem
{
public:
    			uiODVw2DFaultSSTreeItem(const EM::ObjectID&);
    			uiODVw2DFaultSSTreeItem(int displayid,bool dummy);
			~uiODVw2DFaultSSTreeItem();
    
    bool		showSubMenu();
    bool		select();

protected:

    bool		init();
    const char*		parentType() const
			{ return typeid(uiODVw2DFaultSSParentTreeItem).name(); }
    bool		isSelectable() const			{ return true; }

    void		updateCS(const CubeSampling&,bool upd);
    void		deSelCB(CallBacker*);
    void		checkCB(CallBacker*);
    void		emobjAbtToDelCB(CallBacker*);
    void		displayMiniCtab();

    int			cPixmapWidth()				{ return 16; }
    int			cPixmapHeight()				{ return 10; }
    void		emobjChangeCB(CallBacker*);

    EM::ObjectID	emid_;
    VW2DFaultSS3D*	fssview_;
};

#endif

