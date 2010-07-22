#ifndef uiodvw2dfaultsstreeitem_h
#define uiodvw2dfaultsstreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id: uiodvw2dfaultsstreeitem.h,v 1.2 2010-07-22 05:19:08 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uiodvw2dtreeitem.h"

#include "emposid.h"

class VW2DFautSS3D;


mClass uiODVw2DFaultSSParentTreeItem : public uiODVw2DTreeItem
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


mClass uiODVw2DFaultSSTreeItemFactory : public uiTreeItemFactory
{
public:
    const char*         name() const            { return typeid(*this).name(); }
    uiTreeItem*         create() const
			{ return new uiODVw2DFaultSSParentTreeItem(); }
};


mClass uiODVw2DFaultSSTreeItem : public uiODVw2DTreeItem
{
public:
    			uiODVw2DFaultSSTreeItem(const EM::ObjectID&);
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

    const int		cPixmapWidth()				{ return 16; }
    const int		cPixmapHeight()				{ return 10; }
    void		emobjChangeCB(CallBacker*);

    EM::ObjectID	emid_;
    VW2DFautSS3D*	fssview_;
};

#endif
